#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "getcmd.h"

int getData(char line[], int *index, int len, int *data_start, int *data_end,
	    int complex_parse)
{
  int alpha = FALSE;
  int done = FALSE;


  *data_start = *index;

  while (*index < len && done != TRUE) {
    // I'm going to assume either 
    //    a number
    //    a string with a space
    switch (line[*index]) {
    case ' ':
      if (!complex_parse && *index != *data_start) {
        // leave space, next call will skip it.
        done = TRUE;
      }
      else if (*index != *data_start) {
        // we are complex, add the space and continue.
        (*index)++;
      }
      else {
        // we have not started, eat space and continue.
        (*index)++;
        *data_start = *index;
      }
      break;
    case 0:
      done = TRUE;
      break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':  // isnum
      (*index)++;
      break;
    default:
      if (!complex_parse && *index != *data_start && 0 == alpha) {
        // we were a number, but we've hit an alpha 'S0=137S...'
        done = TRUE;
      }
      else {
        (*index)++;
        alpha = TRUE;
      }
      break;
    }
  }
  *data_end = (*index);
  return 0;
}

int getNumber(char line[], int *index, int len)
{
  int num = 0;
  int found = FALSE;


  while (*index < len && 0 != isdigit(line[*index])) {
    num = num * 10 + line[(*index)++] - '0';
    found = 1;
  }
  if (FALSE == found)
    return -1;
  return num;
}

int skip(char line[], int *index, int len, char ch)
{
  while (*index < len && ch == line[*index])
    (*index)++;
  return 0;

}

int getCommand(char line[], int flags, int *index, int *num, int len)
{
  int cmd = line[(*index)++];


  *num = getNumber(line, index, len);
  return cmd;
}

int parseCommand(char line[], int flags, int *index, int *num, int len)
{
  int cmd = getCommand(line, flags, index, num, len);


  if (0 < cmd && 0 > *num)
    *num = 0;
  return toupper(cmd) | flags;
}

int parseRegister(char line[],
                  int flags,
                  int *index,
                  int *num, int len, int *data_start, int *data_end, int complex_parse)
{
  // need to handle S<num>?, which queries that S register.
  int cmd = 0;


  cmd = getCommand(line, flags, index, num, len);
  if (0 > num)
    return AT_CMD_ERR;
  skip(line, index, len, ' ');
  if (len == *index)
    return AT_CMD_ERR;
  switch (line[(*index)++]) {
  case '=':
    // set a register
    skip(line, index, len, ' ');
    if (0 > getData(line, index, len, data_start, data_end, complex_parse))
      return AT_CMD_ERR;
    break;
  case '?':
    // query a register
    flags |= AT_CMD_FLAG_QUERY;
    if (*num < 0)
      *num = 0;
    break;
  default:
    return AT_CMD_ERR;
  }

  return toupper(cmd) | flags;
}

int getcmd(char line[], int *index, int *num, int *data_start, int *data_end)
{
  int len = 0;
  int cmd = AT_CMD_END;


  *num = 0;
  *data_start = 0;
  *data_end = 0;

  if (line == NULL)
    return AT_CMD_NONE;
  len = strlen(line);
  while (*index < len) {
    cmd = toupper(line[*index]);
    switch (cmd) {
    case ' ':
      break;
    case 0:
      return AT_CMD_END;
    case '%':
      (*index)++;
      while (*index < len) {
        switch (toupper(line[*index])) {
        case ' ':
          break;
        case 0:
          return AT_CMD_ERR;
        default:
          return parseCommand(line, AT_CMD_FLAG_PRO_PCT, index, num, len);
        }
        (*index)++;
      }
      break;
    case '\\':
      (*index)++;
      while (*index < len) {
        switch (toupper(line[*index])) {
        case ' ':
          break;
        case 0:
          return AT_CMD_ERR;
        default:
          return parseCommand(line, AT_CMD_FLAG_PRO_BACK, index, num, len);
        }
        (*index)++;
      }
      break;
    case ':':
      (*index)++;
      while (*index < len) {
        switch (toupper(line[*index])) {
        case ' ':
          break;
        case 0:
          return AT_CMD_ERR;
        default:
          return parseCommand(line, AT_CMD_FLAG_PRO_COLON, index, num, len);
        }
        (*index)++;
      }
      break;
    case '-':
      (*index)++;
      while (*index < len) {
        switch (toupper(line[*index])) {
        case ' ':
          break;
        case 0:
          return AT_CMD_ERR;
        default:
          return parseCommand(line, AT_CMD_FLAG_PRO_MINUS, index, num, len);
        }
        (*index)++;
      }
      break;
    case '&':
      (*index)++;
      while (*index < len) {
        switch (toupper(line[*index])) {
        case ' ':
          break;
        case 0:
          return AT_CMD_ERR;
        case 'Z':
          return parseRegister(line, AT_CMD_FLAG_EXT, index, num, len, data_start, data_end,
                               TRUE);
        default:
          return parseCommand(line, AT_CMD_FLAG_EXT, index, num, len);
        }
        (*index)++;
      }
      break;
    case 'D':  // handle Dialing.
      (*index)++;
      *num = 0;
      while (*index < len) {
        switch (toupper(line[*index])) {
        case 0:
          return cmd;
        case 'T':
        case 'P':
        case 'L':
          *num = toupper(line[*index]);
          (*index)++;
        default:
          getData(line, index, len, data_start, data_end, TRUE);
          return cmd;
        }
        (*index)++;
      }
      return cmd;
    case 'S':
      return parseRegister(line, AT_CMD_FLAG_BAS, index, num, len, data_start, data_end, FALSE);
    default:
      return parseCommand(line, AT_CMD_FLAG_BAS, index, num, len);
    }
    (*index)++;
  }
  return cmd;

}

int main_getcmd(int argc, char **argv)
{
  char data[] = "DT 555-1212";
  int index = 0, num = 0, start = 0, end = 0;
  int cmd = 0;


  while (cmd != AT_CMD_END) {
    cmd = getcmd(data, &index, &num, &start, &end);
    printf("Cmd: %c Index: %d Num: %d Start: %d End: %d\n", cmd, index, num, start, end);
  }

  return 0;
}
