#ifndef LINE_H
#define LINE_H 1

int line_init(void);
int line_init_conn(modem_config *cfg);
int line_init_config(modem_config *cfg);
int line_read(modem_config *cfg, char *data, int len);
int line_write(modem_config *cfg, char *data, int len);
int line_listen(modem_config *cfg);
int line_off_hook(modem_config *cfg);
int line_connect(modem_config *cfg);
int line_disconnect(modem_config *cfg);

#endif
