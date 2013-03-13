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
  if (result.size() && result[result.size() - 1] == '\n') {
    int i = -1;
    for (i = result.size() - 1; i >= 0; i--) {
      if (result[i] != '\n' && result[i] != ' ') {
        break;
      }
    }
    result = result.substr(0, i+1);
  }
  return result;
}

void putHostPort(HostPort::Type type, string hostname, string port) {
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

  string user(getenv("USER"));

  string cmd1 = "echo " + hostname +
    " >  /u9/" + user + "/public_html/" + hostPortFile;
  string cmd2 = "echo " + port +
    " >> /u9/" + user + "/public_html/" + hostPortFile;
  system(cmd1.c_str());
  system(cmd2.c_str());
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
      string str;
      getline(f, str);
      if (str.size()) {
        cout << "Using " << hostPortFile << endl;
        ret = new HostPort();
        ret->hostname = str;
        getline(f, str);
        ret->port = atoi(str.c_str());
      }
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

int recvString(int sockfd, string& msg) {
  uint32_t size = 0;
  // Read the length of buffer so that receiving end knows when to stop.
  int num_bytes = 0;
  while (num_bytes < sizeof(size)) {
    char* begin = (char*)&size;
    int rc = read(sockfd, begin + num_bytes, sizeof(size) - num_bytes);
    if (rc == 0) {
      cout << "client terminated?" << endl;
      return -1;
    }
    if (rc < 0) return -1;
    num_bytes += rc;
  }

  if (size == 0) {
    msg = "";
    return 0;
  }

  // Read the buffer
  char* buffer = new char[size]; // leave space for null
  num_bytes = 0;
  while (num_bytes < size) {
    char* begin = (char*)buffer;
    int rc = read(sockfd, begin + num_bytes, size - num_bytes);
    if (rc < 0) return -1;
    num_bytes += rc;
  }
  buffer[size - 1] = NULL; // null terminate.
  msg = string(buffer);
  delete [] buffer;

  return 0;
}

int ArgType::get() const {
  int ret = 0;
  if (m_input) {
    ret |= (1 << ARG_INPUT);
  }
  if (m_output) {
    ret |= 1 << ARG_OUTPUT;
  }
  ret |= m_type << 16;
  ret |= m_num_times;
  return ret;
}

int* formatArgTypes(const vector<ArgType>& args) {
  int* ret = new int[args.size() + 1];

  for (int i = 0; i < args.size(); i++) {
    ret[i] = args.at(i).get();
  }

  ret[args.size()] = 0;

  return ret;
}

string normalizeArgs(const string& name, int* argTypes) {
  // Name of an RPC call is normalized.
  // [ name ] <[arg1_input][arg_output][arg1_type][arg1_array]>...
  int numArgType = 0;
  for (int* c = argTypes; *c != 0; c++) {
    numArgType++;
  }
  char* buffer = new char[name.size() + numArgType*4 + 2];
  char* start = buffer;
  memcpy(start, name.c_str(), name.size());
  start += name.size();

  for (int* arg = argTypes; *arg != 0; arg++) {
    int a = *arg;
    if (a & (1 << ARG_INPUT)) {
      *start++ = 'i';
    } else {
      *start++ = ' ';
    }
    if (a & (1 << ARG_OUTPUT)) {
      *start++ = 'o';
    } else {
      *start++ = ' ';
    }
    a = (a << 2) >> 2; // chop off top two bits.
    int arg_type = (a >> 16); // chop off bottom 16 bits
    *start++ = '0' + arg_type;
    int arg_len = (a & 0xffff); // Lower 16 bit is length
    *start++ = arg_len > 0 ? 'A' : 'S';
  }

  *start = NULL;
  return string(buffer);
}

string serializeCall(const string& name, int* argTypes, void** args, bool outputOnly) {
  string request = "C" + normalizeArgs(name, argTypes) + "#";

  void** it = args;
  int* at = argTypes;
  for (;(*at); it++, at++) {
    // need to know how to increment the ptrs of different size

    char* curr = (char*)*it;
    int type = ((*at)>>16) & 0xFF;
    int length = ((*at)) & 0xFF;
    if (!length) {
      length = 1;
    }
    request += to_string((long long int)length);
    request += ":";

    for (int i = 0; i < length; i++) {
      if (type == ARG_CHAR) {
        request += string(1, *curr) + ";";
        curr += sizeof(char);
      } else if (type == ARG_SHORT) {
        long long int value = *(short*)curr;
        request += to_string(value) + ";";
        curr += sizeof(short);
      } else if (type == ARG_INT) {
        long long int value = *(int*)curr;
        request += to_string(value) + ";";
        curr += sizeof(int);
      } else if (type == ARG_LONG) {
        long long int value = *(long*)curr;
        request += to_string(value) + ";";
        curr += sizeof(long);
      } else if (type == ARG_DOUBLE) {
        long double value = *(double*)curr;
        request += to_string(value) + ";";
        curr += sizeof(double);
      } else if (type == ARG_FLOAT) {
        long double value = *(float*)curr;
        request += to_string(value) + ";";
        curr += sizeof(float);
      } else {
        printf("error type\n");
      }
    }
  }

}
