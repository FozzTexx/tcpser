#ifndef MODEM_CORE_H
#define MODEM_CORE_H 1

#define MDM_RESP_OK 0
#define MDM_RESP_CONNECT 1
#define MDM_RESP_RING 2
#define MDM_RESP_NO_CARRIER 3
#define MDM_RESP_ERROR 4
#define MDM_RESP_CONNECT_1200 5
#define MDM_RESP_NO_DIALTONE 6
#define MDM_RESP_BUSY 7
#define MDM_RESP_NO_ANSWER 8
#define MDM_RESP_CONNECT_0600 9
#define MDM_RESP_CONNECT_2400 10
#define MDM_RESP_CONNECT_4800 11
#define MDM_RESP_CONNECT_9600 12
#define MDM_RESP_CONNECT_7200 13
#define MDM_RESP_CONNECT_12000 14
#define MDM_RESP_CONNECT_14400 15
#define MDM_RESP_CONNECT_19200 16
#define MDM_RESP_CONNECT_38400 17
#define MDM_RESP_CONNECT_57600 18
#define MDM_RESP_CONNECT_115200 19
#define MDM_RESP_CONNECT_234000 20

#define MDM_CL_DSR_LOW 0
#define MDM_CL_DSR_HIGH 1
#define MDM_CL_DCD_LOW 0
#define MDM_CL_DCD_HIGH 2
#define MDM_CL_CTS_LOW 0
#define MDM_CL_CTS_HIGH 4
#define MDM_CL_DTR_LOW 0
#define MDM_CL_DTR_HIGH 8
#define MDM_FC_RTS 1
#define MDM_FC_XON 2

#define MDM_CONN_NONE 0
#define MDM_CONN_OUTGOING 1
#define MDM_CONN_INCOMING 2

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#include "nvt.h"

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

typedef struct line_config {
  int valid_conn;
  int fd;
  int sfd;
  int is_telnet;
  int first_char;
  nvt_vars nvt_data;
} line_config;

typedef struct x_config {
  int mp[2][2];
  int cp[2][2];
  int wp[2][2];
  char no_answer[256];
  char local_connect[256];
  char remote_connect[256];
  char local_answer[256];
  char remote_answer[256];
  char inactive[256];
  unsigned int direct_conn;
  char direct_conn_num[256];
} x_config;

typedef struct dce_config {
  int is_ip232;
  char tty[256];
  int first_char;
  int fd;
  int dp[2][2];
  int sSocket;
  int ip232_is_connected;
  int ip232_dtr;
  int ip232_dcd;
  int ip232_iac;
} dce_config;

enum {
  SRegisterBreak = 2,
  SRegisterCR = 3,
  SRegisterLF = 4,
  SRegisterBackspace = 5,
  SRegisterBlindWait = 6,
  SRegisterCarrierWait = 7,
  SRegisterCommaPause = 8,
  SRegisterCarrierTime = 9,
  SRegisterCarrierLoss = 10,
  SRegisterDTMFTime = 11,
  SRegisterGuardTime = 12,
};

typedef struct modem_config {
  // master configuration information

  // need to eventually change these
  dce_config dce_data;
  line_config line_data;
  x_config data;
  char config0[1024];
  char config1[1024];
  int dce_speed;
  int dte_speed;
  int conn_type;
  int line_ringing;
  int off_hook;
  int dsr_active;
  int dsr_on;
  int dcd_on;
  int invert_dsr;
  int invert_dcd;
  int allow_transmit;
  int rings;
  // command information
  int pre_break_delay;
  int found_a;
  int cmd_started;
  int cmd_mode;
  char last_cmd[1024];
  char cur_line[1024];
  int cur_line_idx;
  // dailing information
  char dialno[256];
  char last_dialno[256];
  char dial_type;
  char last_dial_type;
  int memory_dial;
  // modem config
  int connect_response;
  int response_code_level;
  int send_responses;
  int text_responses;
  int echo;
  int s[100];
  int break_len;
  int disconnect_delay;
  char crlf[3];
  int parity;
  unsigned char pchars[3];
} modem_config;

int mdm_init(void);
void mdm_init_config(modem_config *cfg);
int get_new_cts_state(modem_config *cfg, int up);
int get_new_dsr_state(modem_config *cfg, int up);
int get_new_dcd_state(modem_config *cfg, int up);
int mdm_set_control_lines(modem_config *cfg);
void mdm_write(modem_config *cfg, char data[], int len);
void mdm_send_response(int msg, modem_config *cfg);
int mdm_off_hook(modem_config *cfg);
int mdm_answer(modem_config *cfg);
int mdm_print_speed(modem_config *cfg);
int mdm_connect(modem_config *cfg);
int mdm_listen(modem_config *cfg);
int mdm_disconnect(modem_config *cfg);
int mdm_parse_cmd(modem_config *cfg);
int mdm_handle_char(modem_config *cfg, char ch);
int mdm_clear_break(modem_config *cfg);
int mdm_parse_data(modem_config *cfg, char *data, int len);
int mdm_handle_timeout(modem_config *cfg);
int mdm_send_ring(modem_config *cfg);

#include "line.h"
#include "shared.h"
#include "dce.h"

#endif
