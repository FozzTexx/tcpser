#include <stdio.h>

#include <sys/socket.h> // for recv...
#include <unistd.h>     // for read...
#include <stdlib.h>     // for exit...
#include <sys/param.h>
#include <sys/time.h>
#include <pthread.h>

#include "util.h"
#include "debug.h"
#include "nvt.h"
#include "modem_core.h"
#include "ip.h"
//#include "serial.h"
#include "getcmd.h"
#include "bridge.h"

const char MDM_NO_ANSWER[] = "NO ANSWER\n";

//const char CONNECT_NOTICE[] = "\r\nCONNECTING...\r\n";

//const char TELNET_NOTICE[] = "TELNET MODE ENABLED\r\n";

int accept_connection(modem_config *cfg)
{
  LOG_ENTER();

  cfg->line_data.fd = ip_accept(cfg->line_data.sfd);
  if (cfg->line_data.fd > -1) {
    cfg->line_data.valid_conn = TRUE;
    if (cfg->data.direct_conn == TRUE) {
      cfg->conn_type = MDM_CONN_INCOMING;
      mdm_off_hook(cfg);
      cfg->cmd_mode = TRUE;
    }
    else {
      //line_write(cfg,(char*)CONNECT_NOTICE,strlen(CONNECT_NOTICE));
      cfg->rings = 0;
      mdm_send_ring(cfg);
    }
    // tell parent I got it.
    LOG(LOG_DEBUG, "Informing parent task that I am busy");
    writePipe(cfg->data.mp[0][1], MSG_ACCEPTED);
  }
  LOG_EXIT();
  return 0;
}

int parse_ip_data(modem_config *cfg, unsigned char *data, int len)
{
  // I'm going to cheat and assume it comes in chunks.
  int i = 0;
  int ch;
  char text[1025];
  int text_len = 0;


  if (cfg->line_data.first_char == TRUE) {
    cfg->line_data.first_char = FALSE;
    if ((data[0] == 0xff) || (data[0] == 0x1a)) {
      //line_write(cfg,(char*)TELNET_NOTICE,strlen(TELNET_NOTICE));
      LOG(LOG_INFO, "Detected telnet");
      cfg->line_data.is_telnet = TRUE;
    }
  }

  if (cfg->line_data.is_telnet == TRUE) {
    while (i < len) {
      ch = data[i];
      if (NVT_IAC == ch) {
        // what if we roll off the end?
        ch = data[i + 1];
        switch (ch) {
        case NVT_WILL:
        case NVT_DO:
        case NVT_WONT:
        case NVT_DONT:
          /// again, overflow issues...
          LOG(LOG_INFO, "Parsing nvt command");
          parse_nvt_command(cfg->line_data.fd, &cfg->line_data.nvt_data, ch,
			    data[i + 2], cfg->parity);
          i += 3;
          break;
        case NVT_SB:   // sub negotiation
          // again, overflow...
          i +=
            parse_nvt_subcommand(cfg->line_data.fd, &cfg->line_data.nvt_data,
				 data + i, len - i, cfg->dce_speed);
          break;
        case NVT_IAC:
          if (cfg->line_data.nvt_data.binary_recv)
            text[text_len++] = NVT_IAC;
          // fall through to skip this sequence
        default:
          // ignore...
          i += 2;
        }
      }
      else {
        text[text_len++] = data[i++];
      }
      if (text_len == 1024) {
        text[text_len] = 0;
        // write to serial...
        mdm_write(cfg, text, text_len);
        text_len = 0;
      }
    }
    if (text_len) {
      text[text_len] = 0;
      // write to serial...
      mdm_write(cfg, text, text_len);
    }
  }
  else {
    mdm_write(cfg, (char *) data, len);
  }
  return 0;
}

