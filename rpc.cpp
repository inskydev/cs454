#include "rpc.h"
#include "util.h"
#include "Binder.h"
#include "Server.h"

#define UNSUPPORTED_CALL "UNSUPPORTED_CALL"
#define EXCEED_ARRAY_LEN "EXCEED_ARRAY_LEN"

struct RPCServer : public Server {
  // RPC server handles 10 concurrent requests.
  RPCServer(int binderSock) : Server(HostPort::SERVER, binderSock, 10) {
  }

  virtual ~RPCServer() {
    map<string, int*>::iterator beg = rpcArgsMapping.begin();
    map<string, int*>::iterator end = rpcArgsMapping.end();
    for (; beg != end; beg++) {
      delete [] beg->second;
    }
  }

  virtual void connected(int socketid)  {}
  virtual void disconnected(int socketid) {}
  virtual void handleRequest(int socketid, const string& msg) {
    if (msg[0] == 'T') {
      ++terminating; // stop accept new clients.
      //std::cout << "Server terminating" << endl;
    } else if (msg[0] == 'C') {
      // deserialize ..
      string normalized = msg.substr(1, msg.find('#') - 1);
      map<string, pair<string, skeleton> >::iterator it =
        rpcHandlerMapping.find(normalized);
      if (it == rpcHandlerMapping.end()) {
        sendString(socketid, UNSUPPORTED_CALL);
      } else {
        // Iterate through each argument check array length.
        int* args = rpcArgsMapping[normalized];
        int len = 0; while (args[len] != 0) len++;
        string argStr = msg.substr(msg.find('#') + 1);
        //cout << "Arg str " << argStr <<  " " << argStr.size() << endl;
        bool arrayLenWarning = false;

        void** param = (void**) malloc(sizeof(void*) * len);
        // For every argument.
        for (int l = 0; l < len; l++) {
          int type = (args[l] >> 16) & 0xFF;
          int length = (args[l]) & 0xFF;
          ASSERT(not argStr.empty(), "need more stirng to process");

          size_t delim_position = argStr.find(':');
          int actualLength = atoi(argStr.substr(0, delim_position).c_str());
          if (actualLength > length && length != 0) {
            // note: ignore scalar oversize here.
            arrayLenWarning = true;
          }

          void* argument = NULL;
          if (type == ARG_CHAR) {
            char* v = new char[actualLength];
            argument = v;
          } else if (type == ARG_SHORT) {
            short* v = new short[actualLength];
            argument = v;
          } else if (type == ARG_INT) {
            int* v = new int[actualLength];
            argument = v;
          } else if (type == ARG_LONG) {
            long* v = new long[actualLength];
            argument = v;
          } else if (type == ARG_DOUBLE) {
            double* v = new double[actualLength];
            argument = v;
          } else if (type == ARG_FLOAT) {
            float* v = new float[actualLength];
            argument = v;
          } else {
            ASSERT(false, "Invalid type");
          }

          param[l] = argument;
          argStr = argStr.substr(delim_position + 1);
          //cout << "args nows=\"" << argStr << "\"" << endl;
          //cout << actualLength << endl;
          for (int a = 0; a < actualLength; a++) {
            string tmpStr = argStr.substr(0, argStr.find(';'));
            argStr = argStr.substr(tmpStr.size() + 1);

            if (type == ARG_CHAR) {
              char* v = (char*) argument;
              if (tmpStr.empty()) {
                v[a] = ';';
                argStr = argStr.substr(1);
              } else {
                v[a] = tmpStr.at(0);
              }
            } else if (type == ARG_SHORT) {
              short* v = (short*)argument;
              v[a] = (short)atoi(tmpStr.c_str());
            } else if (type == ARG_INT) {
              int* v = (int*)argument;
              v[a] = (int)atoi(tmpStr.c_str());
            } else if (type == ARG_LONG) {
              long* v = (long*)argument;
              v[a] = (long)atol(tmpStr.c_str());
            } else if (type == ARG_DOUBLE) {
              double* v = (double*)argument;
              v[a] = (double)atof(tmpStr.c_str());
            } else if (type == ARG_FLOAT) {
              float* v = (float*)argument;
              v[a] = (float)atof(tmpStr.c_str());
            } else {
              ASSERT(false, "Invalid type");
            }
          }
        }

        int rc = it->second.second(args, param);
        string ret = serializeCall(it->second.first, rpcArgsMapping[normalized], param);
        //cout << "returing" << ret << endl;
        sendString(socketid, ret); // reply to client
        if (arrayLenWarning) sendString(socketid, "W"); // reply to client
        else sendString(socketid, ""); // reply to client
        sendString(socketid, ret); // reply to client
        for (int l = 0; l < len; l++) {
          int type = (args[l] >> 16) & 0xFF;
          if (type == ARG_CHAR) {
            char* v = (char*)param[l];
            delete [] v;
          } else if (type == ARG_SHORT) {
            short* v = (short*)param[l];
            delete [] v;
          } else if (type == ARG_INT) {
            int* v = (int*)param[l];
            delete [] v;
          } else if (type == ARG_LONG) {
            long* v = (long*)param[l];
            delete [] v;
          } else if (type == ARG_DOUBLE) {
            double* v = (double*)param[l];
            delete [] v;
          } else if (type == ARG_FLOAT) {
            float* v = (float*)param[l];
            delete [] v;
          }
        }
        free(param);
      }
    }
  }

