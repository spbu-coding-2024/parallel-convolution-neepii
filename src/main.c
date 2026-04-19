#include "cbmp.h"
#include "conv.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

void usage(void) {
  printf("USAGE:\n"
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

int main(int argc, char *argv[]) {
  int opt;
  bool parallelize = true;
  char *input_path = NULL;
  char *output_path = NULL;
  char *filter_name = NULL;

  while ((opt = getopt(argc, argv, "f:hi:o:s")) != -1) {
    switch (opt) {
    case 'h':
      usage();
      return EXIT_SUCCESS;
    case 'i':
      input_path = strdup(optarg);
      if (!input_path) {
        perror("strdup failed");
        return EXIT_FAILURE;
      }
      break;
    case 'o':
      output_path = strdup(optarg);
      if (!output_path) {
        perror("strdup failed");
        return EXIT_FAILURE;
      }
      break;
    case 's':
      parallelize = false;
      break;
    case 'f':
      filter_name = strdup(optarg);
      if (!filter_name) {
        perror("strdup failed");
        return EXIT_FAILURE;
      }
      break;
    }
  }

  if (!input_path || !output_path) {
    usage();
    return EXIT_FAILURE;
  }

  BMP *image = bopen(input_path);
  if (!image) {
    fprintf(stderr, "Bad input file\n");
    return EXIT_FAILURE;
  }

  KernelMatrix kernel_mtx = choose_kernel_matrix(filter_name);

  if (parallelize) {
    conv_apply_kernel_parallelly(image, kernel_mtx);
  } else {
    conv_apply_kernel_sequentialy(image, kernel_mtx);
  }

  bwrite(image, output_path);
  bclose(image);
  free(output_path);
  free(input_path);
  free_kernel_matrix(kernel_mtx);

  return EXIT_SUCCESS;
}
