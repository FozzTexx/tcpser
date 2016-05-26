#include <stdio.h>
#include <stdlib.h>     // for exit,atoi
#include <unistd.h>
#include "debug.h"
#include "phone_book.h"
#include "init.h"

void print_help(char *name)
{
  fprintf(stderr, "Usage: %s <parameters>\n", name);
  fprintf(stderr, "  -p   port to listen on (defaults to 6400)\n");
  fprintf(stderr, "               -- or -- \n");
  fprintf(stderr, "  -p  ip_address:port to specify which ip address and port to listen on.\n");
  fprintf(stderr, "  -t   trace flags: (can be combined)\n");
  fprintf(stderr, "       's' = modem input\n");
  fprintf(stderr, "       'S' = modem output\n");
  fprintf(stderr, "       'i' = IP input\n");
  fprintf(stderr, "       'I' = IP output\n");
  fprintf(stderr, "  -l   0 (NONE), 1 (FATAL) - 7 (DEBUG_X) (defaults to 0)\n");
  fprintf(stderr, "  -L   log file (defaults to stderr)\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "  The following can be repeated for each modem desired\n");
  fprintf(stderr, "  (-s, -S, and -i will apply to any subsequent device if not set again)\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "  -d   serial device (e.g. /dev/ttyS0). Cannot be used with -v\n");
  fprintf(stderr, "  -v   tcp port for VICE RS232 (e.g. 25232). Cannot be used with -d\n");
  fprintf(stderr, "  -s   serial port speed (defaults to 38400)\n");
  fprintf(stderr, "  -S   speed modem will report (defaults to -s value)\n");
  fprintf(stderr, "  -I   invert DCD pin\n");
  fprintf(stderr, "  -n   add phone entry (number=replacement)\n");
  fprintf(stderr, "  -a   filename to send to local side upon answer\n");
  fprintf(stderr, "  -A   filename to send to remote side upon answer\n");
  fprintf(stderr, "  -c   filename to send to local side upon connect\n");
  fprintf(stderr, "  -C   filename to send to remote side upon connect\n");
  fprintf(stderr, "  -N   filename to send when no answer\n");
  fprintf(stderr, "  -B   filename to send when modem(s) busy\n");
  fprintf(stderr, "  -T   filename to send upon inactivity timeout\n");
  fprintf(stderr,
          "  -i   modem init string (defaults to '', leave off 'at' prefix when specifying)\n");
  fprintf(stderr,
          "  -D   direct connection (follow with hostname:port for caller, : for receiver)\n");
  exit(1);
}

int init(int argc, char **argv, modem_config cfg[], int max_modem, char **ip_addr,
	 int *port, char *all_busy, int all_busy_len)
{
  int i = 0;
  int j = 0;
  int opt = 0;
  int trace_flags = 0;
  char *tok;
  int dce_set = FALSE;
  int tty_set = FALSE;


  LOG_ENTER();
  *port = 6400;
  mdm_init_config(&cfg[0]);
  cfg[0].dte_speed = 38400;
  cfg[0].dce_speed = 38400;

  while (opt > -1 && i < max_modem) {
    opt = getopt(argc, argv, "p:s:S:d:v:hw:i:Il:L:t:n:a:A:c:C:N:B:T:D:");
    switch (opt) {
    case 't':
      trace_flags = log_get_trace_flags();
      for (j = 0; j < strlen(optarg); j++) {
        switch (optarg[j]) {
        case 's':
          trace_flags |= TRACE_MODEM_IN;
          break;
        case 'S':
          trace_flags |= TRACE_MODEM_OUT;
          break;
        case 'i':
          trace_flags |= TRACE_IP_IN;
          break;
        case 'I':
          trace_flags |= TRACE_IP_OUT;
          break;
        }
        log_set_trace_flags(trace_flags);
      }
      break;
    case 'a':
      strncpy(cfg[i].data.local_answer, optarg, sizeof(cfg[i].data.local_answer));
      break;
    case 'A':
      strncpy(cfg[i].data.remote_answer, optarg, sizeof(cfg[i].data.remote_answer));
      break;
    case 'c':
      strncpy(cfg[i].data.local_connect, optarg, sizeof(cfg[i].data.local_connect));
      break;
    case 'C':
      strncpy(cfg[i].data.remote_connect, optarg, sizeof(cfg[i].data.remote_connect));
      break;
    case 'B':
      strncpy(all_busy, optarg, all_busy_len);
      break;
    case 'N':
      strncpy(cfg[i].data.no_answer, optarg, sizeof(cfg[i].data.no_answer));
      break;
    case 'T':
      strncpy(cfg[i].data.inactive, optarg, sizeof(cfg[i].data.inactive));
      break;
    case 'i':
      strncpy(cfg[i].config0, optarg, 255);
      break;
    case 'I':
      cfg[i].invert_dcd = TRUE;
      break;
    case 'p':
      if (strstr(optarg, ":") > 0) {
	*ip_addr = strtok(optarg, ":");
	*port = (atoi(strtok(NULL,":")));
      }
      else
	*port = (atoi(optarg));
      break;
    case 'n':
      tok = strtok(optarg, "=");
      pb_add(tok, strtok(NULL, "="));
      break;
    case 'l':
      log_set_level(atoi(optarg));
      break;
    case 'L':
      log_set_file(fopen(optarg, "w+"));
      // should check to see if an error occurred...
      break;
    case 's':
      cfg[i].dte_speed = atoi(optarg);
      LOG(LOG_ALL, "Setting DTE speed to %d", cfg[i].dte_speed);
      if (dce_set == FALSE)
        cfg[i].dce_speed = cfg[i].dte_speed;
      break;
    case '?':
    case 'h':
      print_help(argv[0]);
      break;
    case 'd':
    case 'v':
      if (tty_set) {
        if (++i < max_modem) {
          dce_set = FALSE;
          mdm_init_config(&cfg[i]);
          cfg[i].dte_speed = cfg[i - 1].dte_speed;
          cfg[i].dce_speed = cfg[i - 1].dce_speed;
          cfg[i].dce_data.is_ip232 = FALSE;
          strncpy(cfg[i].config0, cfg[i - 1].config0, sizeof(cfg[i].config0));
          strncpy(cfg[i].data.local_connect, cfg[i - 1].data.local_connect,
                  sizeof(cfg[i].data.local_connect));
          strncpy(cfg[i].data.remote_connect, cfg[i - 1].data.remote_connect,
                  sizeof(cfg[i].data.remote_connect));
          strncpy(cfg[i].data.local_answer, cfg[i - 1].data.local_answer,
                  sizeof(cfg[i].data.local_answer));
          strncpy(cfg[i].data.remote_answer, cfg[i - 1].data.remote_answer,
                  sizeof(cfg[i].data.remote_answer));
          strncpy(cfg[i].data.no_answer, cfg[i - 1].data.no_answer,
                  sizeof(cfg[i].data.no_answer));
          strncpy(cfg[i].data.inactive, cfg[i - 1].data.inactive, sizeof(cfg[i].data.inactive));
        }
        else {
          LOG(LOG_WARN, "Maximum modems defined - ignoring extra");
          break;
        }
      }
      strncpy(cfg[i].dce_data.tty, optarg, sizeof(cfg[i].dce_data.tty));
      cfg[i].dce_data.is_ip232 = ('v' == opt);
      tty_set = TRUE;
      break;
    case 'S':
      cfg[i].dce_speed = atoi(optarg);
      dce_set = TRUE;
      break;
    case 'D':
      cfg[i].data.direct_conn = TRUE;
      strncpy(cfg[i].data.direct_conn_num, optarg, sizeof(cfg[i].data.direct_conn_num));
      break;
    }
  }

  if (tty_set) {
    if (i < max_modem)
      ++i;
  }
  else {
    // no modems defined
    LOG(LOG_FATAL, "No modems defined");
    print_help(argv[0]);
  }

  LOG(LOG_DEBUG, "Read configuration for %i serial port(s)", i);

  LOG_EXIT();
  return i;
}
