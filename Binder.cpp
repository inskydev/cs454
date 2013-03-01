#include "Binder.h"

#define SERVER_REGISTER   'R'
#define CLIENT_LOCATE     'L'
#define NONE_REGISTERED   "NONE_REGISTERED"
#define REREGISTER        "REREGISTER"
#define REGISTER_DONE     "DONE"
#define MALFORMED_REQUEST "MALFORMED_REQUEST"

int BinderClient::registerServer(const string&name,
                                 int* argTypes,
                                 const string& server) {
  // Send over the name and arg types as two strings.
  // Server side map the socket to hostport and aappend such data.

  // Name of an RPC call is normalized.
  // [ name ] <[arg1_input][arg_output][arg1_type][arg1_array]>...
  int numArgType = 0;
  for (int* c = argTypes; *c != 0; c++) {
    numArgType++;
  }
  char* buffer = new char[1 + server.size() + 1 + name.size() + numArgType*4 + 1];
  char* start = buffer;
  *start++ = SERVER_REGISTER;
  memcpy(start, server.c_str(), server.size());
  start += server.size();
  *start++ = '#'; // Delimiter
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
    a &= 0x4fffffff; // Chop off top 30bits.
    int arg_type = (a >> 24);
    *start++ = '0' + arg_type;
    int arg_len = (a & 0xffff); // Lower 16 bit is length
    *start++ = arg_len > 0 ? 'A' : 'S';
  }
  *start = NULL;

  if (sendString(transport.m_sockfd, string(buffer)) < 0) {
    return Error::BINDER_UNREACHEABLE;
  } else {
    return 0;
  }
}

void Binder::handleRequest(const string& msg, int clientSocket) {
  if (msg.size() == 0) {
    // Handle client termination by removing hostport in all mapping.

    HostPort hp = socketHostPortMap[clientSocket];
    socketHostPortMap.erase(clientSocket);

    for (map<string, list<HostPort> >::iterator m = mapping.begin();
        m != mapping.end();
        m++) {
      for (list<HostPort>::iterator l = m->second.begin(); l == m->second.end(); l++)
      {
        if (hp == *l) {
          m->second.erase(l);
          break;
        }
      }
    }
  } else if (msg[0] == SERVER_REGISTER) {
    HostPort hp;
    hp.fromString(msg);

    string key = msg.substr(msg.find('#')+1);
    // Remember hostport mapping for client disconnection.
    socketHostPortMap[clientSocket] = hp;

    map<string, list<HostPort> >::iterator s = mapping.find(key);

    if (s == mapping.end()) {
      mapping[key].push_front(hp);
    } else {
      list<HostPort>& l = s->second;
      bool duplicate = false;
      for (list<HostPort>::iterator l = s->second.begin(); l == s->second.end(); l++)
      {
        if (hp == *l) {
          duplicate = true;
          break;
        }
      }
      if (duplicate) {
        sendString(clientSocket, REREGISTER);
        return;
      }
    }
    sendString(clientSocket, REGISTER_DONE);
  } else if (msg[0] == CLIENT_LOCATE) {

    map<string, list<HostPort> >::iterator s = mapping.find(msg.substr(1));
    if (s == mapping.end()) {
      list<HostPort>& l = s->second;
      sendString(clientSocket, l.front().toString()); // send result
      l.push_back(l.front()); // Do round robin thingy.
      l.pop_front();
    } else {
      // does not have mapping yet.
      // FIXME: memory leak since unmapped apis are never removed.
      sendString(clientSocket, NONE_REGISTERED);
    }
  } else {
    sendString(clientSocket, MALFORMED_REQUEST);
  }
}
