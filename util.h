#ifndef UTIL_H_
#define UTIL_H_

#include <string>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <stdint.h>
#include <strings.h>

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

  string hostname;
  int port;
};


string exec(string cmd);

void putHostPort(HostPort::Type type, string hostname, string port, bool put_file = false);

HostPort* getHostPort(HostPort::Type type, bool debug = false, bool use_file = false);

int sendString(int sockfd, string buffer);

string recvString(int sockfd);

#endif // UTIL_H_
