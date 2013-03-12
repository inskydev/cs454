#include "Server.h"

void* Server::thread_run(void* s) {
  Server* server = (Server*)s;
  while (server->terminate.get() == 0) {
    pair<int, string> work = server->workItems.get();
    server->handleRequest(work.first, work.second);
  }
}

int Server::execute() {
  fd_set sockfds;
  FD_ZERO(&sockfds);
  FD_SET(listenSocket, &sockfds);

  // One from accept client socket, number of active client socket,
  // Last 1 for documentation.
  int highest_fds = listenSocket;

  while (terminate.get() == 0 &&
      select(highest_fds + 1, &sockfds, NULL, NULL, NULL) >= 0) {
    cout << "selected" << endl;
    if (FD_ISSET(listenSocket, &sockfds)) {
      cout << "new client" << endl;
      // New client is knocking.
      socklen_t clilen = sizeof(cli_addr);
      int client = accept(listenSocket, (struct sockaddr*)&cli_addr, &clilen);
      connected(client);
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
          disconnected(*client);
          close(*client);
          clientSockets.erase(client); // Client closed socket.
        } else {
          workItems.put(make_pair(*client, msg));
        }
        break;
      }
    }

    // Reset sockets that needed to be selected on.
    FD_ZERO(&sockfds);
    if (terminating.get() == 0) {
      FD_SET(listenSocket, &sockfds);
    }
    for (list<int>::iterator client = clientSockets.begin();
        client != clientSockets.end();
        ++client) {
      FD_SET(*client, &sockfds);
    }
  } // End select

  return 0;
}
