#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#include "modem_core.h"

void print_help(char *name);
int init(int argc,
         char **argv,
         modem_config cfg[], int max_modem, int *port, char *all_busy, int all_busy_len);
