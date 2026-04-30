#ifndef CLI_CLI_H
#define CLI_CLI_H

#include <stdbool.h>
#include <stdint.h>

enum exit_status {
  EXITSUCCESS = 0,
  EXITFAILURE = 1,
};

struct main_args {
  char *output_name;
  char *input_name;
  uint8_t mode_option;
  uint8_t filter_option;
};

enum {
  RIDGE,
  BLUR3,
  BLUR5,
  PREWITT,
  SHARP,
  IDENT,
};

enum {
  SEQUENTIALLY,
  COLUMNS,
  ROWS,
  PIXEL_BY_PIXEL,
  BLOCK,
};

bool get_args(int argc, char *argv[], struct main_args *margs);
void free_main_args(struct main_args *margs);

#endif
