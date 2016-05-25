#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>     // for read...
#include <stdlib.h>     // for atoi...

#include "debug.h"
#include "ip.h"

const int BACK_LOG = 5;

int ip_init_server_conn(char *ip_addr, int port)
{
  int sSocket = 0, on = 0, rc = 0;
  struct sockaddr_in serverName = { 0 };


  LOG_ENTER();

  LOG(LOG_DEBUG, "Creating server socket");

  sSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (-1 == sSocket) {
    ELOG(LOG_FATAL, "Server socket could not be created");
  }
  else {

    /*
     * turn off bind address checking, and allow 
     * port numbers to be reused - otherwise
     * the TIME_WAIT phenomenon will prevent 
     * binding to these addresses.
     */

    on = 1;

    rc = setsockopt(sSocket, SOL_SOCKET, SO_REUSEADDR, (const char *) &on, sizeof(on)
      );
    if (-1 == rc) {
      ELOG(LOG_ERROR, "bind address checking could not be turned off");
    }

    if (ip_addr != NULL) /* gwb */
      serverName.sin_addr.s_addr = inet_addr(ip_addr);
    else 
      serverName.sin_addr.s_addr = htonl(INADDR_ANY);
    serverName.sin_family = AF_INET;

    /* network-order */
    serverName.sin_port = htons(port);
    if (ip_addr != NULL ) /* gwb */
      LOG(LOG_DEBUG, "Using specified ip address %s", ip_addr);

    LOG(LOG_DEBUG, "Binding server socket to port %d", port);
    rc = bind(sSocket, (struct sockaddr *) &serverName, sizeof(serverName)
      );
    if (-1 == rc) {
      ELOG(LOG_FATAL, "Server socket could not be bound to port");
      sSocket = -1;
    }
    else {
      LOG(LOG_INFO, "Server socket bound to port");

      rc = listen(sSocket, BACK_LOG);
      LOG(LOG_INFO, "Server socket listening for connections");
      if (-1 == rc) {
        ELOG(LOG_FATAL, "Server socket could not listen on port");
        sSocket = -1;
      }
    }
  }
  LOG_EXIT();
  return sSocket;
}

int ip_connect(char addy[])
{
  struct sockaddr_in pin;
  struct in_addr cin_addr;
  struct hostent *hp;
  int sd = 0;
  int port = 23;
  char *address;
  char *tmp;


  LOG_ENTER();

  address = (char *) strtok(addy, ":");
  tmp = (char *) strtok((char *) 0, ":");
  if (tmp != NULL && strlen(tmp) > 0) {
    port = atoi(tmp);
  }

  LOG(LOG_DEBUG, "Calling %s", addy);
  memset(&pin, 0, sizeof(pin));

  /* go find out about the desired host machine */
  if ((hp = gethostbyname(address)) == 0) {
    // well, not a DNS entry... Treat as IP...
    if (1 != inet_aton(address, &cin_addr)) {
      ELOG(LOG_ERROR, "Host %s was invalid", addy);
      return -1;
    }
  }
  else {
    cin_addr = *((struct in_addr *) (hp->h_addr));
  }

  pin.sin_family = AF_INET;
  pin.sin_addr.s_addr = cin_addr.s_addr;
  pin.sin_port = htons(port);

  /* grab an Internet domain socket */
  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    ELOG(LOG_ERROR, "could not create client socket");
    return -1;
  }

  /* connect to PORT on HOST */
  if (connect(sd, (struct sockaddr *) &pin, sizeof(pin)) == -1) {
    ELOG(LOG_ERROR, "could not connect to address");
    return -1;
  }
  LOG(LOG_INFO, "Connection to %s established", addy);
  LOG_EXIT();
  return sd;
}

int ip_accept(int sSocket)
{
  struct sockaddr_in clientName = { 0 };
  socklen_t clientLength = sizeof(clientName);
  int cSocket = -1;


  LOG_ENTER();

  (void) memset(&clientName, 0, sizeof(clientName));

  cSocket = accept(sSocket, (struct sockaddr *) &clientName, &clientLength);
  if (-1 == cSocket) {
    ELOG(LOG_ERROR, "Could not accept incoming connection");
    return -1;
  }

  if (-1 == getpeername(cSocket, (struct sockaddr *) &clientName, &clientLength)) {
    ELOG(LOG_WARN, "Could not obtain peer name");
  }
  else {
    LOG(LOG_INFO, "Connection accepted from %s", inet_ntoa(clientName.sin_addr)
      );
  }
  LOG_EXIT();
  return cSocket;
}

int ip_disconnect(int fd)
{
  if (fd > -1)
    close(fd);
  return 0;
}

int ip_write(int fd, char *data, int len)
{
  log_trace(TRACE_IP_OUT, data, len);
  return write(fd, data, len);
}

int ip_read(int fd, char *data, int len)
{
  int res;


  res = recv(fd, data, len, 0);
  log_trace(TRACE_IP_IN, data, res);
  return res;
}
