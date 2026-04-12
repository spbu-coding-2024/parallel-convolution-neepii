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
  printf("usage:\n"
         "\t-h    Display this message\n"
         "\t-i    Pass input file\n"
         "\t-o    Pass output file\n"
         "\t-s    Run program sequentially\n");
}

int main(int argc, char *argv[]) {
  int opt;
  bool parallelize = true;
  char *input_path = NULL;
  char *output_path = NULL;

  while ((opt = getopt(argc, argv, "hi:o:s")) != -1) {
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
  free(input_path);
  /* KernelMatrix kernel_mtx = ker_ridge(); */
  /* KernelMatrix kernel_mtx = ker_3x3_gauss_blur(); */
  KernelMatrix kernel_mtx = ker_5x5_gauss_blur();
  /* KernelMatrix kernel_mtx = ker_identity(); */
  if (parallelize) {

  } else {
    conv_apply_kernel_sequentialy(image, kernel_mtx);
  }

  bwrite(image, output_path);
  bclose(image);
  free_kernel_matrix(kernel_mtx);

  return EXIT_SUCCESS;
}
