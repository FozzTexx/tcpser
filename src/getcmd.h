#define AT_CMD_END -1
#define AT_CMD_ERR -2
#define AT_CMD_NONE -4
#define AT_CMD_IERR -3
#define AT_CMD_FLAG_BAS 0
#define AT_CMD_FLAG_EXT 256
#define AT_CMD_FLAG_PRO_PCT 512
#define AT_CMD_FLAG_PRO_BACK 1024
#define AT_CMD_FLAG_PRO_MINUS 2048
#define AT_CMD_FLAG_PRO_DOLLAR 4096
#define AT_CMD_FLAG_PRO_COLON 8192
#define AT_CMD_FLAG_QUERY 32768

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

int getData(char line[], int *index, int len, int *data_start, int *data_end, int simple_parse);
int getNumber(char line[], int *index, int len);
int skip(char line[], int *index, int len, char ch);
int getCommand(char line[], int flags, int *index, int *num, int len);
int parseCommand(char line[], int flags, int *index, int *num, int len);
int parseRegister(char line[],
                  int flags,
                  int *index,
                  int *num, int len, int *data_start, int *data_end, int simple_parse);
int getcmd(char line[], int *index, int *num, int *data_start, int *data_end);