void *ip_thread(void *arg)
{
  modem_config *cfg = (modem_config *) arg;

  int action_pending = FALSE;
  fd_set readfs;
  int max_fd;
  int res = 0;
  char buf[256];
  int rc;


  LOG_ENTER();
  while (TRUE) {
    FD_ZERO(&readfs);
    FD_SET(cfg->data.cp[1][0], &readfs);
    max_fd = cfg->data.cp[1][0];
    if (action_pending == FALSE
        && cfg->conn_type != MDM_CONN_NONE
        && cfg->cmd_mode == FALSE
        && cfg->line_data.fd > -1 && cfg->line_data.valid_conn == TRUE) {
      FD_SET(cfg->line_data.fd, &readfs);
      max_fd = MAX(max_fd, cfg->line_data.fd);
    }
    max_fd++;
    rc = select(max_fd, &readfs, NULL, NULL, NULL);
    if (rc == -1) {
      ELOG(LOG_WARN, "Select returned error");
      // handle error
    }
    else {
      // we got data
      if (cfg->line_data.valid_conn == TRUE && FD_ISSET(cfg->line_data.fd, &readfs)) {  // socket
        LOG(LOG_DEBUG, "Data available on socket");
        res = recv(cfg->line_data.fd, buf, sizeof(buf), 0);
        if (0 >= res) {
          LOG(LOG_INFO, "No socket data read, assume closed peer");
          writePipe(cfg->data.cp[0][1], MSG_DISCONNECT);
          action_pending = TRUE;
        }
        else {
          LOG(LOG_DEBUG, "Read %d bytes from socket", res);
          buf[res] = 0;
          log_trace(TRACE_IP_IN, buf, res);
          parse_ip_data(cfg, (unsigned char *) buf, res);
        }
      }
      if (FD_ISSET(cfg->data.cp[1][0], &readfs)) {      // pipe

        res = read(cfg->data.cp[1][0], buf, sizeof(buf) - 1);
        LOG(LOG_DEBUG, "IP thread notified");
        action_pending = FALSE;
      }
    }
  }
  LOG_EXIT();
}

void *ctrl_thread(void *arg)
{
  modem_config *cfg = (modem_config *) arg;
  int status;
  int new_status;


  LOG_ENTER();

  status = dce_get_control_lines(cfg);
  while (status > -1) {
    new_status = dce_check_control_lines(cfg);
    if (new_status > -1 && status != new_status) {
      // something changed
      if ((status & MDM_CL_DTR_HIGH) != (new_status & MDM_CL_DTR_HIGH)) {
        if ((new_status & MDM_CL_DTR_HIGH) == 0) {
          LOG(LOG_INFO, "DTR has gone low");
          writePipe(cfg->data.wp[0][1], MSG_DTR_DOWN);
        }
        else {
          LOG(LOG_INFO, "DTR has gone high");
          writePipe(cfg->data.wp[0][1], MSG_DTR_UP);
        }
      }
    }
    status = new_status;
  }
  LOG_EXIT();
  // need to quit application, as status cannot be obtained.
  exit(-1);
}

int spawn_ctrl_thread(modem_config *cfg)
{
  int rc;
  pthread_t thread_id;

  rc = pthread_create(&thread_id, NULL, ctrl_thread, (void *) cfg);
  LOG(LOG_ALL, "CTRL thread ID=%d", (int) thread_id);

  if (rc < 0) {
    ELOG(LOG_FATAL, "CTRL thread could not be started");
    exit(-1);
  }
  return 0;
}

int spawn_ip_thread(modem_config *cfg)
{
  int rc;
  pthread_t thread_id;


  rc = pthread_create(&thread_id, NULL, ip_thread, (void *) cfg);
  LOG(LOG_ALL, "IP thread ID=%d", (int) thread_id);

  if (rc < 0) {
    ELOG(LOG_FATAL, "IP thread could not be started");
    exit(-1);
  }
  return 0;
}

