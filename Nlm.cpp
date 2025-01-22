#include "hls_stream.h"
#include "ap_int.h"
#include "ap_axi_sdata.h"

// Define data types for input and output
typedef ap_uint pixel_t; 
typedef hls::stream<pixel_t> stream_t;

// Define NLM parameters
const int WINDOW_SIZE = 7; // Size of the search window
const int PATCH_SIZE = 3; // Size of the patch for similarity calculation

// Function to calculate Euclidean distance between two patches
ap_uint calc_distance(pixel_t *patch1, pixel_t *patch2) {
  ap_uint dist = 0;
  for (int i = 0; i < PATCH_SIZE * PATCH_SIZE; i++) {
    ap_uint diff = patch1[i] - patch2[i];
    dist += diff * diff;
  }
  return dist;
}

// Function to calculate Gaussian weight
ap_uint gaussian_weight(ap_uint distance, ap_uint sigma) {
  // Simplified Gaussian function for hardware implementation
  // Adjust sigma value based on your requirements
  return 1024 / (1 + (distance / sigma)); // Scale factor for 16-bit representation
}

// NLM filtering function
void NLM_filter(stream_t &in_stream, stream_t &out_stream) {
  // Allocate memory for image buffers (consider optimization techniques)
  pixel_t image[WINDOW_SIZE][WINDOW_SIZE];

  // Read input pixels into the image buffer
  for (int i = 0; i < WINDOW_SIZE; i++) {
    for (int j = 0; j < WINDOW_SIZE; j++) {
      image[i][j] = in_stream.read();
    }
  }

  // Process center pixel
  pixel_t center_pixel = image[WINDOW_SIZE/2][WINDOW_SIZE/2]; 
  ap_uint sum = 0;
  ap_uint weight_sum = 0;

  // Loop through the search window
  for (int i = 0; i < WINDOW_SIZE; i++) {
    for (int j = 0; j < WINDOW_SIZE; j++) {
      // Extract patch 
      pixel_t patch[PATCH_SIZE * PATCH_SIZE];
      int patch_index = 0;
      for (int k = 0; k < PATCH_SIZE; k++) {
        for (int l = 0; l < PATCH_SIZE; l++) {
          patch[patch_index++] = image[i + k - PATCH_SIZE/2][j + l - PATCH_SIZE/2];
        }
      }

      // Calculate distance between patches
      ap_uint dist = calc_distance(&image[WINDOW_SIZE/2 - PATCH_SIZE/2][WINDOW_SIZE/2 - PATCH_SIZE/2], patch);

      // Calculate Gaussian weight
      ap_uint weight = gaussian_weight(dist, 10); // Example sigma value

      // Accumulate weighted pixel values
      sum += weight * patch[PATCH_SIZE * PATCH_SIZE / 2]; 
      weight_sum += weight;
    }
  }

  // Calculate filtered pixel value
  pixel_t filtered_pixel = sum / weight_sum;

  // Write filtered pixel to output stream
  out_stream.write(filtered_pixel);

  // Shift image buffer (circular buffer implementation)
  for (int i = 0; i < WINDOW_SIZE - 1; i++) {
    for (int j = 0; j < WINDOW_SIZE; j++) {
      image[i][j] = image[i + 1][j];
    }
  }

  // Read new row of pixels into the image buffer
  for (int j = 0; j < WINDOW_SIZE; j++) {
    image[WINDOW_SIZE - 1][j] = in_stream.read();
  }
}

// Top-level function
void NLM_filter_top(stream_t &in_stream, stream_t &out_stream) {
  // Loop through the entire image (adjust loop bounds as needed)
  for (int i = 0; i < IMAGE_HEIGHT * IMAGE_WIDTH; i++) {
    NLM_filter(in_stream, out_stream);
  }
}
