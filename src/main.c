#include "cbmp.h"
#include "cli.h"
#include "conv.h"

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  struct main_args args = {
      .output_name = NULL,
      .input_name = NULL,
      .mode_option = COLUMNS,
      .filter_option = IDENT,
  };
  if (!get_args(argc, argv, &args)) {
    return EXITFAILURE;
  }

  BMP *image = bopen(args.input_name);
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
  free_main_args(&args);

  return EXITSUCCESS;
}
