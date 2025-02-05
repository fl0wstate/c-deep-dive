#include <png.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// Using a denser set of ASCII characters for more saturation
// Characters are ordered from darkest to brightest
char arr[] = {'$', '@', 'B', '%', '8', '&',  'W', 'M',  '#', '*', 'o', 'a',
              'h', 'k', 'b', 'd', 'p', 'q',  'w', 'm',  'Z', 'O', '0', 'Q',
              'L', 'C', 'J', 'U', 'Y', 'X',  'z', 'c',  'v', 'u', 'n', 'x',
              'r', 'j', 'f', 't', '/', '\\', '|', '(',  ')', '1', '{', '}',
              '[', ']', '?', '-', '_', '+',  '~', '<',  '>', 'i', '!', 'l',
              'I', ';', ':', ',', '"', '^',  '`', '\'', '.', ' '};

int main()
{
  FILE *file = fopen("toji.png", "rb");
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

  // Adjusted scaling for better detail
  int scale_x =
      orig_width / 170 + 1; // Increased from 110 to 150 for more detail
  int scale_y = orig_height / 120 + 1; // Increased from 55 to 75

  scale_y = scale_y * 2; // Account for terminal character aspect ratio
  scale_x = scale_x * 2;

  int new_width = orig_width / scale_x;
  int new_height = orig_height / scale_y;

  png_bytep *row_pointers =
      (png_bytep *)malloc(sizeof(png_bytep) * orig_height);
  for (int y = 0; y < orig_height; y++)
  {
    row_pointers[y] = (png_byte *)malloc(png_get_rowbytes(png, info));
  }

  png_read_image(png, row_pointers);

  // Enhanced contrast parameters
  const double contrast = 1.5;          // Increase for more contrast
  const double brightness_offset = 0.1; // Adjust for overall brightness

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

      uint8_t r = r_sum / samples;
      uint8_t g = g_sum / samples;
      uint8_t b = b_sum / samples;

      // Enhanced brightness calculation with contrast adjustment
      double brightness = (0.299 * r + 0.587 * g + 0.114 * b) / 255.0;

      // Apply contrast enhancement
      brightness = (brightness - 0.5) * contrast + 0.5 + brightness_offset;
      if (brightness > 1.0)
        brightness = 1.0;
      if (brightness < 0.0)
        brightness = 0.0;

      // Map to the larger character set (70 characters)
      int index = (int)(brightness * 69); // 70 characters - 1

      // Print character twice for aspect ratio
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
