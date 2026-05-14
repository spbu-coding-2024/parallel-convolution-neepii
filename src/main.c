#include "cbmp/cbmp.h"
#include "cli.h"
#include "conv.h"
#include "conv_pipeline.h"

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  struct main_args args = {
      .output_name = NULL, // TODO: REMOVE
      .arr_input = NULL,
      .mode_option = COLUMNS,
      .filter_option = IDENT,
      .use_pipeline = true,
      .num_of_inputs = 0,
  };
  if (!get_args(argc, argv, &args)) {
    return EXITFAILURE;
  }

  if (args.use_pipeline) {
    if (!apply_filter_pipeline(&args)) {
      fputs("Cannot apply filter\n", stderr);
      free_main_args(&args);
      return EXITFAILURE;
    }
  } else {
    for (size_t i = 0; i < args.num_of_inputs; ++i) {
      BMP *image = bopen(args.arr_input[i]);
      if (!image) {
        fputs("Bad input file\n", stderr);
        free_main_args(&args);
        return EXITFAILURE;
      }
      if (!apply_filter(image, args)) {
        fputs("Cannot apply filter\n", stderr);
        free_main_args(&args);
        return EXITFAILURE;
      }
      bwrite(image, args.output_name);
      bclose(image);
    }
  }

  free_main_args(&args);

  return EXITSUCCESS;
}
