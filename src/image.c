#include "image.h"
#include <stdio.h>
#include <stdlib.h>

// TODO: endianness awareness

BMPImage *bmp_load_image(const char *filename) {
  FILE *file = fopen(filename, "rb");
  if (!file) {
    perror("Error opening file");
    return NULL;
  }

  BMPImage *image = (BMPImage *)malloc(sizeof(BMPImage));
  if (!image) {
    fclose(file);
    return NULL;
  }

  fread(&image->file_header.signature, 2, 1, file);
  fread(&image->file_header.file_size, 4, 1, file);
  fread(&image->file_header.reserved1, 2, 1, file);
  fread(&image->file_header.reserved2, 2, 1, file);
  fread(&image->file_header.pixel_offset, 4, 1, file);
  fread(&image->info_header.header_size, 4, 1, file);
  fread(&image->info_header.width, 4, 1, file);
  fread(&image->info_header.height, 4, 1, file);
  fread(&image->info_header.planes, 2, 1, file);
  fread(&image->info_header.bits_per_pixel, 2, 1, file);
  fread(&image->info_header.compression, 4, 1, file);
  fread(&image->info_header.image_size, 4, 1, file);
  fread(&image->info_header.x_pixels_per_meter, 4, 1, file);
  fread(&image->info_header.y_pixels_per_meter, 4, 1, file);
  fread(&image->info_header.colors_used, 4, 1, file);
  fread(&image->info_header.colors_important, 4, 1, file);

  if (image->info_header.bits_per_pixel != 24) {
    perror("Non 24 bit bmp file is not supported");
    free(image);
    fclose(file);
    return NULL;
  }

  uint32_t pixel_count = image->info_header.width * image->info_header.height;

  image->pixels = (Pixel *)malloc(pixel_count * sizeof(Pixel));
  if (!image->pixels) {
    free(image);
    fclose(file);
    return NULL;
  }

  fseek(file, image->file_header.pixel_offset, SEEK_SET);
  fread(image->pixels, sizeof(Pixel), pixel_count, file);

  fclose(file);
  return image;
}

void bmp_save_image(BMPImage *image, const char *output_file) {
  if (!image || !output_file) {
    fprintf(stderr, "Invalid image or output file\n");
    return;
  }

  FILE *file = fopen(output_file, "wb");
  if (!file) {
    perror("Error opening output file");
    return;
  }

  fwrite(&image->file_header.signature, 2, 1, file);
  fwrite(&image->file_header.file_size, 4, 1, file);
  fwrite(&image->file_header.reserved1, 2, 1, file);
  fwrite(&image->file_header.reserved2, 2, 1, file);
  fwrite(&image->file_header.pixel_offset, 4, 1, file);

  fwrite(&image->info_header.header_size, 4, 1, file);
  fwrite(&image->info_header.width, 4, 1, file);
  fwrite(&image->info_header.height, 4, 1, file);
  fwrite(&image->info_header.planes, 2, 1, file);
  fwrite(&image->info_header.bits_per_pixel, 2, 1, file);
  fwrite(&image->info_header.compression, 4, 1, file);
  fwrite(&image->info_header.image_size, 4, 1, file);
  fwrite(&image->info_header.x_pixels_per_meter, 4, 1, file);
  fwrite(&image->info_header.y_pixels_per_meter, 4, 1, file);
  fwrite(&image->info_header.colors_used, 4, 1, file);
  fwrite(&image->info_header.colors_important, 4, 1, file);

  const uint32_t pixel_count =
      image->info_header.width * image->info_header.height;

  fseek(file, image->file_header.pixel_offset, SEEK_SET);
  fwrite(image->pixels, sizeof(Pixel), pixel_count, file);

  fclose(file);
}

void bmp_free_image(BMPImage *image) {
  if (image) {
    free(image->pixels);
    free(image);
  }
}
