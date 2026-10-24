#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "constants.h"
#include <stdio.h>
#include <stdlib.h>

void error_exit(const char* message, int code){
	fprintf(stderr, "Error %d: %s\n", code, message);
#ifdef WIN32
	/* Keep the console window open so the message can be read */
	getchar();
#endif
	exit (code);
}
