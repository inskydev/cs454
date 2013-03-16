#ifndef SERVER_H_
#define SERVER_H_

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include "util.h"
#include <pthread.h>

struct Server {
  static void* thread_run(void* server);

  Server(HostPort::Type type, int binderSock, int numThread = 1) {
    binderSocket = binderSock;
    binderTerminatingClient = -1;
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
    putHostPort(type, hostname,
        std::to_string((long long int)ntohs(serv_addr.sin_port)));
    hostport.hostname = hostname;
    hostport.port = int(ntohs(serv_addr.sin_port));

    for (int i = 0; i < numThread; i++) {
      workers.push_back(pthread_t());
      pthread_t* worker = &workers.back();
      pthread_create(worker, NULL, thread_run, this);
    }
  }

  virtual ~Server() {
    close(listenSocket);
  }

  // Loop on selecting from sockets until termination signal.
  int execute();

  virtual void connected(int socketid) = 0;
  virtual void disconnected(int socketid) = 0;
  virtual void handleRequest(int socketid, const string& msg) = 0;
  virtual bool canTerminateNow() {
    return false;
  }

  // Members
  int listenSocket;
  int binderSocket;
  int binderTerminatingClient;
  list<int> clientSockets;
  struct sockaddr_in serv_addr, cli_addr;
  HostPort hostport;
  Counter terminate;
  Counter terminating;

  // Pairs of client id and message.
  Channel<pair<int, string>> workItems;
  list<pthread_t> workers;
};

#endif // SERVER_H_
