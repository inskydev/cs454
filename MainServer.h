#ifndef MAINSERVER_H
#define MAINSERVER_H


struct MainServer : public Server {

  MainServer() : Server(HostPort::SERVER) {
  }

  virtual ~MainServer() {}

  virtual void connected(int socketid);

  virtual void disconnected(int socketid);

  // Server register or client locate requests.
  virtual void handleRequest(int socketid, const string& msg);

  // Who am I?
  HostPort hostport; 
  
};

#endif // MAINSERVER_H
