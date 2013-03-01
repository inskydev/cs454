#include "rpc.h"
#include "util.h"

// Global variables because it has to be.
BinderClient* binderClient = NULL;

int rpcInit() {
  HostPort* hp = getHostPort(HostPort::BINDER, true, true);
  binderClient = new BinderClient(*hp);
  delete hp;

  // create transport for listening to incoming client connection.

  return 0;
}

int rpcRegister(char* name, int* argTypes, skeleton f) {
  if (!binderClient) return Error::UNINITIALIZED_BINDER;

  // TODO Register at local server handler.

  // Notify binder that the server is ready.
  return binderClient->registerServer(name, argTypes);
}

int rpcCall(char* name, int* argTypes, void** args) {

}

int rpcCacheCall(char* name, int* argTypes, void** args){
  if (!binderClient) return -1;
}

int rpcExecute() {
  if (!binderClient) return -1;
}
int rpcTerminate() {
  if (!binderClient) return -1;
}


