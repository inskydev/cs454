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

  int* argTypes0 = formatArgTypes(
      {
        ArgType(INPUT, OUTPUT, ARG_CHAR, 0),
        ArgType(INPUT, NOT_OUTPUT, ARG_INT, 2)
      });

  //rpcTerminate();
}
