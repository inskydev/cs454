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

  int locateServer(const string& name, int* argType, HostPort* hp);

  int cacheLocation(map<string, list<HostPort> >&);

  int terminateAll();

  HostPort hostport; // Binder hostport
  Transporter transport;
};

struct Binder : public Server {

  Binder() : Server(HostPort::BINDER, -1) {
  }

  virtual ~Binder() {}

  virtual void connected(int socketid)  {}

  // Remove all RPCs provided by the server.
  virtual void disconnected(int socketid);

  // Server register or client locate requests.
  virtual void handleRequest(int socketid, const string& msg);

  virtual bool canTerminateNow() {
    return socketHostPortMap.empty();
  }

  // member vars for apiMapping
  map<string, list<HostPort> > apiMapping;
  map<int, HostPort> socketHostPortMap;
};

#endif // BINDER_H
