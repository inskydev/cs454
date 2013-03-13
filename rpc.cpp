#include "rpc.h"
#include "util.h"
#include "Binder.h"
#include "Server.h"

struct RPCServer : public Server {
  RPCServer() : Server(HostPort::SERVER) {
  }

  virtual ~RPCServer() {}

  virtual void connected(int socketid)  {}
  virtual void disconnected(int socketid) {}
  virtual void handleRequest(int socketid, const string& msg) {
    if (msg[0] == 'T') {
      ++terminate;
    }
    std::cout << "Got request:" << msg << endl;
    // TODO, handle client requests
  }

  map<string, skeleton> rpcHandlerMapping;
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
  string request = normalizeArgs(name, argTypes);
  request+="#";

  void** it = args;
  int* at = argTypes;
  for (;(*at);it++,at++){
    //need to know how to increment the ptrs of different size

    char* curr = (char*)*it;
    int type = ((*at)>>16) & 0xFF;
    int length = ((*at)) & 0xFF;
    if (!length)
      length = 1;

    for (int i =0; i<length; i++){
      char* buffer = (char*)malloc(sizeof(double));
      memset(buffer, 0, sizeof(double));

      switch(type){

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

      delete buffer;
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
  if (!binderClient) return Error::UNINITIALIZED_BINDER;
  return binderClient->terminateAll();
}
