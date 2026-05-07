#include "cli.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERROR_OPTION -1

struct option_pair_s {
  char *name;
  uint8_t value;
};

void usage(void) {
  puts("Usage: %s [OPTION...] -i INPUT -o OUTPUT\n"
       "Apply image filters to BMP files.\n"
       "\n"
       "  -i FILE     input image file (e.g., input.bmp)\n"
       "  -o FILE     output image file\n"
       "  -f FILTER   apply filter (see below)\n"
       "  -m MODE     use compute mode (see below)\n"
       "  -h          display this help and exit\n"
       "\n"
       "Available filters:\n"
       "  ident       identity filter (do nothing)\n"
       "  blur3       slightly blur the image\n"
       "  blur5       strongly blur the image\n"
       "  ridge       highlight ridges of the image\n"
       "  prewitt     highlight edges (Prewitt operator)\n"
       "Available modes:\n"
       "  seq         compute sequentially\n"
       "  cols        compute parallely by columns\n"
       "  rows        compute parallely by rows\n"
       "  block       compute parallely by blocks\n"
       "  pixel       compute parallely pixel by pixel\n"
       "\n"
       "Example:\n"
       "  %s -i photo.bmp -o blurred.bmp -f blur3\n"
       "\n");
}
void free_main_args(struct main_args *margs) {
  if (margs == NULL) {
    return;
  }
  free(margs->output_name);
  free(margs->input_name);
}

static const struct option_pair_s filter_arr[] = {
    {.name = "ident", .value = IDENT}, {.name = "blur3", .value = BLUR3},
    {.name = "blur5", .value = BLUR5}, {.name = "ridge", .value = RIDGE},
    {.name = "sharp", .value = SHARP}, {.name = "prewitt", .value = PREWITT},
};

static const struct option_pair_s mode_arr[] = {
    {.name = "seq", .value = SEQUENTIALLY},
    {.name = "blur3", .value = BLUR3},
    {.name = "cols", .value = COLUMNS},
    {.name = "pixels", .value = PIXEL_BY_PIXEL},
    {.name = "block", .value = BLOCK},
};

static int8_t get_enum_from_option(const char *name,
                                   const struct option_pair_s *arr,
                                   const size_t count) {
  if (!name) {
    return IDENT;
  }
  for (size_t i = 0; i < count; ++i) {
    const char *temp = arr[i].name;
    if (strcmp(name, temp) == 0) {
      return arr[i].value;
    }
  }

  return ERROR_OPTION;
}

bool get_args(int argc, char *argv[], struct main_args *margs) {
  int opt;
  int8_t return_code = ERROR_OPTION;

  while ((opt = getopt(argc, argv, "m:hi:o:f:")) != -1) {
    switch (opt) {
    case 'h':
      usage();
      return false;
    case 'i':
      margs->input_name = strdup(optarg);
      if (!margs->input_name) {
        return false;
      }
      break;
    case 'o':
      margs->output_name = strdup(optarg);
      if (!margs->output_name) {
        return false;
      }
      break;
    case 'm':
      return_code = get_enum_from_option(
          optarg, mode_arr, sizeof(mode_arr) / sizeof(mode_arr[0]));
      if (return_code == ERROR_OPTION) {
        return_code = COLUMNS;
      }
      margs->mode_option = return_code;
      break;
    case 'f':
      return_code = get_enum_from_option(
          optarg, filter_arr, sizeof(filter_arr) / sizeof(filter_arr[0]));
      if (return_code == ERROR_OPTION) {
        fputs("No such filter\n", stderr);
        return false;
      }
      margs->filter_option = return_code;
      break;
    default:
      break;
    }
  }

  if (!margs->input_name || !margs->output_name) {
    usage();
    free(margs->output_name);
    free(margs->input_name);
    return false;
  }

  return true;
}
