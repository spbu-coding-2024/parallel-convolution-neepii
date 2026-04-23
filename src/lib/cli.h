#ifndef CLI_CLI_H
#define CLI_CLI_H

#include <stdbool.h>

enum exit_status {
  EXITSUCCESS = 0,
  EXITFAILURE = 1,
};

struct main_args {
  char *filter_name;
  char *output_name;
  char *input_name;
  bool parallelize;
};

void free_main_args(struct main_args *margs);

#endif
