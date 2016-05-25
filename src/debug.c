#include <stdlib.h>     // for exit...
#include <time.h>
#include <pthread.h>
#include <stdio.h>
#define DEBUG_VARS 1    // need this so we don't get extern defs
#include "debug.h"

int log_level = 0;
FILE *log_file;
int trace_flags = 0;
char *trace_type[9];            // cheesy, but I can't think of another o(1) way
char *log_desc[LOG_TRACE + 1];
pthread_mutex_t log_mutex;

int log_init()
{
  log_file = stdout;
  log_level = 0;
  trace_flags = 0;
  trace_type[TRACE_MODEM_IN] = "RS<-";
  trace_type[TRACE_MODEM_OUT] = "RS->";
  trace_type[TRACE_IP_IN] = "IP<-";
  trace_type[TRACE_IP_OUT] = "IP->";
  log_desc[LOG_FATAL] = "FATAL";
  log_desc[LOG_ERROR] = "ERROR";
  log_desc[LOG_WARN] = "WARN";
  log_desc[LOG_INFO] = "INFO";
  log_desc[LOG_DEBUG] = "DEBUG";
  log_desc[LOG_ENTER_EXIT] = "ENTER_EXIT";
  log_desc[LOG_ALL] = "DEBUG_X";
  log_desc[LOG_TRACE] = "";
  if (-1 == pthread_mutex_init(&log_mutex, NULL)) {
    perror("Could not create Log Mutex");
    exit(-1);
  }
  return 0;
}

void log_set_file(FILE * a)
{
  log_file = a;
}

void log_set_level(int a)
{
  log_level = a;
}

void log_set_trace_flags(int a)
{
  trace_flags = a;
}

int log_get_trace_flags()
{
  return trace_flags;
}

void log_trace(int type, char *line, int len)
{
  int i = 0;
  int ch;
  char data[64] = "\0";
  char *dptr = NULL;
  char text[17];


  if (len == 0)
    return;

  if ((type & trace_flags) != 0) {
    text[16] = 0;
    for (i = 0; i < len; i++) {
      if ((i % 16) == 0) {
        // beginning of line
        dptr = data;
        sprintf(dptr, "%4.4x|", i);
      }
      ch = line[i];
      sprintf(dptr + 5 + ((i % 16) * 3), "%2.2x", ch);
      if (ch > 31 && ch < 127) {
        text[i % 16] = ch;
      }
      else {
        text[i % 16] = '.';
      }
      if ((i % 16) == 15) {
        log_start(LOG_TRACE);
        fprintf(log_file, "%s|%s|%s|", trace_type[type], data, text);
        log_end();
      }
      else {
        sprintf(dptr + 7 + ((i % 16) * 3), " ");
      }
    }
    i = i % 16;
    if (i > 0) {
      for (; i < 16; i++) {
        sprintf(dptr + 5 + ((i % 16) * 3), "  ");
        if ((i % 16) != 15) {
          sprintf(dptr + 7 + ((i % 16) * 3), " ");
        }
        text[i % 16] = ' ';
      }
      log_start(LOG_TRACE);
      fprintf(log_file, "%s|%s|%s|", trace_type[type], data, text);
    }
    log_end();
  }
}

void log_start(int level)
{
  char t[23];
  time_t now;


  if (-1 == pthread_mutex_lock(&log_mutex)) {
    perror("Could not lock the log mutex");
  }
  else {
    // we have the lock.
    now = time(NULL);
    strftime(t, 22, "%Y-%m-%d %H:%M:%S", localtime(&now));
    fprintf(log_file, "%s:%5.5d:%s:", t, (int) pthread_self(), log_desc[level]);
    //free(t);
  }
}

void log_end()
{
  fprintf(log_file, "\n");
  fflush(log_file);
  if (-1 == pthread_mutex_unlock(&log_mutex)) {
    perror("Could not lock the log mutex");
  }
}
