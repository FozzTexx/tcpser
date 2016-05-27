#include <sys/socket.h> // for recv...
#include <stdlib.h>     // for exit...
#include <sys/file.h>
#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <pthread.h>

#include "util.h"
#include "debug.h"
#include "modem_core.h"
#include "ip.h"
#include "ip232.h"

void *ip232_thread(void *arg)
{
  modem_config *cfg = (modem_config *) arg;
  int accept_pending = FALSE;
  int rc;
  int res = 0;
  char buf[256];

  fd_set readfs;
  int max_fd = 0;
  int cSocket;


  LOG_ENTER();
  for (;;) {
    FD_ZERO(&readfs);
    FD_SET(cfg->dce_data.dp[1][0], &readfs);
    max_fd = cfg->dce_data.dp[1][0];
    if (accept_pending == FALSE) {
      FD_SET(cfg->dce_data.sSocket, &readfs);
      max_fd = MAX(max_fd, cfg->dce_data.sSocket);
    }
    LOG(LOG_ALL, "Waiting for incoming ip232 connections");
    rc = select(max_fd + 1, &readfs, NULL, NULL, NULL);
    if (rc < 0) {

      // handle error
    }
    else {
      if (FD_ISSET(cfg->dce_data.dp[1][0], &readfs)) {  // pipe
        res = read(cfg->dce_data.dp[1][0], buf, sizeof(buf) - 1);
        LOG(LOG_DEBUG, "ip232 thread notified");
        accept_pending = FALSE;
      }
      if (FD_ISSET(cfg->dce_data.sSocket, &readfs)) {   // ip connection
        if (cfg->dce_data.ip232_is_connected) {
          LOG(LOG_DEBUG, "Already have ip232 connection, rejecting new");

          // already have a connection... accept and close
          cSocket = ip_accept(cfg->dce_data.sSocket);
          if (cSocket > -1) {
            close(cSocket);
          }
        }
        else {
          LOG(LOG_DEBUG, "Incoming ip232 connection");
          writePipe(cfg->dce_data.dp[0][1], MSG_ACCEPT);
          accept_pending = TRUE;
        }
      }
    }
  }
  LOG_EXIT();
}

int spawn_ip232_thread(modem_config *cfg)
{
  int rc;
  pthread_t thread_id;


  rc = pthread_create(&thread_id, NULL, ip232_thread, (void *) cfg);
  LOG(LOG_ALL, "ip232 thread ID=%d", (int) thread_id);
  if (rc < 0) {
    ELOG(LOG_FATAL, "ip232 thread could not be started");
    exit(-1);
  }
  return 0;
}

int ip232_init_conn(modem_config *cfg)
{
  int rc = -1;
  int port;


  LOG_ENTER();
  LOG(LOG_INFO, "Opening ip232 device");
  port = atoi(cfg->dce_data.tty);
  rc = ip_init_server_conn("", port);
  if (rc < 0) {
    ELOG(LOG_FATAL, "Could not initialize ip232 server socket");
    exit(-1);
  }
  if (-1 == pipe(cfg->dce_data.dp[0])) {
    ELOG(LOG_FATAL, "ip232 thread incoming IPC pipe could not be created");
    exit(-1);
  }
  if (-1 == pipe(cfg->dce_data.dp[1])) {
    ELOG(LOG_FATAL, "ip232 thread outgoing IPC pipe could not be created");
    exit(-1);
  }
  cfg->dce_data.sSocket = rc;
  cfg->dce_data.ip232_is_connected = FALSE;
  cfg->dce_data.fd = cfg->dce_data.dp[0][0];
  spawn_ip232_thread(cfg);
  LOG(LOG_INFO, "ip232 device configured");
  LOG_EXIT();
  return rc;
}

int ip232_set_flow_control(modem_config *cfg, int status)
{
  return 0;
}

