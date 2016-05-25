#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#include "modem_core.h"

extern void print_help(char *name);
extern int init(int argc, char **argv, modem_config cfg[], int max_modem, char **ip_addr,
		int *port, char *all_busy, int all_busy_len);
