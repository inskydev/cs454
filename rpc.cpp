#include "rpc.h"
#include "util.h"
#include "Binder.h"
#include "Server.h"

struct RPCServer : public Server {
  RPCServer(bool put_file = false) : Server(put_file) {
  }

  virtual ~RPCServer() {}

  virtual void connected(int socketid)  {}
  virtual void disconnected(int socketid) {}
  virtual void handleRequest(int socketid, const string& msg) {}
};

// Global variables because it has to be.
BinderClient* binderClient = NULL;
RPCServer* rpcServer = NULL;

int rpcInit() {
  // Set up connection to binder.
  HostPort* hp = getHostPort(HostPort::BINDER, true, true);
  binderClient = new BinderClient(*hp);
  delete hp;

  // Set up ports to accept client requests.
  rpcServer = new RPCServer();

  // create transport for listening to incoming client connection.

  return 0;
}

int rpcRegister(char* name, int* argTypes, skeleton f) {
  if (!binderClient) return Error::UNINITIALIZED_BINDER;
  if (!rpcServer) return Error::UNINITIALIZED_SERVER;

  // TODO Register at local server handler.

  // Notify binder that the server is ready.
  return binderClient->registerServer(
      string(name), argTypes, rpcServer->hostport.toString());
}

int rpcCall(char* name, int* argTypes, void** args) {

}

int rpcCacheCall(char* name, int* argTypes, void** args){
}

int rpcExecute() {
}

int rpcTerminate() {
}


