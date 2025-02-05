#include <lz4.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Simple example of using LZ4 compression and decompression.
 * Return (0);
 */
int main()
{
  // Original data to compress
  const char *original_data =
      "Okay, the user asked how to compress data using LZ4 in C. Let me "
      "think about the best way to explain this. First, I need to make sure "
      "they have the necessary library installed. So I should mention "
      "installing liblz4. On Ubuntu, that's sudo apt-get install "
      "liblz4-dev. For macOS, maybe using Homebrew."
      "Next, the example code. They'll need to include the LZ4 header file. "
      "Then, I should outline the steps: create input data, determine the "
      "maximum compressed size, allocate memory for compressed data, call "
      "LZ4_compress_default, check the result, handle the compressed data, and "
      "then decompress if needed."
      "Wait, in the previous answer, I provided decompression examples for "
      "various algorithms. Now they want compression. So I need to focus on "
      "the "
      "compression part. Let me write a simple example. But maybe they also "
      "want to see decompression to verify it works. Including both makes "
      "sense."
      "I should mention error checking. Like if the compressed size is zero or "
      "negative, that's an error. Also, in the decompression step, using "
      "LZ4_decompress_safe for safety. Including comments in the code to "
      "explain each part"
      "Compiling the code requires linking against liblz4. The command would "
      "be "
      "gcc -o lz4_example lz4_example.c -llz4. I should make sure to note that"
      "Potential pitfalls: buffer sizes. The compressed buffer needs to be "
      "large enough. LZ4_compressBound gives the maximum size. Also, when "
      "decompressing, the destination buffer must be at least the original "
      "size."
      "Maybe add some print statements to show the original and compressed "
      "sizes. That helps in understanding the efficiency. Also, checking if "
      "decompression returns the original size to confirm success."
      "I should keep the code simple and self-contained. Avoid unnecessary "
      "complexity. Use static strings for input to keep it straightforward. "
      "Make sure the code compiles and runs as expected."
      "Finally, summarize the steps and mention use cases where LZ4 is "
      "beneficial, like real-time systems where speed is crucial. Maybe "
      "compare "
      "with other algorithms briefly, but focus on LZ4 here.";

  int original_size =
      strlen(original_data) + 1; // +1 to include null terminator

  // Allocate memory for compressed data (LZ4 requires a specific buffer size)
  int max_compressed_size = LZ4_compressBound(original_size);
  char *compressed_data = malloc(max_compressed_size);
  if (!compressed_data)
  {
    fprintf(stderr, "Failed to allocate memory for compressed data\n");
    return 1;
  }

  // Compress the data
  int compressed_size = LZ4_compress_default(
      original_data, compressed_data, original_size, max_compressed_size);

  if (compressed_size <= 0)
  {
    fprintf(stderr, "Compression failed\n");
    free(compressed_data);
    return 1;
  }

  printf("Original size: %d | Compressed size: %d\n", original_size,
         compressed_size);

  // Allocate memory for decompressed data
  char *decompressed_data = malloc(original_size);
  if (!decompressed_data)
  {
    fprintf(stderr, "Failed to allocate memory for decompressed data\n");
    free(compressed_data);
    return 1;
  }

  // Decompress the data
  int decompressed_size = LZ4_decompress_safe(
      compressed_data, decompressed_data, compressed_size, original_size);

  if (decompressed_size != original_size)
  {
    fprintf(stderr, "Decompression failed (expected %d bytes, got %d)\n",
            original_size, decompressed_size);
    free(compressed_data);
    free(decompressed_data);
    return 1;
  }

  printf("Decompressed data: %s\n", decompressed_data);

  // Cleanup
  free(compressed_data);
  free(decompressed_data);
  return 0;
}
