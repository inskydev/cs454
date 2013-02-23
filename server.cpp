#include <algorithm>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "util.cpp"

using namespace std;

void error(const char *msg) {
      perror(msg);
          exit(1);
}

string toTitleCase(string data) {
  // Everything lower first.
  std::transform(data.begin(), data.end(), data.begin(), ::tolower);

  // Capitalize first of everyword.
  bool beginWord = true;
  for (size_t i = 0; i < data.size(); i++) {
    if (data[i] == ' ' || data[i] == '\t') {
      beginWord = true;
    }
    if (beginWord && data[i] != ' ') {
      data[i] = (char)toupper(data[i]);
      beginWord = false;
    }
  }
  return data;
}

int main() {
  int sockfd, newsockfd;
  socklen_t clilen;
  char buffer[256];
  struct sockaddr_in serv_addr, cli_addr;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    error("ERROR opening socket");
  }

  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(0);
  if (bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
    error("ERROR on binding");
  }
  listen(sockfd, 5);

  socklen_t addrlen = sizeof(serv_addr);
  getsockname(sockfd, (struct sockaddr*)&serv_addr, &addrlen);
  string hostname = exec("hostname -A");
  putHostPort(HostPort::SERVER, hostname, to_string((long long int)ntohs(serv_addr.sin_port)));

  clilen = sizeof(cli_addr);
  newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);
  if (newsockfd < 0) {
    error("ERROR on accept");
  }

  string msg;
  size_t bytes_read;
  do {
      bzero(buffer, 256);
      bytes_read = read(newsockfd, buffer, 255);
      msg += string(buffer, 255);
  } while (bytes_read > 0);
  if (bytes_read < 0) error("ERROR reading from socket");
  cout << msg << endl;

  int n = write(newsockfd, "I got your message", 18);
  if (n < 0) error("ERROR writing to socket");
  close(newsockfd);
  close(sockfd);

  return 0;
}
