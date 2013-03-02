#ifndef BINDER_H
#define BINDER_H

#include "util.h"
#include "Transporter.h"
#include "Server.h"

struct BinderClient {
  BinderClient(const HostPort& hp)
    : hostport(hp), transport(hp.hostname, hp.port) {
    transport.connect();
  }

  int registerServer(const string& name, int* argTypes, const string& server);

  void locateServer();

  HostPort hostport; // Binder hostport
  Transporter transport;
};

struct Binder : public Server {

  Binder() : Server(HostPort::BINDER) {
  }

  virtual ~Binder() {}

  virtual void connected(int socketid)  {}
  virtual void disconnected(int socketid) {}
  virtual void handleRequest(int socketid, const string& msg);

  // member vars for mapping
  map<string, list<HostPort> > mapping;
  map<int, HostPort> socketHostPortMap;
};

#endif // BINDER_H