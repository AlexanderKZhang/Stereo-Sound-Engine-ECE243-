#include "audio_array.h"

#define AUDIO_BASE 0xFF203040
#define SCALE_FACTOR 1.5

// If array has 687,168 bytes, it has 171,792 32-bit words
#define AUDIO_WORD_COUNT (687168 / 4)

int main(void) {
  struct audio_t {
    volatile unsigned int control;
    volatile unsigned int fifospace;
    volatile unsigned int left_fifo;
    volatile unsigned int right_fifo;
  };

  struct audio_t* const audiop = ((struct audio_t*)AUDIO_BASE);

  int word_index_counter = 0;
  int left, right;

  // THE MAGIC TRICK:
  // Cast your 8-bit byte array into a 32-bit integer array.
  // (Replace 'bombinsound...' with your exact array name)
  const int* audio_word_array =
      (const int*)bombinsound_the_motherx27s_day_21_second;

  while (1) {
    unsigned int space = audiop->fifospace;

    int wsrc = (space & 0x00FF0000) >> 16;
    int wslc = (space & 0xFF000000) >> 24;

    if ((wsrc > 0) && (wslc > 0)) {
      // Read the fully assembled 32-bit word directly from the casted array!
      int raw_sample_word = audio_word_array[word_index_counter];

      // Apply the volume scale factor
      left = (int)(raw_sample_word * SCALE_FACTOR);
      right = left;

      // Write to the hardware FIFOs
      audiop->left_fifo = left;
      audiop->right_fifo = right;

      // Advance the index. (Change to += 11 if you still need to speed it up)
      word_index_counter++;

      // Wrap around using the WORD count, not the BYTE size
      if (word_index_counter >= AUDIO_WORD_COUNT) {
        word_index_counter = 0;
      }
    }
  }
  return 0;
}