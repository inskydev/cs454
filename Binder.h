#ifndef BINDER_H
#define BINDER_H

#include "util.h"
#include "Transporter.h"

struct BinderClient {
  BinderClient(HostPort hp)
    : hostport(hp), transport(hp.hostname, hp.port) {
  }

  int registerServer(const string& name, int* argTypes, const string& server);

  void locateServer();

  HostPort hostport; // Binder hostport
  Transporter transport;
};

struct Binder {

  void handleRequest(const string& msg, int clientSocket);

  // member vars for mapping
  map<string, list<HostPort> > mapping;
  map<int, HostPort> socketHostPortMap;
};

#endif // BINDER_H
