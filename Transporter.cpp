#include "Transporter.h"

void Transporter::connect() {
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
