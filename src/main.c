#include "cbmp.h"
#include "conv.h"

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
         "\t-o    Pass output file\n");
}

int main(int argc, char *argv[]) {
  int opt;
  char *input_path = NULL;
  char *output_path = NULL;

  while ((opt = getopt(argc, argv, "hi:o:")) != -1) {
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

  /* conv_apply_kernel_sequentialy(image, ker_gauss_blur()); */
  /* conv_apply_kernel_sequentialy(image, ker_identity()); */
  conv_apply_kernel_sequentialy(image, ker_ridge());
  bwrite(image, output_path);
  bclose(image);

  return EXIT_SUCCESS;
}
