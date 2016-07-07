#define MSG_NOT_ACTIVE 'a'
#define MSG_ACTIVE 'A'
#define MSG_NOT_LISTENING 'l'
#define MSG_LISTENING 'L'
#define MSG_ACCEPT '+'
#define MSG_ACCEPTED '+'
#define MSG_DTR_DOWN 'd'
#define MSG_DTR_UP 'D'
#define MSG_RTS_DOWN 'r'
#define MSG_RTS_UP 'R'
#define MSG_CD_DOWN 'c'
#define MSG_CD_UP 'c'
#define MSG_RNG_DOWN 'n'
#define MSG_RNG_UP 'N'
#define MSG_DISCONNECT 'D'
#define MSG_NOTIFY 'N'

int accept_connection(modem_config *);
int parse_ip_data(modem_config *cfg, unsigned char *data, int len);
void *run_bridge(void *arg);
