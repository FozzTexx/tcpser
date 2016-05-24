#include "modem_core.h"

int sh_init_config(modem_config *cfg)
{
  cfg->data.local_connect[0] = 0;
  cfg->data.remote_connect[0] = 0;
  cfg->data.local_answer[0] = 0;
  cfg->data.remote_answer[0] = 0;
  cfg->data.no_answer[0] = 0;
  cfg->data.inactive[0] = 0;
  cfg->data.direct_conn = FALSE;
  cfg->data.direct_conn_num[0] = 0;
  return 0;
}
