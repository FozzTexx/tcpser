#ifndef DCE_H
#define DCE_H 1

int dce_init(void);
int dce_init_config(modem_config *cfg);
int dce_init_conn(modem_config *cfg);
int dce_set_flow_control(modem_config *cfg, int opts);
int dce_set_control_lines(modem_config *cfg, int state);
int dce_get_control_lines(modem_config *cfg);
int dce_check_control_lines(modem_config *cfg);
int dce_read(modem_config *cfg, char *data, int len);
int dce_write(modem_config *cfg, char *data, int len);

//int dce_check_for_break(modem_config *cfg, char ch, int chars_left);

#endif
