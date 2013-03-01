#ifndef TRANSPORTER_H
#define TRANSPORTER_H

#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "util.h"

// Manages client side connections.
struct Transporter {
  Transporter(string hostname, int port)
    : m_hostname(hostname),
      m_port(port),
      m_sockfd(-1)
  {}

  ~Transporter() {
    close(m_sockfd);
    m_sockfd = -1;
  }

  void connect();

  // Client only. Async
  void sendString(string msg) {
    ASSERT(m_sockfd != -1, "Trying to query without connecting");
    int rc = ::sendString(m_sockfd, msg);
    ASSERT(rc >= 0, "ERROR writing to socket");
  }

  string m_hostname;
  int m_port;

  int m_sockfd;
  struct sockaddr_in m_host_addr;
  struct hostent* m_host;
};

#endif // TRANSPORTER_H
