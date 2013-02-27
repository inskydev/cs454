#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <list>

#include "util.cpp"
#include "Transporter.cpp"

using namespace std;

Counter numOutstandingMsg;
Counter done_reading;
list<string> bufferedMsgs;
PMutex bufferedMsgs_lock;

void* print_server_msg(void* t) {
  Transporter* transporter = (Transporter*) t;

  // Rely on main exiting to terminate.
  while (done_reading.get() == 0 || numOutstandingMsg.get() > 0) {
    if (numOutstandingMsg.get() > 0) {
      string msg = recvString(transporter->m_sockfd); // Blocking receive.
      if (msg.size()) {
        cout << "Server: " << msg << endl;
        --numOutstandingMsg;
      }
    }
    sleep(1);
  }
}

void* query_server_msg(void* t) {
  Transporter* transporter = (Transporter*) t;
  while (done_reading.get() == 0 || numOutstandingMsg.get() > 0) {
    string msg;
    bufferedMsgs_lock.lock();
    if (bufferedMsgs.size()) {
      msg = bufferedMsgs.front();
      bufferedMsgs.pop_front();
    }
    bufferedMsgs_lock.unlock();

    if (msg.size()) {
      transporter->query(msg);
    }
    // Polls every 2 seconds.
    sleep(2);
  }
}

int main(int argc, char *argv[]) {
  bool is_debug = argc > 1;
  bool use_published_hostport = argc > 2;
  // Random 2nd and 3rd arguments forces
  HostPort* hp = getHostPort(HostPort::SERVER, is_debug, use_published_hostport);
  ASSERT(hp, "Error, no server hostport found.");
  Transporter transport(hp->hostname, hp->port);
  transport.connect();

  pthread_t querier, receiver;
  pthread_create(&receiver, NULL, print_server_msg, &transport);
  pthread_create(&querier, NULL, query_server_msg, &transport);

  while (1) {
    string msg;
    getline(cin, msg);
    if (msg.size()) {
      // enqueue work.
      bufferedMsgs_lock.lock();
      bufferedMsgs.push_back(msg);
      ++numOutstandingMsg;
      bufferedMsgs_lock.unlock();
    } else {
      break; // No more input
    }
  }
  ++done_reading; // Set flag saying read is complete.

  while (numOutstandingMsg.get() != 0) {
    sleep(1);
  }

  pthread_exit(NULL);
  return 0;
}
