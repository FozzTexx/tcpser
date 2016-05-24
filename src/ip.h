#ifndef IP_H
#define IP_H

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

int ip_init(void);
int ip_init_server_conn(int port);
int ip_connect(char addy[]);
int ip_accept(int sSocket);
int ip_disconnect(int fd);
int ip_write(int fd, char *data, int len);
int ip_read(int fd, char *data, int len);

#endif
