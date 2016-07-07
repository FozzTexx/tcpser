#include <string.h>

#include "debug.h"
#include "ip.h"
#include "nvt.h"

int nvt_init_config(nvt_vars *vars)
{
  int i;


  vars->binary_xmit = FALSE;
  vars->binary_recv = FALSE;
  for (i = 0; i < 256; i++)
    vars->term[i] = 0;

  return 0;
}

int get_nvt_cmd_response(int action, int type)
{
  char rc = 0;


  if (type == TRUE) {
    switch (action) {
    case NVT_DO:
      rc = NVT_WILL_RESP;
      break;
    case NVT_DONT:
      rc = NVT_WONT_RESP;
      break;
    case NVT_WILL:
      rc = NVT_DO_RESP;
      break;
    case NVT_WONT:
      rc = NVT_DONT_RESP;
      break;
    }
  }
  else {
    switch (action) {
    case NVT_DO:
    case NVT_DONT:
      rc = NVT_WONT_RESP;
      break;
    case NVT_WILL:
    case NVT_WONT:
      rc = NVT_DONT_RESP;
      break;
    }
  }
  return rc;
}

int parse_nvt_subcommand(int fd, nvt_vars *vars, unsigned char *data, int len, int speed)
{
  // overflow issue, again...
  nvtOption opt = data[2];
  char resp[100];
  char *response = resp + 3;
  int resp_len = 0;
  int response_len = 0;
  char tty_type[] = "VT100";
  int rc;
  int slen = 0;
  char buf[50];


  for (rc = 2; rc < len - 1; rc++) {
    if (NVT_IAC == data[rc])
      if (NVT_SE == data[rc + 1]) {
        rc += 2;
        break;
      }
  }

  if (rc > 5 && (NVT_SB_SEND == data[3])) {
    switch (opt) {
    case NVT_OPT_TERMINAL_TYPE:
    case NVT_OPT_TERMINAL_SPEED:
    case NVT_OPT_X_DISPLAY_LOCATION:   // should not have to have these
    case NVT_OPT_ENVIRON:	       // but telnet seems to expect.
    case NVT_OPT_NEW_ENVIRON:	       // them.
      response[response_len++] = NVT_SB_IS;
      switch (opt) {
      case NVT_OPT_TERMINAL_TYPE:
        slen = strlen(tty_type);
        strncpy(response + response_len, tty_type, slen);
        response_len += slen;
        break;

      case NVT_OPT_TERMINAL_SPEED:
	sprintf(buf, "%i,%i", speed, speed);
	slen = strlen(buf);
	strncpy(response + response_len, buf, slen);
	response_len += slen;
	break;
	
      default:
	break;
      }
      break;

    default:
      break;
    }
  }

  if (response_len) {
    resp[resp_len++] = NVT_IAC;
    resp[resp_len++] = NVT_SB;
    resp[resp_len++] = opt;
    resp_len += response_len;
    resp[resp_len++] = NVT_IAC;
    resp[resp_len++] = NVT_SE;
    ip_write(fd, resp, resp_len);
  }
  return rc;
}

int send_nvt_command(int fd, nvt_vars *vars, char action, int opt)
{
  char cmd[3];


  cmd[0] = NVT_IAC;
  cmd[1] = action;
  cmd[2] = opt;

  ip_write(fd, cmd, 3);
  vars->term[opt] = action;

  return 0;
}

int parse_nvt_command(int fd, nvt_vars *vars, nvtCommand action, nvtOption opt, int parity)
{
  char resp[3];
  int accept = FALSE;
  

  resp[0] = NVT_IAC;
  resp[2] = opt;

  switch (opt) {
  case NVT_OPT_TRANSMIT_BINARY:
    switch (action) {
    case NVT_DO:
      if (!parity) {
	LOG(LOG_INFO, "Enabling telnet binary xmit");
	vars->binary_xmit = TRUE;
	accept = TRUE;
      }
      break;
    case NVT_DONT:
      LOG(LOG_INFO, "Disabling telnet binary xmit");
      vars->binary_xmit = FALSE;
      accept = TRUE;
      break;
    case NVT_WILL:
      if (!parity) {
	LOG(LOG_INFO, "Enabling telnet binary recv");
	vars->binary_recv = TRUE;
	accept = TRUE;
      }
      break;
    case NVT_WONT:
      LOG(LOG_INFO, "Disabling telnet binary recv");
      vars->binary_recv = FALSE;
      accept = TRUE;
      break;

    default:
      break;
    }
    resp[1] = get_nvt_cmd_response(action, accept);
    break;

  case NVT_OPT_NAWS:
  case NVT_OPT_TERMINAL_TYPE:
  case NVT_OPT_TERMINAL_SPEED:
  case NVT_OPT_SUPPRESS_GO_AHEAD:
  case NVT_OPT_ECHO:
  case NVT_OPT_X_DISPLAY_LOCATION:     // should not have to have these
  case NVT_OPT_ENVIRON:		       // but telnet seems to expect.
  case NVT_OPT_NEW_ENVIRON:	       // them.
    resp[1] = get_nvt_cmd_response(action, TRUE);
    break;

  default:
    resp[1] = get_nvt_cmd_response(action, FALSE);
    break;
  }
  ip_write(fd, resp, 3);
  return 0;
}
