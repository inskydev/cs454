#include "util.h"

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

void putHostPort(HostPort::Type type, string hostname, string port, bool put_file) {
  cout << "SERVER_ADDRESS " << hostname << endl;
  cout << "SERVER_PORT " << port << endl;

  if (put_file) {
    string user(getenv("USER"));

    string cmd1 = "echo " + hostname +
      " >  /u9/" + user + "/public_html/server_hostport";
    string cmd2 = "echo " + port +
      " >> /u9/" + user + "/public_html/server_hostport";
    system(cmd1.c_str());
    system(cmd2.c_str());
  }
}

HostPort* getHostPort(HostPort::Type type, bool debug, bool use_file) {
  HostPort* ret = NULL;
  if (type == HostPort::SERVER) {
    char* host = getenv("SERVER_ADDRESS");
    char* port = getenv("SERVER_PORT");
    if (host && port && not use_file) {
      ret = new HostPort();
      ret->hostname = string(host);
      ret->port = atoi(port);
    } else if (use_file) {
      string user(getenv("USER"));
      system("rm -f server_hostport");
      // Source a web accessible file
      string wget_cmd = "wget http://www.student.cs.uwaterloo.ca/~" + user
        + "/server_hostport -O server_hostport --quiet ";
      system(wget_cmd.c_str());
      ifstream f("server_hostport", ios::in);
      if (f.is_open()) {
        cout << "Using server published hostport." << endl;
        ret = new HostPort();
        getline(f, ret->hostname);
        string tmp;
        getline(f, tmp);
        ret->port = atoi(tmp.c_str());
      }
      f.close();
    }
  }
  if (ret && debug) {
    cout << ret->hostname << endl;
    cout << ret->port     << endl;
  }
  // TODO, binder
  return ret;
}

int sendString(int sockfd, string buffer) {
  uint32_t size = buffer.size() + 1; // includes null char
  // Send the length of buffer so that receiving end knows when to stop.
  // TODO, endian-ness??
  int num_bytes = 0;
  while (num_bytes < sizeof(size)) {
    char* begin = (char*)&size;
    int rc = write(sockfd, begin + num_bytes, sizeof(size) - num_bytes);
    if (rc < 0) return rc;
    num_bytes += rc;
  }

  // Send the buffer
  num_bytes = 0;
  while (num_bytes < size) {
    char* begin = (char*)buffer.c_str();
    int rc = write(sockfd, begin + num_bytes, size - num_bytes);
    if (rc < 0) return rc;
    num_bytes += rc;
  }

  return 0;
}

string recvString(int sockfd) {
  uint32_t size = 0;
  // Read the length of buffer so that receiving end knows when to stop.
  int num_bytes = 0;
  while (num_bytes < sizeof(size)) {
    char* begin = (char*)&size;
    int rc = read(sockfd, begin + num_bytes, sizeof(size) - num_bytes);
    if (rc == 0) {
      return "";
    }
    if (rc < 0) return "";
    num_bytes += rc;
  }

  // Read the buffer
  char* buffer = new char[size]; // leave space for null
  num_bytes = 0;
  while (num_bytes < size) {
    char* begin = (char*)buffer;
    int rc = read(sockfd, begin + num_bytes, size - num_bytes);
    if (rc < 0) return "";
    num_bytes += rc;
  }
  buffer[size] = NULL; // null terminate.
  string s(buffer);
  delete [] buffer;

  return s;
}

