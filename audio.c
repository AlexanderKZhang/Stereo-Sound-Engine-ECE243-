#include "diff_style-electric-storm-mono-sound.h"

#define AUDIO_BASE 0xFF203040
#define SCALE_FACTOR 1.5
#define AUDIO_ARRAY_SIZE 3048594

// Tell the compiler your array is made of signed 8-bit values
typedef signed char byte;

int main(void) {
  // Corrected Audio port structure for 32-bit hardware access
  struct audio_t {
    volatile unsigned int control;
    volatile char RARC;
    volatile char RALC;
    volatile char WSRC;
    volatile char WSLC;  // Corrected from WDLC to WSLC
    volatile unsigned int left_fifo;
    volatile unsigned int right_fifo;
  };

  struct audio_t* const audiop = ((struct audio_t*)AUDIO_BASE);

  int sound_index_counter = 0;
  int left, right;

  while (1) {
    // Ensure space is available to write to BOTH outgoing FIFOs.
    if ((audiop->WSRC > 0) && (audiop->WSLC > 0)) {
      // Get the 8-bit signed sound from the audio array
      byte raw_sample = diff_style_electric_storm_mono[sound_index_counter];

      // Shift the 8-bit signed value up by 24 bits to fill the 32-bit range
      int scaled_sample = ((int)raw_sample) << 24;

      // Apply your SCALE_FACTOR and assign to the left channel
      left = (int)(scaled_sample * SCALE_FACTOR);

      // Since it is mono, copy the exact same signal to the right channel
      right = left;

      // Write the newly mixed audio to the output FIFOs
      audiop->left_fifo = left;
      audiop->right_fifo = right;

      // Advance the circular buffer pointer and wrap around
      sound_index_counter++;
      if (sound_index_counter >= AUDIO_ARRAY_SIZE) {
        sound_index_counter = 0;
      }
    }
  }
  return 0;
}