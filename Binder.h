#ifndef BINDER_H
#define BINDER_H

#include <map>
#include "util.h"

struct BinderClient {
  Binder(HostPort hp) : hostport(hp) {
    // TODO control transport
  }

  int registerServer(char* name, int* argTypes);

  void locateServer();

  HostPort hostport;
  Transport transport;
};


struct Binder {

  void handleRequest(const string& msg);

  // member vars for mapping
  map<string, list<HostPort> > mapping;
};

#endif // BINDER_H
