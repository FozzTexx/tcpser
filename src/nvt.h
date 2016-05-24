#ifndef NVT_H
#define NVT_H 1

#define NVT_SE 240
#define NVT_NOP 241
#define NVT_DM 242
#define NVT_SB 250
#define NVT_WILL 251
#define NVT_WONT 252
#define NVT_DO 253
#define NVT_DONT 254
#define NVT_IAC 255
#define NVT_WILL_RESP 251
#define NVT_WONT_RESP 252
#define NVT_DO_RESP 253
#define NVT_DONT_RESP 254

#define NVT_OPT_TRANSMIT_BINARY 0
#define NVT_OPT_ECHO 1
#define NVT_OPT_SUPPRESS_GO_AHEAD 3
#define NVT_OPT_STATUS 5
#define NVT_OPT_RCTE 7
#define NVT_OPT_TIMING_MARK 6
#define NVT_OPT_NAOCRD 10
#define NVT_OPT_TERMINAL_TYPE 24
#define NVT_OPT_NAWS 31
#define NVT_OPT_TERMINAL_SPEED 32
#define NVT_OPT_LINEMODE 34
#define NVT_OPT_X_DISPLAY_LOCATION 35
#define NVT_OPT_ENVIRON 36
#define NVT_OPT_NEW_ENVIRON 39

#define NVT_SB_IS 0
#define NVT_SB_SEND 1

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

typedef struct nvt_vars {
  int binary_xmit;
  int binary_recv;
  char term[256];
} nvt_vars;


char get_nvt_cmd_response(char action, char type);
int parse_nvt_subcommand(int fd, nvt_vars *vars, char *data, int len);
int parse_nvt_command(int fd, nvt_vars *vars, char action, int opt);
int nvt_init_config(nvt_vars *vars);
int send_nvt_command(int fd, nvt_vars *vars, char action, int opt);

#endif