void *run_bridge(void *arg)
{
  modem_config *cfg = (modem_config *) arg;
  struct timeval timer;
  struct timeval *ptimer;
  int max_fd = 0;
  fd_set readfs;
  int res = 0;
  char buf[256];
  int rc = 0;

  int last_conn_type;
  int last_cmd_mode = cfg->cmd_mode;


  LOG_ENTER();

  if (-1 == pipe(cfg->data.wp[0])) {
    ELOG(LOG_FATAL, "Control line watch task incoming IPC pipe could not be created");
    exit(-1);
  }
  if (-1 == pipe(cfg->data.cp[0])) {
    ELOG(LOG_FATAL, "IP thread incoming IPC pipe could not be created");
    exit(-1);
  }
  if (-1 == pipe(cfg->data.cp[1])) {
    ELOG(LOG_FATAL, "IP thread outgoing IPC pipe could not be created");
    exit(-1);
  }

  spawn_ctrl_thread(cfg);
  spawn_ip_thread(cfg);

  mdm_set_control_lines(cfg);
  strncpy(cfg->cur_line, cfg->config0, sizeof(cfg->cur_line));
  last_conn_type = cfg->conn_type;
  last_cmd_mode = cfg->cmd_mode;
  cfg->allow_transmit = FALSE;
  // call some functions behind the scenes
  mdm_disconnect(cfg);
  mdm_parse_cmd(cfg);
  // if direct connection, and num length > 0, dial number.
  if (cfg->data.direct_conn == TRUE) {
    if (strlen(cfg->data.direct_conn_num) > 0 && cfg->data.direct_conn_num[0] != ':') {
      // we have a direct number to connect to.
      strncpy(cfg->dialno, cfg->data.direct_conn_num, sizeof(cfg->dialno));
      if (0 != line_connect(cfg)) {
        LOG(LOG_FATAL, "Cannot connect to Direct line address!");
        // probably should exit...
        exit(-1);
      }
      else {
        cfg->conn_type = MDM_CONN_OUTGOING;
      }

    }
  }
  cfg->allow_transmit = TRUE;
  for (;;) {
    if (last_conn_type != cfg->conn_type) {
      LOG(LOG_ALL, "Connection status change, handling");
      //writePipe(cfg->data.mp[0][1],MSG_NOTIFY);
      writePipe(cfg->data.cp[1][1], MSG_NOTIFY);
      if (cfg->conn_type == MDM_CONN_OUTGOING) {
        if (strlen(cfg->data.local_connect) > 0) {
          writeFile(cfg->data.local_connect, cfg->line_data.fd);
        }
        if (strlen(cfg->data.remote_connect) > 0) {
          writeFile(cfg->data.remote_connect, cfg->line_data.fd);
        }
      }
      else if (cfg->conn_type == MDM_CONN_INCOMING) {
        if (strlen(cfg->data.local_answer) > 0) {
          writeFile(cfg->data.local_answer, cfg->line_data.fd);
        }
        if (strlen(cfg->data.remote_answer) > 0) {
          writeFile(cfg->data.remote_answer, cfg->line_data.fd);
        }
      }
      last_conn_type = cfg->conn_type;
    }
    if (last_cmd_mode != cfg->cmd_mode) {
      writePipe(cfg->data.cp[1][1], MSG_NOTIFY);
      last_cmd_mode = cfg->cmd_mode;
    }
    LOG(LOG_ALL, "Waiting for modem/control line/timer/socket activity");
    LOG(LOG_ALL, "Command Mode=%d, Connection status=%d", cfg->cmd_mode, cfg->conn_type);
    max_fd = MAX(cfg->data.mp[1][0], cfg->dce_data.fd);
    max_fd = MAX(max_fd, cfg->data.wp[0][0]);
    max_fd = MAX(max_fd, cfg->data.cp[0][0]);
    FD_ZERO(&readfs);
    FD_SET(cfg->data.mp[1][0], &readfs);
    FD_SET(cfg->dce_data.fd, &readfs);
    FD_SET(cfg->data.wp[0][0], &readfs);
    FD_SET(cfg->data.cp[0][0], &readfs);
    ptimer = NULL;
    if (cfg->cmd_mode == FALSE) {
      if (cfg->pre_break_delay == FALSE || cfg->break_len == 3) {
        LOG(LOG_ALL, "Setting timer for break delay");
        timer.tv_sec = 0;
        timer.tv_usec = cfg->s[SRegisterGuardTime] * 20000;
        ptimer = &timer;
      }
      else if (cfg->pre_break_delay == TRUE && cfg->break_len > 0) {
        LOG(LOG_ALL, "Setting timer for inter-break character delay");
        timer.tv_sec = 1;       // 1 second
        timer.tv_usec = 0;
        ptimer = &timer;
      }
      else if (cfg->s[30] != 0) {
        LOG(LOG_ALL, "Setting timer for inactivity delay");
        timer.tv_sec = cfg->s[30] * 10;
        timer.tv_usec = 0;
        ptimer = &timer;
      }
    }
    else if (cfg->cmd_mode == TRUE && cfg->conn_type == MDM_CONN_NONE
             && cfg->line_data.valid_conn == TRUE) {
      LOG(LOG_ALL, "Setting timer for rings");
      timer.tv_sec = 4;
      timer.tv_usec = 0;
      ptimer = &timer;
    }
    max_fd++;
    rc = select(max_fd, &readfs, NULL, NULL, ptimer);
    if (rc == -1) {
      ELOG(LOG_WARN, "Select returned error");
      // handle error
    }
    else if (rc == 0) {
      // timer popped.
      if (cfg->cmd_mode == TRUE && cfg->conn_type == MDM_CONN_NONE
          && cfg->line_data.valid_conn == TRUE) {
        if (cfg->s[0] == 0 && cfg->rings == 10) {
          // not going to answer, send some data back to IP and disconnect.
          if (strlen(cfg->data.no_answer) == 0) {
            line_write(cfg, (char *) MDM_NO_ANSWER, strlen(MDM_NO_ANSWER));
          }
          else {
            writeFile(cfg->data.no_answer, cfg->line_data.fd);
          }
          mdm_disconnect(cfg);
        }
        else
          mdm_send_ring(cfg);
      }
      else
        mdm_handle_timeout(cfg);
    }
    if (FD_ISSET(cfg->dce_data.fd, &readfs)) {  // serial port
      LOG(LOG_DEBUG, "Data available on serial port");
      res = dce_read(cfg, buf, sizeof(buf));
      LOG(LOG_DEBUG, "Read %d bytes from serial port", res);
      if (res > 0) {
        if (cfg->conn_type == MDM_CONN_NONE && cfg->off_hook == TRUE) {
          // this handles the case where atdt goes off hook, but no
          // connection
          mdm_disconnect(cfg);
          mdm_send_response(MDM_RESP_OK, cfg);
        }
        else {
          mdm_parse_data(cfg, buf, res);
        }
      }

    }
    if (FD_ISSET(cfg->data.wp[0][0], &readfs)) {        // control pipe
      res = read(cfg->data.wp[0][0], buf, sizeof(buf) - 1);
      buf[res] = 0;
      LOG(LOG_DEBUG, "Received %c from Control line watch task", buf[0]);
      switch (buf[0]) {
      case MSG_DTR_DOWN:
        // DTR drop, close any active connection and put
        // in cmd_mode
        mdm_disconnect(cfg);
        break;
      case MSG_DTR_UP:
        break;
      }
    }
    if (FD_ISSET(cfg->data.cp[0][0], &readfs)) {        // ip thread pipe
      res = read(cfg->data.cp[0][0], buf, sizeof(buf));
      LOG(LOG_DEBUG, "Received %c from ip thread", buf[0]);
      switch (buf[0]) {
      case MSG_DISCONNECT:
        if (cfg->data.direct_conn == TRUE) {
          // what should we do here...
          LOG(LOG_ERROR,
              "Direct Connection Link broken, disconnecting and awaiting new direct connection");
          cfg->data.direct_conn = FALSE;
          mdm_disconnect(cfg);
          cfg->data.direct_conn = TRUE;
        }
        else {
          mdm_disconnect(cfg);
        }
        break;
      }
    }
    if (FD_ISSET(cfg->data.mp[1][0], &readfs)) {        // parent pipe
      LOG(LOG_DEBUG, "Data available on incoming IPC pipe");
      res = read(cfg->data.mp[1][0], buf, sizeof(buf));
      switch (buf[0]) {
      case MSG_ACCEPT: // accept connection.
        accept_connection(cfg);
        break;
      }
    }
  }
  LOG_EXIT();
}
