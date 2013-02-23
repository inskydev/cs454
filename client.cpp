#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.cpp"
#include "Transporter.cpp"

using namespace std;

int main(int argc, char *argv[]) {
  HostPort* hp = getHostPort(HostPort::SERVER);
  ASSERT(hp, "Error, no server hostport found.");
  Transporter transport(hp->hostname, hp->port);

  printf("Please enter the message: \n");
  //string msg;
  //cin >> msg;
  //string reply = transport.query(msg);
  string reply = transport.query("hhahahaha");

  printf("%s\n", reply.c_str());


  return 0;
}
