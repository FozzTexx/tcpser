#include <sys/file.h>
#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "debug.h"
#include "serial.h"

int ser_get_bps_const(int speed)
{
  int bps_rate = 0;

  LOG_ENTER();

  LOG(LOG_DEBUG, "Checking speed: %d", speed);

  switch (speed) {
  case 115200:
    bps_rate = B115200;
    break;
  case 57600:
    bps_rate = B57600;
    break;
  case 38400:
    bps_rate = B38400;
    break;
  case 19200:
    bps_rate = B19200;
    break;
  case 9600:
    bps_rate = B9600;
    break;
  case 4800:
    bps_rate = B4800;
    break;
  case 2400:
    bps_rate = B2400;
    break;
  case 1200:
    bps_rate = B1200;
    break;
  case 600:
    bps_rate = B600;
    break;
  case 300:
    bps_rate = B300;
    break;
  case 150:
    bps_rate = B150;
    break;
  case 134:
    bps_rate = B134;
    break;
  case 110:
    bps_rate = B110;
    break;
  case 75:
    bps_rate = B75;
    break;
  case 50:
    bps_rate = B50;
    break;
  case 0:
    bps_rate = B0;
    break;
  default:
    ELOG(LOG_FATAL, "Unknown baud rate");
    bps_rate = -1;
  }
  LOG_EXIT();
  return bps_rate;

}

int ser_init_conn(char *tty, int speed)
{
  int fd = -1;
  struct termios tio;
  int bps_rate = 0;

  LOG_ENTER();

  bps_rate = ser_get_bps_const(speed);

  if (bps_rate > -1) {
    /* open the device to be non-blocking (read will return immediatly) */
    LOG(LOG_INFO, "Opening serial device");

    fd = open(tty, O_RDWR | O_NOCTTY);

    if (fd < 0) {
      ELOG(LOG_FATAL, "TTY %s could not be opened", tty);
    }
    else {
      LOG(LOG_INFO, "Opened serial device %s at speed %d as fd %d", tty, speed, fd);

      /* Make the file descriptor asynchronous (the manual page says only 
         O_APPEND and O_NONBLOCK, will work with F_SETFL...) */
      fcntl(fd, F_SETFL, FASYNC);
      
      tio.c_cflag = CS8 | CLOCAL | CREAD | CRTSCTS;
      tio.c_iflag = IGNBRK;
      tio.c_oflag = 0;
      tio.c_lflag = 0;
      cfsetispeed(&tio, bps_rate);
      cfsetospeed(&tio, bps_rate);

      tio.c_cc[VMIN] = 1;
      tio.c_cc[VTIME] = 0;

      tcflush(fd, TCIFLUSH);
      tcsetattr(fd, TCSANOW, &tio);
      LOG(LOG_INFO, "serial device configured");
    }
  }

  LOG_EXIT();
  return fd;
}

int ser_set_flow_control(int fd, int status)
{
  struct termios tio;

  if (0 != tcgetattr(fd, &tio)) {
    ELOG(LOG_FATAL, "Could not get serial port attributes");
    return -1;
  }
  // turn all off.
  tio.c_cflag &= ~(IXON | IXOFF | CRTSCTS);
  tio.c_cflag |= status;
  if (0 != tcsetattr(fd, TCSANOW, &tio)) {
    ELOG(LOG_FATAL, "Could not set serial port attributes");
    return -1;
  }
  return 0;
}

int ser_get_control_lines(int fd)
{
  int status;


  if (0 > ioctl(fd, TIOCMGET, &status)) {
    ELOG(LOG_FATAL, "Could not obtain serial port status");
    return -1;
  }
  return status;
}

int ser_set_control_lines(int fd, int state)
{
  unsigned int status;


  if (0 > (status = ser_get_control_lines(fd))) {
    return status;
  }
  status &= ~(TIOCM_RTS | TIOCM_DTR);
  status |= state;
  if (0 > ioctl(fd, TIOCMSET, &status)) {
#ifndef WIN32
    ELOG(LOG_FATAL, "Could not set serial port status");
    return -1;
#else
    ELOG(LOG_WARN, "Could not set serial port status, CYGWIN bug?");
#endif
  }
  return 0;
}

int ser_write(int fd, char *data, int len)
{
  log_trace(TRACE_MODEM_OUT, data, len);
  return write(fd, data, len);
}

int ser_read(int fd, char *data, int len)
{
  int res;

  res = read(fd, data, len);
  log_trace(TRACE_MODEM_IN, data, res);
  return res;
}
