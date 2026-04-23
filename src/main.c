#include "cbmp.h"
#include "cli.h"
#include "conv.h"

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void usage(void) {
  puts("USAGE:\n"
       "\t-h    Display this message\n"
       "\t-i    Pass an input file (e.g. -i input.bmp)\n"
       "\t-o    Pass an output file\n"
       "\t-s    Run program sequentially\n"
       "\t-f    Choose a filter (e.g. -f blur3, see FILTER section)\n"
       "\nFILTERS:\n"
       "\tident - identity filter, do nothing\n"
       "\tblur3 - make image slightly blurry\n"
       "\tblur5 - make image more blurry\n"
       "\tridge - highlight ridges of image\n"

  );
}

static bool get_args(int argc, char *argv[], struct main_args *margs) {
  int opt;

  while ((opt = getopt(argc, argv, "shi:o:f:")) != -1) {
    switch (opt) {
    case 'h':
      usage();
      free_main_args(margs);
      return false;
    case 'i':
      margs->input_name = strdup(optarg);
      if (!margs->input_name) {
        free_main_args(margs);
        return false;
      }
      break;
    case 'o':
      margs->output_name = strdup(optarg);
      if (!margs->output_name) {
        free_main_args(margs);
        return false;
      }
      break;
    case 's':
      margs->parallelize = false;
      break;
    case 'f':
      margs->filter_name = strdup(optarg);
      if (!margs->filter_name) {
        free_main_args(margs);
        return false;
      }
      break;
    default:
      break;
    }
  }

  if (!margs->input_name || !margs->output_name || !margs->filter_name) {
    usage();
    free_main_args(margs);
    return false;
  }

  return true;
}

int main(int argc, char *argv[]) {
  struct main_args args = {
      .filter_name = NULL,
      .output_name = NULL,
      .input_name = NULL,
      .parallelize = true,
  };
  if (!get_args(argc, argv, &args)) {
    return EXITFAILURE;
  }

  BMP *image = bopen(args.input_name);
  if (!image) {
    fputs("Bad input file\n", stderr);
    return EXITFAILURE;
  }

  if (!apply_filter(image, args)) {
    return EXITFAILURE;
  }

  bwrite(image, args.output_name);
  bclose(image);
  free_main_args(&args);

  return EXITSUCCESS;
}
