#include "cli.h"
#include <stdlib.h>

void free_main_args(struct main_args *margs) {
  free(margs->filter_name);
  free(margs->output_name);
  free(margs->input_name);
}
