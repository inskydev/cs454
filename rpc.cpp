#include "rpc.h"
#include "util.h"
#include "Binder.h"
#include "Server.h"

#define UNSUPPORTED_CALL "UNSUPPORTED_CALL"
#define EXCEED_ARRAY_LEN "EXCEED_ARRAY_LEN"

struct RPCServer : public Server {
  // RPC server handles 10 concurrent requests.
  RPCServer() : Server(HostPort::SERVER, 10) {
  }

  virtual ~RPCServer() {}

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
      map<string, skeleton>::iterator it =
        rpcHandlerMapping.find(normalized);
      if (it == rpcHandlerMapping.end()) {
        cout << "No API supported here" << endl;
        sendString(socketid, UNSUPPORTED_CALL);
      } else {
        cout << "Supported" << endl;
        // Iterate through each argument check array length.
        const list<int>& args = rpcArgsMapping[normalized];
        string argsStr = msg.substr(msg.find('#') + 1);
        int aa;
        void** param = NULL;
        if (!argsStr.empty()) {
          cout << "not empty" << endl;
          it->second(&aa, param);
          cout << argsStr << endl;
          // Read the first args.
          string num = argsStr.substr(0, argsStr.find(':'));
          cout << num << endl;
        }
        cout << "calling" << endl;
        int rc = it->second(&aa, param);
        cout << "ret:" << rc << endl;
        // TODO, loop through output params and send them back.
      }
      // check with rpcHandlerMapping again
      // call skeleton with values.
      sendString(socketid, ""); // reply to client
    }
  }

  map<string, skeleton> rpcHandlerMapping;
  map<string, list<int>> rpcArgsMapping;
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
    rpcServer = new RPCServer();
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
  rpcServer->rpcHandlerMapping[args] = f;
  list<int> argsList;
  while (*argTypes != 0) {
    argsList.push_back(*argTypes);
    argTypes++;
  }
  rpcServer->rpcArgsMapping[args] = argsList;

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

  // marshall inputs
  string request = "C" + normalizeArgs(name, argTypes);
  request += "#";

  void** it = args;
  int* at = argTypes;
  for (;(*at);it++,at++) {
    // need to know how to increment the ptrs of different size

    char* curr = (char*)*it;
    int type = ((*at)>>16) & 0xFF;
    int length = ((*at)) & 0xFF;
    if (!length) {
      length = 1;
    }
    request += to_string((long long int)length);
    request += ":";

    for (int i =0; i<length; i++){
      char buffer[16];
      memset(buffer, 0, 16);

      switch (type) {
        case ARG_CHAR:
          memcpy(buffer, curr, sizeof(char));
          curr += sizeof(char);
          break;
        case ARG_SHORT:
          memcpy(buffer, curr, sizeof(short));
          curr += sizeof(short);
          break;
        case ARG_INT:
          memcpy(buffer, curr, sizeof(int));
          curr += sizeof(int);
          break;
        case ARG_LONG:
          memcpy(buffer, curr, sizeof(long));
          curr += sizeof(long);
          break;
        case ARG_DOUBLE:
          memcpy(buffer, curr, sizeof(double));
          curr += sizeof(double);
          break;
        case ARG_FLOAT:
          memcpy(buffer, curr, sizeof(float));
          curr += sizeof(float);
          break;
        default:
          printf("error type\n");
      }
      request += string(buffer);
    }
  }
  Transporter transServer(hpServer.hostname, hpServer.port);
  transServer.connect();

  // request should have everything now
  sendString(transServer.m_sockfd, request);


  //take the reply and shove the parameters back to the pointers
  string processedString;
  rc = recvString(transServer.m_sockfd, processedString);
  if (rc < 0) return rc;

  char* cpString = new char[processedString.length()+1];

  processedString.c_str();

  strcpy(cpString, processedString.c_str());

  while (*cpString != '#')
    cpString ++;

  it=args;
  at = argTypes;
  for (;(*at);it++,at++){
    // need to know how to increment the ptrs of different size

    char* curr = (char*)*it;

    int type = ((*at)>>16) & 0xFF;
    int length = ((*at)) & 0xFF;

    if (!length)
      length = 1;

    for (int i =0; i<length; i++){

      if (!((*at) && 0x80000000)) //check if this is output
        break;

      switch(((*at)>>16) & 0xFF ){

        case ARG_CHAR:
          memcpy(curr, cpString, sizeof(char));
          cpString += sizeof(char);
          break;
        case ARG_SHORT:
          memcpy(curr, cpString, sizeof(short));
          cpString += sizeof(short);
          break;
        case ARG_INT:
          memcpy(curr, cpString, sizeof(int));
          cpString += sizeof(int);
          break;
        case ARG_LONG:
          memcpy(curr, cpString, sizeof(long));
          cpString += sizeof(long);
          break;
        case ARG_DOUBLE:
          memcpy(curr, cpString, sizeof(double));
          cpString += sizeof(double);
          break;
        case ARG_FLOAT:
          memcpy(curr, cpString, sizeof(float));
          cpString += sizeof(float);
          break;
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
