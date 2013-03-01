#include "rpc.h"
#include "util.h"
#include "Binder.h"

// Global variables because it has to be.
BinderClient* binderClient = NULL;
HostPort* serverHostPort = NULL;

int rpcInit() {
  HostPort* hp = getHostPort(HostPort::BINDER, true, true);
  binderClient = new BinderClient(*hp);
  delete hp;

  // create transport for listening to incoming client connection.

  return 0;
}

int rpcRegister(char* name, int* argTypes, skeleton f) {
  if (!binderClient) return Error::UNINITIALIZED_BINDER;
  if (!serverHostPort) return Error::UNINITIALIZED_SERVER;

  // TODO Register at local server handler.

  // Notify binder that the server is ready.
  return binderClient->registerServer(
      string(name), argTypes, serverHostPort->toString());
}

int rpcCall(char* name, int* argTypes, void** args) {

}

int rpcCacheCall(char* name, int* argTypes, void** args){
}

int rpcExecute() {
}

int rpcTerminate() {
}


