#include <jpeglib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

// Structure to hold image data
typedef struct
{
  unsigned char *data; // Raw pixel data (in RGB format)
  int width;           // Image width
  int height;          // Image height
  int channels;        // Number of color channels (3 for RGB)
} Image;

// Function to read a JPEG file
Image read_jpeg(const char *filename)
{
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  FILE *infile;
  Image img = {NULL, 0, 0, 0};

  // Open the JPEG file
  if ((infile = fopen(filename, "rb")) == NULL)
  {
    fprintf(stderr, "Error opening %s\n", filename);
    return img;
  }

  // Initialize JPEG decompression
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);
  jpeg_stdio_src(&cinfo, infile);
  jpeg_read_header(&cinfo, TRUE);

  // Start decompression
  jpeg_start_decompress(&cinfo);

  // Allocate memory for pixel data
  img.width = cinfo.output_width;
  img.height = cinfo.output_height;
  img.channels = cinfo.output_components; // Usually 3 for RGB
  img.data = (unsigned char *)malloc(img.width * img.height * img.channels);

  // Read scanlines one by one
  unsigned char *row = img.data;
  int row_stride = img.width * img.channels;
  while (cinfo.output_scanline < cinfo.output_height)
  {
    row = img.data + cinfo.output_scanline * row_stride;
    jpeg_read_scanlines(&cinfo, &row, 1);
  }

  // Cleanup
  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
  fclose(infile);

  return img;
}

int main(int argc, char **argv)
{
  if (argc != 2)
  {
    fprintf(stderr, "Usage: %s <input.jpg>\n", argv[0]);
    return 1;
  }

  // Read the JPEG file
  Image img = read_jpeg(argv[1]);
  if (!img.data)
  {
    fprintf(stderr, "Failed to read image\n");
    return 1;
  }

  // Print image metadata
  printf("Image width: %d\n", img.width);
  printf("Image height: %d\n", img.height);
  printf("Number of channels: %d\n", img.channels);

  // Example: Print RGB values of the first 10 pixels
  for (int i = 0; i < 10; i++)
  {
    printf("Pixel %d: R=%d, G=%d, B=%d\n", i, img.data[i * img.channels],
           img.data[i * img.channels + 1], img.data[i * img.channels + 2]);
  }

  // Free allocated memory
  free(img.data);
  return 0;
}
