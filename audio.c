#include "Antila_Floriography.h"

#define AUDIO_BASE 0xFF203040
#define SCALE_FACTOR 1.5

#define AUDIO_WORD_COUNT (1852068 / 2)

int main(void) {
  struct audio_t {
    volatile unsigned int control;
    volatile unsigned int fifospace;
    volatile unsigned int left_fifo;
    volatile unsigned int right_fifo;
  };

  struct audio_t* const audiop = ((struct audio_t*)AUDIO_BASE);

  int left_index_counter = 30;
  int right_index_counter = 0;
  int left, right;

  // THE MAGIC TRICK:
  // Cast the 8-bit byte array into a 16-bit integer array.
  const short* audio_word_array = (const short*)Antila_Floriography;

  while (1) {
    unsigned int space = audiop->fifospace;

    int wsrc = (space & 0x00FF0000) >> 16;
    int wslc = (space & 0xFF000000) >> 24;

    if ((wsrc > 0) && (wslc > 0)) {
      // Cast
      int left_raw_sample_word = audio_word_array[left_index_counter] << 16;
      int right_raw_sample_word = audio_word_array[right_index_counter] << 16;

      // Apply the volume scale factor if needed later
      left = (int)(left_raw_sample_word);
      right = right_raw_sample_word;

      // Write to the hardware FIFOs
      audiop->left_fifo = left;
      audiop->right_fifo = right;

      // Advance the index. (Change to += 11 if you still need to speed it up)
      left_index_counter++;
      right_index_counter++;

      // Wrap around using the WORD count, not the BYTE size
      if (left_raw_sample_word >= AUDIO_WORD_COUNT) {
        left_raw_sample_word = 0;
      }
      if (right_raw_sample_word >= AUDIO_WORD_COUNT) {
        right_raw_sample_word = 0
      }
    }
  }
  return 0;
}