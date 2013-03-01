#ifndef SERVER_H_
#define SERVER_H_

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include "util.h"

struct Server {
  Server(bool put_file = false) {
    int rc;
    listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT(listenSocket >= 0, "ERROR opening socket");

    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(0);
    rc = bind(listenSocket, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
    ASSERT(rc >= 0, "ERROR on binding");
    listen(listenSocket, 5);

    // Get a randomly assigned port number
    socklen_t addrlen = sizeof(serv_addr);
    getsockname(listenSocket, (struct sockaddr*)&serv_addr, &addrlen);
    // Get complete fully qualified name.
    string hostname = exec("hostname -A");
    // Publish hostport value to internet. (so we don't have to set env var)
    putHostPort(HostPort::SERVER, hostname,
        std::to_string((long long int)ntohs(serv_addr.sin_port)),
        put_file);
    hostport.hostname = hostname;
    hostport.port = int(ntohs(serv_addr.sin_port));
  }

  virtual ~Server() {
    close(listenSocket);
  }

  // Infinite loop on selecting from sockets.
  void execute();

  virtual void connected(int socketid) = 0;
  virtual void disconnected(int socketid) = 0;
  virtual void handleRequest(int socketid, const string& msg) = 0;

  // Members
  int listenSocket;
  list<int> clientSockets;
  struct sockaddr_in serv_addr, cli_addr;
  HostPort hostport;
};

#endif // SERVER_H_
