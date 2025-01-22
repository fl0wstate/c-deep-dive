#include <png.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// Reversed the array so brighter pixels use denser characters
char arr[] = {'@', '#', '&', '+', '~', '*', ',', '.', '`', ' '};

int main()
{
  FILE *file = fopen("robert.png", "rb");
  if (!file)
  {
    perror("Failed to open file");
    return 1;
  }

  unsigned char header[8];
  if (fread(header, 1, 8, file) != 8 || png_sig_cmp(header, 0, 8))
  {
    fprintf(stderr, "Invalid PNG file\n");
    fclose(file);
    return 1;
  }

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

  if (setjmp(png_jmpbuf(png)))
  {
    png_destroy_read_struct(&png, &info, NULL);
    fclose(file);
    return 1;
  }

  png_init_io(png, file);
  png_set_sig_bytes(png, 8);
  png_read_info(png, info);

  int orig_width = png_get_image_width(png, info);
  int orig_height = png_get_image_height(png, info);
  png_byte color_type = png_get_color_type(png, info);
  png_byte bit_depth = png_get_bit_depth(png, info);

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

  png_read_update_info(png, info);

  // Adjust scaling to maintain better aspect ratio
  int scale_x = orig_width / 110 + 1;
  int scale_y = orig_height / 55 + 1;

  // Adjust scale_y to account for terminal character aspect ratio (characters
  // are taller than wide)
  scale_y = scale_y * 2;

  int new_width = orig_width / scale_x;
  int new_height = orig_height / scale_y;

  png_bytep *row_pointers =
      (png_bytep *)malloc(sizeof(png_bytep) * orig_height);
  for (int y = 0; y < orig_height; y++)
  {
    row_pointers[y] = (png_byte *)malloc(png_get_rowbytes(png, info));
  }

  png_read_image(png, row_pointers);

  // Print image dimensions for debugging
  fprintf(stderr, "Original: %dx%d, Scaled: %dx%d\n", orig_width, orig_height,
          new_width, new_height);

  for (int y = 0; y < new_height; y++)
  {
    for (int x = 0; x < new_width; x++)
    {
      long r_sum = 0, g_sum = 0, b_sum = 0;
      int samples = 0;

      for (int sy = 0; sy < scale_y && (y * scale_y + sy) < orig_height; sy++)
      {
        for (int sx = 0; sx < scale_x && (x * scale_x + sx) < orig_width; sx++)
        {
          png_bytep px =
              &(row_pointers[y * scale_y + sy][(x * scale_x + sx) * 3]);
          r_sum += px[0];
          g_sum += px[1];
          b_sum += px[2];
          samples++;
        }
      }

      // Calculate average RGB values
      uint8_t r = r_sum / samples;
      uint8_t g = g_sum / samples;
      uint8_t b = b_sum / samples;

      // Calculate perceived brightness using human perception weights
      double brightness = (0.299 * r + 0.587 * g + 0.114 * b);

      // Map brightness to ASCII character index
      int index = (int)((brightness / 255.0) * 9);

      // Print the character twice to account for terminal character aspect
      // ratio
      printf("%c%c", arr[index], arr[index]);
    }
    printf("\n");
  }

  for (int y = 0; y < orig_height; y++)
  {
    free(row_pointers[y]);
  }
  free(row_pointers);
  png_destroy_read_struct(&png, &info, NULL);
  fclose(file);

  return 0;
}