  map<string, pair<string, skeleton> > rpcHandlerMapping;
  map<string, int*> rpcArgsMapping;
};

// =====================================================================
// Global variables because it has to be.
BinderClient* binderClient = NULL;
RPCServer* rpcServer = NULL;
// =====================================================================

int initBinderClient() {
  if (!binderClient) {
    // Set up connection to binder.
    HostPort* hp = getHostPort(HostPort::BINDER, true, true);
    if (!hp) return Error::MISSING_ENV_VAR;
    binderClient = new BinderClient(*hp);
    delete hp;
  }
  return 0;
}

int rpcInit() {
  try {
    int rc = initBinderClient();
    if (rc < 0) return rc;

    // Set up ports to accept (but have not started accepting yet);
    rpcServer = new RPCServer(binderClient->transport.m_sockfd);
    return 0;
  } catch (int i) {
    return i;
  }
}

// =====================================================================
int rpcRegister(char* name, int* argTypes, skeleton f) {
  if (!binderClient) return Error::UNINITIALIZED_BINDER_CLIENT;
  if (!rpcServer) return Error::UNINITIALIZED_SERVER;
  if (!argTypes) return Error::INVALID_ARGTYPES;
  if (!f) return Error::INVALID_SKELETON;

  // Register at local server handler.
  string args = normalizeArgs(string(name), argTypes);
  if (rpcServer->rpcHandlerMapping.find(args) !=
      rpcServer->rpcHandlerMapping.end()) {
    // Overwrite the handler
    rpcServer->rpcHandlerMapping[args] = make_pair(string(name), f);
    return Warn::REREGISTER_SAME_INTERFACE;
  }

  rpcServer->rpcHandlerMapping[args] = make_pair(string(name), f);

  int len = 0;
  while (argTypes[len] != 0) {
    len++;
  }
  int* argTypesCopy = new int[len + 1];
  memcpy(argTypesCopy, argTypes, sizeof(int) * len + 1);
  rpcServer->rpcArgsMapping[args] = argTypesCopy;

  // Notify binder that the server is ready.
  return binderClient->registerServer(
      string(name), argTypes, rpcServer->hostport.toString());
}

