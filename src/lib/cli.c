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

static void usage(void) {
  puts("Usage: %s [OPTION...] -o OUTPUT [INPUT FILES...]\n"
       "Apply image filters to BMP files.\n"
       "\n"
       "  -o FILE     output image file\n"
       "  -f FILTER   apply filter (see below)\n"
       "  -m MODE     use compute mode (see below)\n"
       "  -r          don't use pipeline\n"
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
       "EXAMPLE:\n"
       "  %s -o blurred.bmp -f blur3 photo.bmp");
}
void free_main_args(struct main_args *margs) {
  if (margs == NULL) {
    return;
  }
  free(margs->output_name);
  for (size_t i = 0; i < margs->num_of_inputs; ++i) {
    free(margs->arr_input[i]);
  }
  free(margs->arr_input);
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

void from_input_to_output(char *input, size_t output_len, char *buffer) {
  strncpy(buffer, input, output_len);
  buffer[output_len] = 'o';
  buffer[output_len + 1] = '\0';
}

bool get_args(int argc, char *argv[], struct main_args *margs) {
  int opt;
  int8_t return_code = ERROR_OPTION;

  margs->arr_input = malloc(sizeof(char *) * argc);
  if (!margs->arr_input) {
    return false;
  }

  while ((opt = getopt(argc, argv, "rm:ho:f:")) != -1) {
    switch (opt) {
    case 'h':
      usage();
      goto args_free;
    case 'r':
      margs->use_pipeline = false;
      break;
    case 'o':
      margs->output_name = strdup(optarg);
      if (!margs->output_name) {
        fputs("Can't allocate memory for output name\n", stderr);
        goto args_free;
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
        goto args_free;
      }
      margs->filter_option = return_code;
      break;
    default:
      break;
    }
  }

  for (int i = optind; i < argc; ++i) {
    margs->arr_input[margs->num_of_inputs] = strdup(argv[i]);
    if (!margs->arr_input[margs->num_of_inputs]) {
      return false;
    }
    margs->num_of_inputs++;
  }
  if (margs->num_of_inputs == 0 || !margs->output_name) {
    usage();
    goto args_free;
  }

  return true;

args_free:
  free(margs->output_name);
  for (size_t i = 0; i < margs->num_of_inputs; ++i) {
    free(margs->arr_input[i]);
  }
  free(margs->arr_input);
  return false;
}
