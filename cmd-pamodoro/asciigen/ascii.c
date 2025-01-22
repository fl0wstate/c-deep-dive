#include <png.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

char arr[] = {' ', '`', '.', ',', '*', '~', '+', '&', '#', '@'};

int main()
{
  // Open the image file
  FILE *file = fopen("./flow.png", "rb");
  if (!file)
  {
    perror("Failed to open file");
    return 1;
  }

  // Check PNG signature
  unsigned char header[8];
  if (fread(header, 1, 8, file) != 8 || png_sig_cmp(header, 0, 8))
  {
    fprintf(stderr, "Invalid PNG file\n");
    fclose(file);
    return 1;
  }

  // Initialize PNG structs
  png_structp png =
      png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png)
  {
    fclose(file);
    return 1;
  }

  png_infop info = png_create_info_struct(png);
  if (!info)
  {
    png_destroy_read_struct(&png, NULL, NULL);
    fclose(file);
    return 1;
  }

  // Error handling
  if (setjmp(png_jmpbuf(png)))
  {
    png_destroy_read_struct(&png, &info, NULL);
    fclose(file);
    return 1;
  }

  // Initialize PNG I/O
  png_init_io(png, file);
  png_set_sig_bytes(png, 8);

  // Read PNG info
  png_read_info(png, info);

  // Get image dimensions and color type
  int orig_width = png_get_image_width(png, info);
  int orig_height = png_get_image_height(png, info);
  png_byte color_type = png_get_color_type(png, info);
  png_byte bit_depth = png_get_bit_depth(png, info);

  // Transform PNG to RGB format if necessary
  if (color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png);
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_expand_gray_1_2_4_to_8(png);
  if (png_get_valid(png, info, PNG_INFO_tRNS))
    png_set_tRNS_to_alpha(png);
  if (bit_depth == 16)
    png_set_strip_16(png);
  if (color_type & PNG_COLOR_MASK_ALPHA)
    png_set_strip_alpha(png);

  // Update info structure
  png_read_update_info(png, info);

  // Calculate scaling factors
  int scale_x = orig_width / 110;
  int scale_y = orig_height / 55;
  if (scale_x < 1)
    scale_x = 1;
  if (scale_y < 1)
    scale_y = 1;

  // Calculate new dimensions
  int new_width = orig_width / scale_x;
  int new_height = orig_height / scale_y;

  // Allocate memory for image data
  png_bytep *row_pointers =
      (png_bytep *)malloc(sizeof(png_bytep) * orig_height);
  for (int y = 0; y < orig_height; y++)
  {
    row_pointers[y] = (png_byte *)malloc(png_get_rowbytes(png, info));
  }

  // Read image data
  png_read_image(png, row_pointers);

  // Process and print ASCII art
  for (int y = 0; y < new_height; y++)
  {
    for (int x = 0; x < new_width; x++)
    {
      // Calculate average RGB values for the scaled pixel
      uint32_t r_sum = 0, g_sum = 0, b_sum = 0;
      int samples = 0;

      for (int sy = 0; sy < scale_y && y * scale_y + sy < orig_height; sy++)
      {
        for (int sx = 0; sx < scale_x && x * scale_x + sx < orig_width; sx++)
        {
          png_bytep px =
              &(row_pointers[y * scale_y + sy][(x * scale_x + sx) * 3]);
          r_sum += px[0];
          g_sum += px[1];
          b_sum += px[2];
          samples++;
        }
      }

      uint32_t r = r_sum / samples;
      uint32_t g = g_sum / samples;
      uint32_t b = b_sum / samples;

      // Compute luminance using BT.601 coefficients
      uint32_t lum = (19595 * r + 38470 * g + 7471 * b + (1 << 15)) >> 24;

      // Map luminance to ASCII character
      int index = lum * 9 / 255; // Changed to 9 to prevent index out of bounds
      printf("%c", arr[index]);
    }
    printf("\n");
  }

  // Clean up
  for (int y = 0; y < orig_height; y++)
  {
    free(row_pointers[y]);
  }
  free(row_pointers);
  png_destroy_read_struct(&png, &info, NULL);
  fclose(file);

  return 0;
}
