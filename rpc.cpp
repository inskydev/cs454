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
      std::cout << "Server terminating" << endl;
    } else if (msg[0] == 'C') {
      std::cout << "Got request:" << msg << endl;
      // TODO, array length too long.
      // deserialize ..
      string normalized = msg.substr(1, msg.find('#') - 1);
      cout << normalized << endl;
      map<string, pair<string, skeleton> >::iterator it =
        rpcHandlerMapping.find(normalized);
      if (it == rpcHandlerMapping.end()) {
        cout << "No API supported here" << endl;
        sendString(socketid, UNSUPPORTED_CALL);
      } else {
        cout << "Supported" << endl;
        // Iterate through each argument check array length.
        int* args = rpcArgsMapping[normalized];
        int len = 0; while (args[len] != 0) len++;
        string argStr = msg.substr(msg.find('#') + 1);
        cout << "Arg str:" << argStr <<  " " << argStr.size() << endl;
        bool arrayLenWarning = false;

        void** param = (void**) malloc(sizeof(void*) * len);
        // For every argument.
        for (int l = 0; l < len; l++) {
          int type = (args[l] >> 16) & 0xFF;
          int length = (args[l]) & 0xFF;
          bool isInput = (args[l] & (1 << ARG_INPUT));
          ASSERT(not argStr.empty(), "need more stirng to process");

          size_t delim_position = argStr.find(':');
          int actualLength = atoi(argStr.substr(0, delim_position).c_str());
          if (actualLength > length) arrayLenWarning = true;

          void* argument = NULL;
          if (type == ARG_CHAR) {
            cout << "Char array:" << endl;
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
          cout << "args nows=\"" << argStr << "\"" << endl;
          for (int a = 0; a < actualLength; a++) {
            string tmpStr = argStr.substr(0, argStr.find(';'));
            argStr = argStr.substr(tmpStr.size() + 1);

            if (type == ARG_CHAR) {
              char* v = (char*) argument;
              v[a] = tmpStr.at(0);
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
        //cout << "calling" << endl;
        int rc = it->second.second(args, param);
        //free(param);
        // TODO deletes,
        // TODO, loop through output params and send them back.
        string ret = serializeCall(it->second.first, rpcArgsMapping[normalized], param);
        cout << "returing" << ret << endl;
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
    if (!hp) return Error::NO_BINDER_ADDRESS;
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
  // TODO, check arguments are not null
  if (!binderClient) return Error::UNINITIALIZED_BINDER;
  if (!rpcServer) return Error::UNINITIALIZED_SERVER;
  if (!argTypes) return Error::INVALID_ARGTYPES;
  //if (!f) return Error::INVALID_SKELETON;

  // Register at local server handler.
  string args = normalizeArgs(string(name), argTypes);
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
int rpcCall(char* name, int* argTypes, void** args) {
  if (!name) return Error::INVALID_NAME;
  if (!argTypes) return Error::INVALID_ARGTYPES;
  int rc = initBinderClient();
  if (rc < 0) return rc;

  cout << "rpc call" << endl;

  HostPort hpServer;
  rc = binderClient->locateServer(string(name), argTypes, &hpServer);
  if (rc < 0) return rc;
  cout << "located " << hpServer.toString() << endl;

  Transporter transServer(hpServer.hostname, hpServer.port);
  transServer.connect();

  string request = serializeCall(string(name), argTypes, args);
  cout << "Send: " << request << " " << request.size() << endl;

  // request should have everything now
  sendString(transServer.m_sockfd, request);

  //take the reply and shove the parameters back to the pointers
  string reply;
  rc = recvString(transServer.m_sockfd, reply);
  if (rc < 0) return rc;
  cout << "Recv: " << reply << endl;

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
      cout << "args nows=\"" << argStr << "\"" << endl;
      string tmpStr = argStr.substr(0, argStr.find(';'));
      argStr = argStr.substr(tmpStr.size() + 1);
      if (!isOutput) continue;

      if (type == ARG_CHAR) {
        char* v = (char*) args[l];
        v[a] = tmpStr.at(0);
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

  return 0;
}

// =====================================================================
int rpcCacheCall(char* name, int* argTypes, void** args){
}

// =====================================================================
int rpcExecute() {
  if (!binderClient) return Error::UNINITIALIZED_BINDER;
  if (!rpcServer) return Error::UNINITIALIZED_SERVER;
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
