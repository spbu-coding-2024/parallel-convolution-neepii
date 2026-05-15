#ifndef CLI_CLI_H
#define CLI_CLI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum exit_status {
  EXITSUCCESS = 0,
  EXITFAILURE = 1,
};

struct main_args {
  char **arr_input;
  size_t num_of_inputs;
  uint8_t mode_option;
  uint8_t filter_option;
  bool use_pipeline;
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

void from_input_to_output(char *input, size_t output_len, char *buffer);
bool get_args(int argc, char *argv[], struct main_args *margs);
void free_main_args(struct main_args *margs);

#endif
