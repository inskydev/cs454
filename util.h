#ifndef UTIL_H_
#define UTIL_H_

#include <map>
#include <list>
#include <vector>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <stdint.h>
#include <strings.h>
#include "rpc.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define INPUT true
#define OUTPUT true
#define NOT_INPUT true
#define NOT_OUTPUT true

struct Error {
  enum TYPE {
    MISSING_ENV_VAR = -1,
    UNINITIALIZED_BINDER = -2,
    BINDER_UNREACHEABLE = -3,
    UNINITIALIZED_SERVER = -4,
  };
};

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
struct PMutex {
  PMutex() {
    pthread_mutex_init(&mutex, NULL); // defualt mutex, unlocked
  }

  void lock() {
    pthread_mutex_lock(&mutex);
  }

  void unlock() {
    pthread_mutex_unlock(&mutex);
  }


  pthread_mutex_t mutex;
};

struct Counter {
  Counter() : value(0) {
  }
  void operator++() {
    mutex.lock();
    value++;
    mutex.unlock();
  }

  void operator--() {
    mutex.lock();
    value--;
    mutex.unlock();
  }

  int get() {
    mutex.lock();
    int copy = value;
    mutex.unlock();
    return copy;
  }

  PMutex mutex;
  int value;
};


struct HostPort {
  enum Type {
    SERVER,
    BINDER,
  };

  HostPort()
    : hostname("UNINIT_HOSTNAME"),
      port(-1) {}

  string toString() const {
    return hostname + ":" + to_string((long long int)port);
  }

  bool operator==(const HostPort& rhs) const {
    return hostname == rhs.hostname && port == rhs.port;
  }

  void fromString(const string& msg) {
    size_t port = msg.find(':');
    hostname = msg.substr(1, port); // Skip msg header type
    port = atoi(string(msg.substr(port+1, msg.find('#') - port + 1)).c_str());
  }

  string hostname;
  int port;
};

struct ArgType {
  ArgType(bool input, bool output, int type, int num_times)
    : input(input), output(output), type(type), num_times(num_times){
      if (type != ARG_CHAR || type != ARG_SHORT ||
          type != ARG_INT || type != ARG_LONG ||
          type != ARG_DOUBLE || type != ARG_FLOAT) {
        ASSERT(false, "Unknown type %d", type);
      }
      if (num_times > 0xffff) {
        ASSERT(false, "Cannot have array length greater than %d", num_times);
      }

    }

  int get() const;

  bool input;
  bool output;
  int type;      // INT or something else
  int num_times; // zero means scalar

};


string exec(string cmd);

void putHostPort(HostPort::Type type, string hostname, string port, bool put_file = false);

HostPort* getHostPort(HostPort::Type type, bool debug = false, bool use_file = false);

int sendString(int sockfd, string buffer);

string recvString(int sockfd);

int* formatArgTypes(const vector<ArgType>& args);

#endif // UTIL_H_
