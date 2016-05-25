#ifndef IP232_H
#define IP232_H 1

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif /*  */

#define MSG_ACCEPT '+'
#define MSG_ACCEPTED '+'
int ip232_init_conn(modem_config *);
int ip232_set_flow_control(modem_config *, int status);
int ip232_get_control_lines(modem_config *);
int ip232_set_control_lines(modem_config *, int state);
int ip232_write(modem_config *, char *data, int len);
int ip232_read(modem_config *, char *data, int len);

#endif /*  */
