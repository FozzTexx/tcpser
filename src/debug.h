#ifndef DEBUG_H
#define DEBUG_H 1

#define LOG_TRACE 10
#define LOG_ALL 7
#define LOG_ENTER_EXIT 6
#define LOG_DEBUG 5
#define LOG_INFO 4
#define LOG_WARN 3
#define LOG_ERROR 2
#define LOG_FATAL 1
#define LOG_NONE 0

#define TRACE_MODEM_IN 1
#define TRACE_MODEM_OUT 2
#define TRACE_IP_IN 4
#define TRACE_IP_OUT 8

#include <stdio.h>      // needed for strerror
#include <string.h>     // needed for strerror
#include <errno.h>      // needed for errno

#if __STDC_VERSION__ < 199901L
#if __GNUC__ >= 2
#define __func__ __FUNCTION__
#else
#define __func__ "<unknown>"
#endif
#endif

#define LOG(a,args...) do { \
                         if(a <= log_level) { \
                           log_start(a); \
                           fprintf(log_file,args); \
                           log_end(); \
                         } \
                       } while(0)

#define ELOG(a,args...) do { \
                         if(a <= log_level) { \
                           log_start(a); \
                           fprintf(log_file,args); \
                           fprintf(log_file," (%s)\n",strerror(errno)); \
                           log_end(); \
                         } \
                       } while(0)

#define LOG_ENTER() LOG(LOG_ENTER_EXIT,"Entering %s function",__func__);
#define LOG_EXIT() LOG(LOG_ENTER_EXIT,"Exitting %s function",__func__);
int log_init(void);
void log_set_file(FILE * a);
void log_set_level(int a);
int log_get_trace_flags();
void log_set_trace_flags(int a);
void log_trace(int type, char *line, int len);
void log_start(int level);
void log_end();

#endif
#ifndef DEBUG_VARS
#define DEBUG_VARS 1

#include <stdio.h>

extern int log_level;
extern FILE *log_file;

#endif
