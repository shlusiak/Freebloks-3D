#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "constants.h"
#include <stdio.h>
#include <stdlib.h>

void error_exit(const char* message, int code){
	printf("Fehler %d: %s\n", code, message);
	getchar();
	exit (code);
}
