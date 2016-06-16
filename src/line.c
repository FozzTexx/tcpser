#include "debug.h"
#include "modem_core.h"
#include "phone_book.h"
#include "ip.h"
#include "bridge.h"
#include "line.h"

int line_init_config(modem_config *cfg)
{
  cfg->line_data.fd = -1;
  cfg->line_data.is_telnet = FALSE;
  cfg->line_data.first_char = TRUE;
  cfg->line_data.valid_conn = FALSE;
  nvt_init_config(&cfg->line_data.nvt_data);
  return 0;
}

int line_write(modem_config *cfg, char *data, int len)
{
  int retval;
  int i = 0;
  int double_iac = FALSE;
  char text[1024];
  int text_len = 0;
  int mask = 0x7f;
  int ch;


  if (cfg->line_data.is_telnet || cfg->parity) {
    if (cfg->line_data.nvt_data.binary_xmit)
      mask = 0xff;
    
    retval = 0;
    while (i < len) {
      if (double_iac) {
        text[text_len++] = NVT_IAC;
        double_iac = FALSE;
        i++;
      }
      else {
	ch = data[i] & mask;
        if (NVT_IAC == ch) {
          text[text_len++] = NVT_IAC;
          double_iac = TRUE;
        }
        else {
          text[text_len++] = ch;
	  i++;
        }
      }
      if (text_len == sizeof(text_len)) {
        retval = ip_write(cfg->line_data.fd, text, text_len);
        text_len = 0;
      }
    }
    if (text_len) {
      retval = ip_write(cfg->line_data.fd, text, text_len);
    }
    return retval;
  }

  return ip_write(cfg->line_data.fd, data, len);
}

int line_listen(modem_config *cfg)
{
  return 0;
}

int line_off_hook(modem_config *cfg)
{
  return 0;
}

int line_connect(modem_config *cfg)
{
  char *addy = cfg->dialno;


  /* Reset everything we know about the line, it may not be the same as last time. */
  line_init_config(cfg);
  
  LOG(LOG_INFO, "Connecting");
  addy = pb_search(addy);
  cfg->line_data.fd = ip_connect(addy);
  if (cfg->line_data.fd > -1) {
    LOG(LOG_ALL, "Connected to %s", addy);
    cfg->line_data.valid_conn = TRUE;
    /* we need to let the other end know that our end will
     * handle the echo - otherwise "true" telnet clients like
     * those that come with Linux & Windows will echo characters
     * typed and you'll end up with doubled characters if the remote
     * host is echoing as well...
     * - gwb
     */
    send_nvt_command(cfg->line_data.fd, &cfg->line_data.nvt_data, NVT_WILL, NVT_OPT_ECHO);

    /* If space parity is detected treat it as 8 bit and try to enable binary mode */
    if (!cfg->parity) {
      send_nvt_command(cfg->line_data.fd, &cfg->line_data.nvt_data,
		       NVT_WILL, NVT_OPT_TRANSMIT_BINARY);
      send_nvt_command(cfg->line_data.fd, &cfg->line_data.nvt_data,
		       NVT_DO, NVT_OPT_TRANSMIT_BINARY);
    }
    return 0;
  }
  else {
    LOG(LOG_ALL, "Could not connect to %s", addy);
    cfg->line_data.valid_conn = FALSE;
    return -1;
  }
}

int line_disconnect(modem_config *cfg)
{
  LOG(LOG_INFO, "Disconnecting");
  if (cfg->data.direct_conn == TRUE) {
    LOG(LOG_INFO, "Direct connection active, maintaining link");
    return -1;
  }
  else {
    cfg->line_data.is_telnet = FALSE;
    cfg->line_data.first_char = TRUE;
    if (cfg->line_data.valid_conn == TRUE) {
      ip_disconnect(cfg->line_data.fd);
      cfg->line_data.valid_conn = FALSE;
    }
  }
  return 0;
}
