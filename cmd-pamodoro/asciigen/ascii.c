#include <math.h>
#include <png.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>

typedef enum LOG_LEVEL
{
  INFO,
  DEBUG,
  ERROR
} logll;

void LOG(logll level, const char *format, ...)
{
  va_list args;
  time_t now;
  struct tm *timeinfo;
  char timestamp[20];

  // get the time
  time(&now);
  // store the time in a timeinfo struct
  timeinfo = localtime(&now);
  // collect the relevant information from the timeinfo strcut
  strftime(timestamp, 20, "%Y-%m-%d %H:%M:%S", timeinfo);

  // prefix timestamps to the logs
  switch (level)
  {
  case INFO:
    fprintf(stdout, "[%s INFO] ", timestamp);
    break;
  case DEBUG:
    fprintf(stdout, "[%s DEBUG] ", timestamp);
    break;
  case ERROR:
    fprintf(stderr, "[%s ERROR] ", timestamp);
    break;
  }

  va_start(args, format);
  switch (level)
  {
  case INFO:
  case DEBUG:
    vfprintf(stdout, format, args);
    break;
  case ERROR:
    vfprintf(stderr, format, args);
    break;
  }
  va_end(args);
}

// Enhanced character set with 14 levels (0-13 indices)
// const char arr[] = " .,:;i1tfLCG08@"; // 14 characters + null terminator
const char arr[] = " .,:;ox%0@";
// const char arr[] = "@0%xo;:,. ";
// const char arr[] = "@80GCLft1i;:,. ";

int main(int ac, char **av)
{
  if (ac < 2)
  {
    fprintf(stderr, "Usage: %s <png-file>\n", av[0]);
    return EXIT_FAILURE;
  }

  FILE *file = fopen(av[1], "rb");
  if (!file)
  {
    perror("Failed to open file");
    return EXIT_FAILURE;
  }

  // ... [PNG initialization code remains identical until scaling] ...
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

  // ---------------------------------------------------------------
  // 1. Precision Scaling with Aspect Ratio Correction
  // ---------------------------------------------------------------
  const int target_width = 100;   // Adjust for desired output width
  const int target_height = 50;   // Maintain 2:1 aspect ratio
  const double aspect_ratio = .9; // Terminal character aspect ratio

  LOG(DEBUG, "Original Dimensions of the Image [%d, %d]\n", orig_width,
      orig_height);
  // Calculate scaling factors with ceiling division
  int scale_x = (orig_width + target_width - 1) / target_width;
  int scale_y = (orig_height + target_height - 1) / target_height;

  // Apply terminal aspect ratio compensation
  scale_y = (int)(scale_y / aspect_ratio);
  scale_x = (int)(scale_x / aspect_ratio);

  // Ensure minimum scaling of 1
  scale_x = scale_x < 1 ? 1 : scale_x;
  scale_y = scale_y < 1 ? 1 : scale_y;

  // ---------------------------------------------------------------
  // 2. Memory-Safe Image Processing
  // ---------------------------------------------------------------
  png_bytep *row_pointers = NULL;
  int success = 0;

  // Wrap image processing in setjmp-safe block
  if (!setjmp(png_jmpbuf(png)))
  {
    row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * orig_height);
    for (int y = 0; y < orig_height; y++)
    {
      row_pointers[y] = (png_byte *)malloc(png_get_rowbytes(png, info));
    }
    png_read_image(png, row_pointers);
    success = 1;
  }

  if (!success)
  {
    // Cleanup on error
    if (row_pointers)
    {
      for (int y = 0; y < orig_height; y++)
        free(row_pointers[y]);
      free(row_pointers);
    }
    png_destroy_read_struct(&png, &info, NULL);
    fclose(file);
    return EXIT_FAILURE;
  }

  // ---------------------------------------------------------------
  // 3. Optimized Pixel Processing
  // ---------------------------------------------------------------
  const int new_width = orig_width / scale_x;
  const int new_height = orig_height / scale_y;
  const int num_chars = (int)(sizeof(arr) - 2); // 14 - 1 (null) - 1 (safety)

  // ---------------------------------------------------------------
  // Edge-Preserving Sampling
  // ---------------------------------------------------------------
  // TODO: Fix this error
  for (int y = 0; y < new_height; y++)
  {
    for (int x = 0; x < new_width; x++)
    {
      double r_sum = 0, g_sum = 0, b_sum = 0;
      double contrast_sum = 0; // For edge detection
      int samples = 0;

      // Sample block with edge detection
      for (int sy = 0; sy < scale_y; sy++)
      {
        int orig_y = y * scale_y + sy;
        if (orig_y >= orig_height)
          break;

        png_bytep row = row_pointers[orig_y];
        png_bytep prev_row = orig_y > 0 ? row_pointers[orig_y - 1] : NULL;

        for (int sx = 0; sx < scale_x; sx++)
        {
          int orig_x = x * scale_x + sx;
          if (orig_x >= orig_width)
            break;

          png_bytep px = &row[orig_x * 3];

          // Edge detection weight
          double edge_weight = 1.0;
          if (prev_row && orig_x > 0)
          {
            // Simple vertical/horizontal gradient detection
            int diff_v = abs(px[0] - prev_row[orig_x * 3]);
            int diff_h = abs(px[0] - row[(orig_x - 1) * 3]);
            edge_weight += (diff_v + diff_h) / 100.0;
          }

          r_sum += px[0] * edge_weight;
          g_sum += px[1] * edge_weight;
          b_sum += px[2] * edge_weight;
          contrast_sum += edge_weight;
          samples++;
        }
      }
      // -------------------------------------------------------
      // Adaptive Brightness with Detail Boost
      // -------------------------------------------------------
      const double r_avg = r_sum / contrast_sum;
      const double g_avg = g_sum / contrast_sum;
      const double b_avg = b_sum / contrast_sum;

      // Perceptual brightness with gamma correction (2.4)
      double brightness = 0.299 * r_avg + 0.587 * g_avg + 0.114 * b_avg;
      brightness = pow(brightness / 255.0, 1 / 2.4) * 255;

      // Detail boost for dark areas
      if (brightness < 128)
      {
        brightness *= 1.2;
        brightness = brightness > 255 ? 255 : brightness;
      }

      // -------------------------------------------------------
      // Enhanced Character Mapping
      // -------------------------------------------------------
      const int num_chars = sizeof(arr) - 2;
      int index = (int)((brightness / 255.0) * num_chars);
      index = index < 0 ? 0 : (index >= num_chars ? num_chars - 1 : index);
      // Print two characters for aspect ratio compensation, while including the
      // color
      printf("\x1b[38;2;%d;%d;%dm%c\x1b[0m", (int)r_avg, (int)g_avg, (int)b_avg,
             arr[index]);
    }
    printf("\n");
  }
  LOG(DEBUG, "New Ascii Image Dimensions of the Image [%d, %d]\n", new_height,
      new_width);

  // ---------------------------------------------------------------
  // 5. Cleanup
  // ---------------------------------------------------------------
  for (int y = 0; y < orig_height; y++)
    free(row_pointers[y]);
  free(row_pointers);
  png_destroy_read_struct(&png, &info, NULL);
  fclose(file);

  return EXIT_SUCCESS;
}
