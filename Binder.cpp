#include "Binder.h"

#define SERVER_REGISTER 'R'
#define CLIENT_LOCATE   'L'
#define NONE_REGISTERED "NONE_REGISTERED"

int BinderClient::registerServer(char* name, int* argTypes, const string& server) {
  // Send over the name and arg types as two strings.
  // Server side map the socket to hostport and aappend such data.

  // Name of an RPC call is normalized.
  // [ name ] <[arg1_input][arg_output][arg1_type][arg1_array]>...
  int numArgType = 0;
  for (int* c = argTypes; *c != NULL; c++) {
    numArgType++;
  }
  char* buffer = new char[1 + server.size() + 1 + name.size() + numArgType*4 + 1];
  char* start = buffer;
  *start++ = BINDER_REGISTER;
  memcpy(start, server.c_str(), server.size());
  start += server.size();
  *start++ = '#'; // Delimiter
  memcpy(start, name.c_str(), name.size());
  start += name.size();
  for (int* arg = argTypes; *arg != NULL; arg++) {
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
    a &= 0x4fffffff; // Chop off top 30bits.
    int arg_type = (a >> 24);
    *start++ = '0' + arg_type;
    int arg_len = (a & 0xffff); // Lower 16 bit is length
    *start++ = arg_len > 0 ? 'A' : 'S';
  }
  *start = NULL;

  if (sendString(BinderClient->transport->socket, string(buffer)) < 0) {
    return BINDER_UNREACHEABLE;
  } else {
    return 0;
  }
}

void Binder::handleRequest(const string& msg) {
  if (msg[0] == SERVER_REGISTER) {
    HostPort hp;
    hp.fromString(msg);
    mapping[msg.substr(msg.find('#')+1)].push_front(hp);
  } else if (msg[0] == CLIENT_LOCATE) {
    list<HostPort>& s = mapping[msg.substr(1)];
    if (s.size()) {
      sendString(s.front().toString()); // send result
      s.push_back(s.front()); // Do round robin thingy.
      s.pop_front();
    } else {
      // does not have mapping yet.
      sendString(NONE_REGISTERED);
    }
  } else {
    cout << "unknown request type" << endl;

  }
}