int ip232_get_control_lines(modem_config *cfg)
{
  int status = 0;


  if (cfg->dce_data.ip232_is_connected && cfg->dce_data.ip232_dtr) {
    status |= TIOCM_DSR;
  }
  return status;
}

int ip232_set_control_lines(modem_config *cfg, int state)
{
  int dcd;
  char cmd[2];


  if (cfg->dce_data.ip232_is_connected) {
    dcd = (state & TIOCM_DTR) ? TRUE : FALSE;
    if (dcd != cfg->dce_data.ip232_dcd) {
      cfg->dce_data.ip232_dcd = dcd;
      cmd[0] = 255;
      cmd[1] = dcd ? 1 : 0;
      ip_write(cfg->dce_data.fd, cmd, sizeof(cmd));
    }
  }
  return 0;
}

int ip232_write(modem_config *cfg, char *data, int len)
{
  int retval;
  int i = 0;
  int double_iac = FALSE;
  char text[1024];
  int text_len = 0;


  log_trace(TRACE_MODEM_OUT, data, len);
  retval = len;
  if (cfg->dce_data.ip232_is_connected) {
    while (i < len) {
      if (double_iac) {
        text[text_len++] = 255;
        double_iac = FALSE;
        i++;
      }
      else {
        if (255 == data[i]) {
          text[text_len++] = 255;
          double_iac = TRUE;
        }
        else {
          text[text_len++] = data[i++];
        }
      }
      if (text_len == sizeof(text)) {
        retval = ip_write(cfg->dce_data.fd, text, text_len);
        text_len = 0;
      }
    }
    if (text_len) {
      retval = ip_write(cfg->dce_data.fd, text, text_len);
    }
  }
  return retval;
}

int ip232_read(modem_config *cfg, char *data, int len)
{
  int res;
  int rc;
  char buf[256];
  int i = 0;
  int ch;
  int text_len = 0;


  LOG_ENTER();
  if (len > sizeof(buf)) {
    LOG(LOG_FATAL, "ip232_read: len > sizeof(buf)");
    exit(-1);
  }
  if (cfg->dce_data.ip232_is_connected) {
    res = recv(cfg->dce_data.fd, buf, len, 0);
    if (0 >= res) {
      LOG(LOG_INFO, "No ip232 socket data read, assume closed peer");
      ip_disconnect(cfg->dce_data.fd);
      cfg->dce_data.fd = cfg->dce_data.dp[0][0];
      cfg->dce_data.ip232_is_connected = FALSE;
    }
    else {
      LOG(LOG_DEBUG, "Read %d bytes from ip232 socket", res);
      log_trace(TRACE_MODEM_IN, buf, res);
      while (i < res) {
        ch = buf[i];
        if (cfg->dce_data.ip232_iac) {
          cfg->dce_data.ip232_iac = FALSE;
          switch (ch) {
          case 0:
            cfg->dce_data.ip232_dtr = FALSE;
            break;
          case 1:
            cfg->dce_data.ip232_dtr = TRUE;
            break;
          case 255:
            data[text_len++] = 255;
            break;
          }
        }
        else {
          if (255 == ch) {
            cfg->dce_data.ip232_iac = TRUE;
          }
          else {
            data[text_len++] = ch;
          }
        }
        i++;
      }
    }
  }
  else {
    // not connected
    res = read(cfg->dce_data.dp[0][0], buf, sizeof(buf));
    switch (buf[0]) {
    case MSG_ACCEPT:   // accept connection.
      LOG(LOG_INFO, "Accepting ip232 connection...");
      rc = ip_accept(cfg->dce_data.sSocket);
      if (res > -1) {
        cfg->dce_data.fd = rc;
        cfg->dce_data.ip232_is_connected = TRUE;
        cfg->dce_data.ip232_dtr = FALSE;
        cfg->dce_data.ip232_dcd = FALSE;
        writePipe(cfg->dce_data.dp[1][1], MSG_ACCEPTED);
      }
      break;
    }
  }

  LOG_EXIT();
  return text_len;
}
