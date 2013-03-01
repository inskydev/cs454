#include <algorithm>
#include <list>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

#include "util.h"
#include "Binder.h"

int main(int argc, char* argv[]) {
  bool debug = argc > 1;
  bool put_file = argc > 2;

  Binder binder;

  int listenSocket;
  list<int> clientSockets;
  struct sockaddr_in serv_addr, cli_addr;
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
  putHostPort(HostPort::BINDER, hostname,
      std::to_string((long long int)ntohs(serv_addr.sin_port)),
      put_file);
  fd_set sockfds;
  FD_ZERO(&sockfds);
  FD_SET(listenSocket, &sockfds);

  // One from accept client socket, number of active client socket,
  // Last 1 for documentation.
  int highest_fds = listenSocket;

  while (select(highest_fds + 1, &sockfds, NULL, NULL, NULL) >= 0) {
    if (FD_ISSET(listenSocket, &sockfds)) {
      // New client is knocking.
      socklen_t clilen = sizeof(cli_addr);
      int client = accept(listenSocket, (struct sockaddr*)&cli_addr, &clilen);
      ASSERT(client >= 0, "ERROR on accept");
      FD_SET(client, &sockfds);
      clientSockets.push_back(client);
      highest_fds = std::max(highest_fds, client);
      continue;
    }

    // Client wants to interact.
    bool handled_one = false;
    for (list<int>::iterator client = clientSockets.begin();
        client != clientSockets.end();
        ++client) {

      if (FD_ISSET(*client, &sockfds)) {
        handled_one = true;
        string msg = recvString(*client);
        if (msg.size() == 0) {
          close(*client);
          clientSockets.erase(client); // Client closed socket.
        } else {
          binder.handleRequest(msg, *client);
        }
        break;
      }
    }

    // Reset sockets that needed to be selected on.
    FD_ZERO(&sockfds);
    FD_SET(listenSocket, &sockfds);
    for (list<int>::iterator client = clientSockets.begin();
        client != clientSockets.end();
        ++client) {
      FD_SET(*client, &sockfds);
    }
  } // End select

  close(listenSocket);

  return 0;
}
