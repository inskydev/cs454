#include "Binder.h"

#define SERVER_REGISTER   'R'
#define CLIENT_LOCATE     'L'
#define TERMINATE_ALL     'T'
#define NONE_REGISTERED   "NONE_REGISTERED"
#define REREGISTER        "REREGISTER"
#define REGISTER_DONE     "DONE"
#define MALFORMED_REQUEST "MALFORMED_REQUEST"

int BinderClient::registerServer(const string& name,
                                 int* argTypes,
                                 const string& server) {
  // Send over the name and arg types as two strings.
  // Server side map the socket to hostport and append such data.
  string args = normalizeArgs(name, argTypes);
  string msg = string(1, SERVER_REGISTER) + server + "#" + args;
  cout << msg << endl;
  if (transport.sendString(msg) < 0) {
    return Error::BINDER_UNREACHEABLE;
  } else {
    return 0;
  }
}

int BinderClient::terminateAll() {
  if (transport.sendString(string(1, TERMINATE_ALL)) < 0) {
    return Error::BINDER_UNREACHEABLE;
  } else {
    return 0;
  }
}


int BinderClient::locateServer(const string& name, int* argType){
  
  string args = normalizeArgs(name, argType);
  string msg = string(1, CLIENT_LOCATE) + "#" + args;
  if (sendString(transport.m_sockfd, msg) < 0) {
    return Error::BINDER_UNREACHEABLE;
  } else {
    return 0;
  }
}



void Binder::disconnected(int clientSocket) {
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

  if (socketHostPortMap.size() == 0) ++terminate;
}

void Binder::handleRequest(int clientSocket, const string& msg) {
  if (msg[0] == SERVER_REGISTER) {
    cout << "register" << endl;
    cout << msg << endl;

    string hostportStr = msg.substr(1, msg.find('#') - 1);
    HostPort hp;
    hp.fromString(hostportStr);

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
      sendString(clientSocket, NONE_REGISTERED);
    }
  } else if (msg[0] == TERMINATE_ALL) {
    // Do not accept new connections
    ++terminating;
    if (socketHostPortMap.size() == 0) ++terminate;
    // Send message to all servers to terminate.
    for (map<int, HostPort>::iterator i = socketHostPortMap.begin();
        i != socketHostPortMap.end();
        i++) {
      sendString(i->first, msg);
    }
  } else {
    sendString(clientSocket, MALFORMED_REQUEST);
  }
}
