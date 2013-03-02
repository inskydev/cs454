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
    // TODO, handle client requests
  }

  map<string, skeleton> rpcHandlerMapping;
};

// =====================================================================
// Global variables because it has to be.
BinderClient* binderClient = NULL;
RPCServer* rpcServer = NULL;
// =====================================================================

int rpcInit() {
  // Set up connection to binder.
  HostPort* hp = getHostPort(HostPort::BINDER, true, true);
  if (!hp) return Error::NO_BINDER_ADDRESS;
  binderClient = new BinderClient(*hp);
  delete hp;

  // Set up ports to accept (but have not started accepting yet);
  rpcServer = new RPCServer();

  return 0;
}

// =====================================================================
int rpcRegister(char* name, int* argTypes, skeleton f) {
  // TODO, check arguments are not null
  if (!binderClient) return Error::UNINITIALIZED_BINDER;
  if (!rpcServer) return Error::UNINITIALIZED_SERVER;

  // Register at local server handler.
  string args = normalizeArgs(string(name), argTypes);
  rpcServer->rpcHandlerMapping[args] = f;

  // Notify binder that the server is ready.
  return binderClient->registerServer(
      string(name), argTypes, rpcServer->hostport.toString());
}

// =====================================================================
int rpcCall(char* name, int* argTypes, void** args) {
	
  binderClient->locateServer(string(name), argTypes);
  //get reply from binder
  string serverMsg = recvString(binderClient->transport.m_sockfd);
  
  HostPort *hpServer;
  hpServer->fromString(serverMsg);
  
  //marshall inputs
  string request = normalizeArgs(name, argTypes);
  request+="#";
  
  void** it=args;
  int* at = argTypes;
  for (;it;it++,at++){
    //need to know how to incremeant the ptrs of different size
    
    char* curr = (char*)*it;
    
    while (curr){
    
      char* buffer = (char*)malloc(sizeof(double));
      memset(buffer, 0, sizeof(double));
      
      switch(*at){
        
        case ARG_CHAR:
          memcpy(curr, buffer, sizeof(char));
          curr += sizeof(char);
          break;
        case ARG_SHORT:
          memcpy(curr, buffer, sizeof(short));
          curr += sizeof(short);
          break;
        case ARG_INT:
          memcpy(curr, buffer, sizeof(int));
          curr += sizeof(int);
          break;
        case ARG_LONG:
          memcpy(curr, buffer, sizeof(long));
          curr += sizeof(long);
          break;
        case ARG_DOUBLE:
          memcpy(curr, buffer, sizeof(double));
          curr += sizeof(double);
          break;
        case ARG_FLOAT:
          memcpy(curr, buffer, sizeof(float));
          curr += sizeof(float);
          break;
      }
      request+=(string)buffer;
      
      delete buffer;
    }
  }
  Transporter transServer(hpServer->hostname, hpServer->port);
  transServer.connect();
  
  //request should have everything now
  return sendString(transServer.m_sockfd, request);
    

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
