#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "util.h"
#include "Transporter.cpp"

int main(int argc, char *argv[]) {

  int argTypes = 0;
  void** args0 = NULL; //(void **)malloc(0 * sizeof(void *));
  int s4 = rpcCall("f0", &argTypes, NULL);

  int* a1 = formatArgTypes(
      {
        ArgType(INPUT, OUTPUT, ARG_CHAR, 0),
      });

  char value = 'v';

  void** args = (void**)malloc(sizeof(void*) * 1);
  args[0] = &value;

  int s2 = rpcCall("f1", a1, args);

  //rpcTerminate();
}
