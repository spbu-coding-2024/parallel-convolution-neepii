#ifndef _IMAGE_H
#define _IMAGE_H

#include <stdint.h>

typedef struct {
  uint16_t signature;
  uint32_t file_size;
  uint16_t reserved1;
  uint16_t reserved2;
  uint32_t pixel_offset;
} BMPFileHeader;

typedef struct {
  uint32_t header_size;
  int32_t width;
  int32_t height;
  uint16_t planes;
  uint16_t bits_per_pixel;
  uint32_t compression;
  uint32_t image_size;
  int32_t x_pixels_per_meter;
  int32_t y_pixels_per_meter;
  uint32_t colors_used;
  uint32_t colors_important;
} BMPInfoHeader;

typedef struct {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
} Pixel;

typedef struct {
  BMPFileHeader file_header;
  BMPInfoHeader info_header;
  Pixel *pixels;
} BMPImage;

BMPImage *bmp_load_image(const char *filename);
void bmp_save_image(BMPImage *image, const char *output_file);

void bmp_free_image(BMPImage *image);

#endif
