#include "Binder.h"

#define SERVER_REGISTER   'R'
#define CLIENT_LOCATE     'L'
#define TERMINATE_ALL     'T'
#define CACHE_LOCATE      'C'
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
  if (transport.sendString(msg) < 0) {
    return Error::BINDER_UNREACHEABLE;
  } else {
    string serverMsg;
    int rc = recvString(transport.m_sockfd, serverMsg);
    if (rc < 0) return Error::BINDER_UNREACHEABLE;

    // TODO, server may reply with warning.
    return serverMsg == REGISTER_DONE ? 0 : -1;
  }
}

int BinderClient::terminateAll() {
  if (transport.sendString(string(1, TERMINATE_ALL)) < 0) {
    return Error::BINDER_UNREACHEABLE;
  }

  string serverMsg;
  int rc = recvString(transport.m_sockfd, serverMsg);
  if (rc < 0) return Error::BINDER_UNREACHEABLE;
  return 0;
}


int BinderClient::locateServer(const string& name, int* argType, HostPort* hp) {
  string args = normalizeArgs(name, argType);
  string msg = string(1, CLIENT_LOCATE) + "#" + args;
  if (sendString(transport.m_sockfd, msg) < 0) {
    return Error::BINDER_UNREACHEABLE;
  }

  //get reply from binder
  string serverMsg;
  int rc = recvString(transport.m_sockfd, serverMsg);
  if (rc < 0) return Error::BINDER_UNREACHEABLE;

  if (serverMsg == NONE_REGISTERED) {
    return Error::NO_SERVER_WITH_ARGTYPE;
  }

  hp->fromString(serverMsg);
  return 0;
}


int BinderClient::cacheLocation(map<string, list<HostPort> >& ret) {
  string msg = string(1, CACHE_LOCATE);
  if (sendString(transport.m_sockfd, msg) < 0) {
    return Error::BINDER_UNREACHEABLE;
  }

  string reply;
  int rc = recvString(transport.m_sockfd, reply);
  if (rc < 0) return rc;

  ret.clear();
  vector<string> api_level = split(reply, '#');
  vector<string>::iterator api = api_level.begin();
  ++api;
  for (;api != api_level.end(); api++) {
    vector<string> hostports = split(*api, '|');
    vector<string>::iterator hp = hostports.begin();
    string args = *hp;
    hp++;

    for (; hp != hostports.end(); hp++) {
      HostPort tmp;
      tmp.fromString(*hp);
      ret[args].push_back(tmp);
    }
  }

  return 0;
}

void Binder::disconnected(int clientSocket) {
  // Handle "server" client termination by removing hostport in all apiMapping.
  if (socketHostPortMap.find(clientSocket) == socketHostPortMap.end()) {
    return;
  }

  HostPort hp = socketHostPortMap[clientSocket];
  socketHostPortMap.erase(clientSocket);

  for (map<string, list<HostPort> >::iterator m = apiMapping.begin();
      m != apiMapping.end();
      m++) {
    for (list<HostPort>::iterator l = m->second.begin(); l != m->second.end(); l++)
    {
      if (hp == *l) {
        m->second.erase(l);
        break;
      }
    }
  }

  if (terminating.get() && socketHostPortMap.size() == 0) ++terminate;
}

void Binder::handleRequest(int clientSocket, const string& msg) {
  // NOTE, this must be single threaded.
  if (msg[0] == SERVER_REGISTER) {
    string hostportStr = msg.substr(1, msg.find('#') - 1);
    HostPort hp;
    hp.fromString(hostportStr);

    string key = msg.substr(msg.find('#')+1);
    // Remember hostport apiMapping for client disconnection.
    socketHostPortMap[clientSocket] = hp;

    map<string, list<HostPort> >::iterator s = apiMapping.find(key);
    if (s == apiMapping.end()) {
      apiMapping[key].push_front(hp);
    } else {
      list<HostPort>& l = s->second;
      bool duplicate = false;
      for (list<HostPort>::iterator l = s->second.begin(); l != s->second.end(); l++)
      {
        if (hp == *l) {
          duplicate = true;
          break;
        }
      }
      if (duplicate) {
        sendString(clientSocket, REREGISTER);
        return;
      } else {
        l.push_front(hp);
      }
    }
    sendString(clientSocket, REGISTER_DONE);
  } else if (msg[0] == CLIENT_LOCATE) {
    string key = msg.substr(msg.find('#')+1);

    map<string, list<HostPort> >::iterator s = apiMapping.find(key);
    if (s != apiMapping.end() && s->second.size()) {
      list<HostPort>& l = s->second;
      sendString(clientSocket, l.front().toString()); // send result
      l.push_back(l.front()); // Do round robin thingy.
      l.pop_front();
    } else {
      // does not have apiMapping yet.
      sendString(clientSocket, NONE_REGISTERED);
    }
  } else if (msg[0] == TERMINATE_ALL) {
    // Do not accept new connections
    ++terminating;

    // Send message to all servers to terminate.
    for (map<int, HostPort>::iterator i = socketHostPortMap.begin();
        i != socketHostPortMap.end();
        i++) {
      sendString(i->first, msg);
    }
    // Asyncrhonously wait for each server to ack
  } else if (msg[0] == CACHE_LOCATE) {
    string response = "C";
    for (map<string, list<HostPort> >::iterator m = apiMapping.begin();
      m != apiMapping.end();
      m++) {

      response += "#" + m->first + "|";
      for (list<HostPort>::iterator l = m->second.begin(); l != m->second.end(); l++)
      {
        response += l->toString() + "|";
      }
    }
    sendString(clientSocket, response);
  } else {
    sendString(clientSocket, MALFORMED_REQUEST);
  }
}
