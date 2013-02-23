#include <string>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <stdint.h>


#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define ASSERT(X, ...) { \
    if (!(X)) { \
      printf("Assertion failed in file " __FILE__ " line:" TOSTRING(__LINE__) "\n"); \
      printf("[%s] ", __func__); \
      printf(__VA_ARGS__); \
      printf("\n"); \
      exit(1); \
    } \
}


using namespace std;

struct HostPort {
  enum Type {
    SERVER,
    BINDER,
  };

  HostPort()
    : hostname("UNINIT_HOSTNAME"),
      port(-1) {}

  string hostname;
  int port;
};

std::string exec(string cmd) {
  FILE* pipe = popen(cmd.c_str(), "r");
  if (!pipe) return "ERROR";
  char buffer[128];
  std::string result = "";
  while (!feof(pipe)) {
    if (fgets(buffer, 128, pipe) != NULL) {
      result += string(buffer);
    }
  }
  pclose(pipe);
  // trim newline.
  if (result.size() && result[result.size()-1] == '\n') {
    result = result.substr(0, result.size() - 1);
  }
  return result;
}

void putHostPort(HostPort::Type type, string hostname, string port) {
  cout << "SERVER_ADDRESS " << hostname << endl;
  cout << "SERVER_PORT " << port << endl;
  string user(getenv("USER"));

  string cmd1 = "echo " + hostname +
    " >  /u9/" + user + "/public_html/server_hostport";
  string cmd2 = "echo " + port +
    " >> /u9/" + user + "/public_html/server_hostport";
  system(cmd1.c_str());
  system(cmd2.c_str());
}

HostPort* getHostPort(HostPort::Type type) {
  HostPort* ret = NULL;
  if (type == HostPort::SERVER) {
    char* host = getenv("SERVER_ADDRESS");
    char* port = getenv("SERVER_PORT");
    if (host && port) {
      ret = new HostPort();
      ret->hostname = string(host);
      ret->port = atoi(port);
    } else {
      string user(getenv("USER"));
      system("rm -f server_hostport");
      // Source a web accessible file
      string wget_cmd = "wget http://www.student.cs.uwaterloo.ca/~" + user
        + "/server_hostport -O server_hostport --quiet ";
      cout << wget_cmd << endl;
      system(wget_cmd.c_str());
      ifstream f("server_hostport", ios::in);
      if (f.is_open()) {
        cout << "opened." << endl;
        ret = new HostPort();
        getline(f, ret->hostname);
        string tmp;
        getline(f, tmp);
        ret->port = atoi(tmp.c_str());
      }
      f.close();
    }
  }
  if (ret) {
    cout << ret->hostname << endl;
    cout << ret->port     << endl;
  }
  // TODO, binder
  return ret;
}

int sendString(int sockfd, string buffer) {
  int rc;
  uint64_t size = buffer.size();
  cout << "sending " << size << "bytes." << endl;
  // Send the length of buffer so that receiving end knows when to stop.
  // TODO, endian-ness??
  rc = write(sockfd, (char*)&size, sizeof(uint64_t));
  cout << sizeof(size) << endl;

  // Send the buffer
  rc = write(sockfd, buffer.c_str(), size);

  return rc;
}

string recvString(int sockfd) {
  int rc;
  uint64_t size = 0;
  // Send the length of buffer so that receiving end knows when to stop.
  rc = read(sockfd, (char*)&size, sizeof(size));
  ASSERT(rc >= 0, "ERROR reading from socket");
  cout << "receiving " << size << "bytes." << endl;

  string ret(' ', size + 1);

  // Send the buffer
  rc = read(sockfd, &(ret[0]), size);
  ret[size] = '\0'; // null terminate.

  return ret;
}

