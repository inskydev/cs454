#include "Server.h"

void* Server::thread_run(void* s) {
  Server* server = (Server*)s;
  while (server->terminate.get() == 0) {
    pair<int, string> work = server->workItems.get();
    cout << work.second << endl;
    if (work.second == "END") break;
    server->handleRequest(work.first, work.second);
  }

  return NULL;
}

int Server::execute() {
  fd_set sockfds;
  FD_ZERO(&sockfds);
  FD_SET(listenSocket, &sockfds);
  if (binderSocket != -1) FD_SET(binderSocket, &sockfds);

  // One from accept client socket, number of active client socket,
  // Last 1 for documentation.
  int highest_fds = listenSocket;

  while (terminate.get() == 0 &&
      select(highest_fds + 1, &sockfds, NULL, NULL, NULL) >= 0) {
    if (FD_ISSET(listenSocket, &sockfds)) {
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
    if (binderSocket > 0 && FD_ISSET(binderSocket, &sockfds)) {
      string msg;
      int rc = recvString(binderSocket, msg);
      cout << "Binder msg:" << msg << endl;
      if (msg.size() && msg.at(0) == 'T') {
        for (int i = 0; i < workers.size(); i++) {
          workItems.put(make_pair(0, "END"));
        }
        break; // Wait for threads to end.
      } else if (msg.empty()) {
        close(binderSocket);
        binderSocket = -2;
      }
    }

    // Client wants to interact.
    bool handled_one = false;
    bool isTerminating = false;
    for (list<int>::iterator client = clientSockets.begin();
        client != clientSockets.end();
        ++client) {

      if (FD_ISSET(*client, &sockfds)) {
        handled_one = true;
        string msg;
        int rc = recvString(*client, msg);
        if (rc < 0) {
          disconnected(*client);
          close(*client);
          clientSockets.erase(client); // Client closed socket.
        } else {
          binderTerminatingClient = *client;
          workItems.put(make_pair(*client, msg));
          isTerminating = msg.size() && msg.at(0) == 'T';
        }
        break;
      }
    }

    if (isTerminating && canTerminateNow()) break;
    // Reset sockets that needed to be selected on.
    FD_ZERO(&sockfds);
    if (terminating.get() == 0) {
      FD_SET(listenSocket, &sockfds);
      if (binderSocket > 0) FD_SET(binderSocket, &sockfds);
    }

    for (list<int>::iterator client = clientSockets.begin();
        client != clientSockets.end();
        ++client) {
      FD_SET(*client, &sockfds);
    }
  } // End select

  if (binderTerminatingClient > 0) {
    workItems.put(make_pair(0, "END"));
    sendString(binderTerminatingClient, "");
  }
  // Wait for threads to finish.
  for (list<pthread_t>::iterator worker = workers.begin();
      worker != workers.end();
      ++worker) {
    pthread_join(*worker, NULL);
  }

  if (binderSocket > 0) {
    close(binderSocket);
  }

  // close all clients
  for (list<int>::iterator client = clientSockets.begin();
        client != clientSockets.end();
        ++client) {
    close(*client);
  }

  return 0;
}
