#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

using namespace std;

struct Transporter {
  Transporter(string hostname, int port)
    : m_hostname(hostname),
      m_port(port),
      m_sockfd(-1)
  {
  }

  ~Transporter() {
    close(m_sockfd);
    m_sockfd = -1;
  }

  void connect() {
    int rc;
    m_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT(m_sockfd >= 0, "Error opening socket.");

    m_host = gethostbyname(m_hostname.c_str());
    ASSERT(m_host != NULL, "Error, no such host.");

    bzero((char *) &m_host_addr, sizeof(m_host_addr));
    m_host_addr.sin_family = AF_INET;
    bcopy((char *)m_host->h_addr,
          (char *)&m_host_addr.sin_addr.s_addr,
          m_host->h_length);
    m_host_addr.sin_port = htons(m_port);
    rc = ::connect(m_sockfd, (struct sockaddr*)&m_host_addr, sizeof(m_host_addr));
    ASSERT(rc >= 0, "Error connecting");
  }

  // Client only.
  string query(string msg) {
    ASSERT(m_sockfd != -1, "Trying to query without connecting");
    int rc = sendString(m_sockfd, msg);
    ASSERT(rc, "ERROR writing to socket");

    return recvString(m_sockfd);
  }

  private:
  string m_hostname;
  int m_port;

  int m_sockfd;
  struct sockaddr_in m_host_addr;
  struct hostent* m_host;

};
