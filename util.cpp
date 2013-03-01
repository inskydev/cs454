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
  string hostEnvVar;
  string portEnvVar;
  string hostPortFile;

  if (type == HostPort::SERVER) {
    hostEnvVar = "SERVER_ADDRESS";
    portEnvVar = "SERVER_PORT";
    hostPortFile = "server_hostport";
  } else if (type == HostPort::BINDER) {
    hostEnvVar = "BINDER_ADDRESS";
    portEnvVar = "BINDER_PORT";
    hostPortFile = "binder_hostport";
  } else {
    return;
  }

  cout << hostEnvVar << " " << hostname << endl;
  cout << portEnvVar << " " << port << endl;

  if (put_file) {
    string user(getenv("USER"));

    string cmd1 = "echo " + hostname +
      " >  /u9/" + user + "/public_html/" + hostPortFile;
    string cmd2 = "echo " + port +
      " >> /u9/" + user + "/public_html/" + hostPortFile;
    system(cmd1.c_str());
    system(cmd2.c_str());
  }
}

HostPort* getHostPort(HostPort::Type type, bool debug, bool use_file) {
  HostPort* ret = NULL;
  string hostEnvVar;
  string portEnvVar;
  string hostPortFile;

  if (type == HostPort::SERVER) {
    hostEnvVar = "SERVER_ADDRESS";
    portEnvVar = "SERVER_PORT";
    hostPortFile = "server_hostport";
  } else if (type == HostPort::BINDER) {
    hostEnvVar = "BINDER_ADDRESS";
    portEnvVar = "BINDER_PORT";
    hostPortFile = "binder_hostport";
  } else {
    return NULL;
  }

  char* host = getenv(hostEnvVar.c_str());
  char* port = getenv(portEnvVar.c_str());
  if (host && port && not use_file) {
    ret = new HostPort();
    ret->hostname = string(host);
    ret->port = atoi(port);
  } else if (use_file) {
    string user(getenv("USER"));
    string rmCmd = "rm -f " + hostPortFile;
    system(rmCmd.c_str());
    // Source a web accessible file
    string wget_cmd = "wget http://www.student.cs.uwaterloo.ca/~" + user
      + "/" + hostPortFile +  " -O " + hostPortFile + " --quiet ";
    system(wget_cmd.c_str());
    ifstream f(hostPortFile, ios::in);
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
  if (ret && debug) {
    cout << ret->hostname << endl;
    cout << ret->port     << endl;
  }

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

