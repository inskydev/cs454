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
  virtual void handleRequest(int socketid, const string& msg) {}

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

}

// =====================================================================
int rpcCacheCall(char* name, int* argTypes, void** args){
}

// =====================================================================
int rpcExecute() {
}

// =====================================================================
int rpcTerminate() {
}


