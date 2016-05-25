#ifndef IP_H
#define IP_H

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

extern int ip_init(void);
extern int ip_init_server_conn(char *ip_addr, int port);
extern int ip_connect(char addy[]);
extern int ip_accept(int sSocket);
extern int ip_disconnect(int fd);
extern int ip_write(int fd, char *data, int len);
extern int ip_read(int fd, char *data, int len);

#endif