// =====================================================================
int rpcCallHelper(char* name, int* argTypes, void** args, HostPort* hp) {
  if (!name) return Error::INVALID_NAME;
  if (!argTypes) return Error::INVALID_ARGTYPES;
  int rc = initBinderClient();
  if (rc < 0) return rc;

  string hostname;
  int port;

  if (!hp) {
    HostPort hpServer;
    rc = binderClient->locateServer(string(name), argTypes, &hpServer);
    if (rc < 0) return rc;
    //cout << "located " << hpServer.toString() << endl;
    hostname = hpServer.hostname;
    port = hpServer.port;
  } else {
    hostname = hp->hostname;
    port = hp->port;
  }

  Transporter transServer(hostname, port);
  transServer.connect();

  string request = serializeCall(string(name), argTypes, args);
  //cout << "Send: " << request << " " << request.size() << endl;

  // request should have everything now
  sendString(transServer.m_sockfd, request);

  //take the reply and shove the parameters back to the pointers
  string reply;
  rc = recvString(transServer.m_sockfd, reply);
  if (rc < 0) return Error::SERVER_UNREACHEABLE;
  if (reply == UNSUPPORTED_CALL) return Error::SERVER_DOES_NOT_SUPPORT_CALL;
  //cout << "Recv: " << reply << endl;

  int len = 0; while (argTypes[len] != 0) len++;
  string argStr = reply.substr(reply.find('#') + 1);
  for (int l = 0; l < len; l++) {
    int type = (argTypes[l] >> 16) & 0xFF;
    int length = (argTypes[l]) & 0xFF;
    bool isOutput = (argTypes[l] & (1 << ARG_OUTPUT));
    ASSERT(not argStr.empty(), "need more stirng to process");

    size_t delim_position = argStr.find(':');
    int actualLength = atoi(argStr.substr(0, delim_position).c_str());

    argStr = argStr.substr(delim_position + 1);

    for (int a = 0; a < actualLength; a++) {
      //cout << "args nows=\"" << argStr << "\"" << endl;
      string tmpStr = argStr.substr(0, argStr.find(';'));
      argStr = argStr.substr(tmpStr.size() + 1);
      // If not output skip,
      // If serverside length is longer than user provided, skip extra
      if (!isOutput || a >= length) continue;

      if (type == ARG_CHAR) {
        char* v = (char*) args[l];
        if (tmpStr.empty()) {
          v[a] = ';';
          argStr = argStr.substr(1);
        } else {
          v[a] = tmpStr.at(0);
        }

      } else if (type == ARG_SHORT) {
        short* v = (short*) args[l];
        v[a] = (short)atoi(tmpStr.c_str());
      } else if (type == ARG_INT) {
        int* v = (int*) args[l];
        v[a] = (int)atoi(tmpStr.c_str());
      } else if (type == ARG_LONG) {
        long* v = (long*) args[l];
        v[a] = (long)atol(tmpStr.c_str());
      } else if (type == ARG_DOUBLE) {
        double* v = (double*) args[l];
        v[a] = (double)atof(tmpStr.c_str());
      } else if (type == ARG_FLOAT) {
        float* v = (float*) args[l];
        v[a] = (float)atof(tmpStr.c_str());
      } else {
        ASSERT(false, "Invalid type");
      }
    }
  }

  rc = recvString(transServer.m_sockfd, reply);
  if (rc < 0) return Error::SERVER_UNREACHEABLE;

  if (reply.size()) return Warn::EXCEED_MAX_ARRAY_LEN;

  return 0;

}
int rpcCall(char* name, int* argTypes, void** args) {
  return rpcCallHelper(name, argTypes, args, NULL);
}

// =====================================================================
map<string, list<HostPort> > cache;
int rpcCacheCall(char* name, int* argTypes, void** args) {
  int rc = initBinderClient();
  if (rc < 0) {
    return Error::BINDER_UNREACHEABLE;
  }
  string norm = normalizeArgs(name, argTypes);
  map<string, list<HostPort> >::iterator api = cache.find(norm);

  if (api == cache.end()) {
    rc = binderClient->cacheLocation(cache);
    if (rc < 0){
      cache.clear();
      return Error::BINDER_UNREACHEABLE;
    }
    api = cache.find(norm);
  }

  if (api == cache.end()) {
    return Error::NO_SERVER_WITH_ARGTYPE;
  }

  bool success = false;
  for (list<HostPort>::iterator hp = api->second.begin();
      hp != api->second.end();
      hp++) {
    HostPort& tmp = *hp;
    rc = rpcCallHelper(name, argTypes, args, &tmp);
    if (rc >= 0) {
      success = true;
      break;
    }
  }

  if (success) {
    return rc;
  } else {
    // Update current location and try again.
    rc = binderClient->cacheLocation(cache);
    if (rc < 0){
      cache.clear();
      return Error::BINDER_UNREACHEABLE;
    }
    api = cache.find(norm);
  }

  if (api == cache.end()) {
    return Error::NO_SERVER_WITH_ARGTYPE;
  }

  for (list<HostPort>::iterator hp = api->second.begin();
      hp != api->second.end();
      hp++) {
    HostPort& tmp = *hp;
    rc = rpcCallHelper(name, argTypes, args, &tmp);
    if (rc >= 0) {
      break;
    }
  }

  return rc;
}

// =====================================================================
int rpcExecute() {
  if (!binderClient) return Error::UNINITIALIZED_BINDER_CLIENT;
  if (!rpcServer) return Error::UNINITIALIZED_SERVER;
  if (rpcServer->rpcArgsMapping.size() == 0) return Error::NO_FUNCTION_TO_SERVE;
  return rpcServer->execute();
}

// =====================================================================
int rpcTerminate() {
  int rc = initBinderClient();
  if (rc < 0) return rc;

  rc = binderClient->terminateAll();
  delete binderClient;
  binderClient = NULL;

  return rc;
}
