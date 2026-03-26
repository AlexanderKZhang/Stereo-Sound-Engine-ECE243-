#include "Antila_Floriography.h"
#include "hrtf_matrix.h"

#define AUDIO_BASE 0xFF203040
#define SCALE_FACTOR 1.5
#define AUDIO_WORD_COUNT (1852068 / 2)

struct audio_t {
  volatile unsigned int control;
  volatile unsigned int fifospace;
  volatile unsigned int left_fifo;
  volatile unsigned int right_fifo;
};

struct audio_t* const audiop = ((struct audio_t*)AUDIO_BASE);

int left_index_counter = 0;
int right_index_counter = 0;
int left, right;

// function prototype
int* convolve(const short* audio_word_array, int* result);

void audio_setup(void) {
  // setting 1 to the WE bit to generate an interrupt when either of the Write
  // FIFOs are less that 25% full
  audiop->control = 0x2;
}

void handle_audio(void) {
  // THE MAGIC TRICK:
  // Cast the 8-bit byte array into a 16-bit integer array.
  const short* audio_word_array = (const short*)Antila_Floriography;

  while (1) {
    unsigned int space = audiop->fifospace;

    int wsrc = (space & 0x00FF0000) >> 16;
    int wslc = (space & 0xFF000000) >> 24;

    if ((wsrc > 0) && (wslc > 0)) {
      // Cast
      // int left_raw_sample_word = audio_word_array[left_index_counter] << 16;
      // int right_raw_sample_word = audio_word_array[right_index_counter] <<
      // 16;
      int left_raw_sample_word = audio_word_array[left_index_counter];
      int right_raw_sample_word = audio_word_array[right_index_counter];

      // Apply the volume scale factor if needed later
      left = (int)(left_raw_sample_word);
      right = right_raw_sample_word;

      // convolve
      int result[2] = {0};
      convolve(audio_word_array, &result);

      // Write to the hardware FIFOs
      audiop->left_fifo = result[0];
      audiop->right_fifo = result[1];

      // Advance the index. (Change to += 11 if you still need to speed it up)
      left_index_counter++;
      right_index_counter++;

      // Wrap around using the WORD count, not the BYTE size
      if (left_index_counter >= AUDIO_WORD_COUNT) {
        left_index_counter = 0;
      }
      if (right_index_counter >= AUDIO_WORD_COUNT) {
        right_index_counter = 0;
      }
    }
  }
  return 0;
}

const short** fourty_five_deg_hrtf_left = (const short*)hrtf_left_matrix;
const short** fourty_five_deg_hrtf_right = (const short*)hrtf_right_matrix;

int* convolve(const short* audio_word_array, int* result) {
  if (left_index_counter < HRTF_LENGTH) {
    for (int i = 0; i <= left_index_counter; i++) {
      result[0] += (int) (audio_word_array[left_index_counter - i] * hrtf_left_matrix[10][i]);
      result[1] += (int) (audio_word_array[right_index_counter - i] * hrtf_right_matrix[10][i]);
    }
  } else {
    for (int i = 0; i <= (HRTF_LENGTH - 1); i++) {
      result[0] += (int) (audio_word_array[left_index_counter - i] *
                    hrtf_left_matrix[10][i]);
      result[1] += (int) (audio_word_array[left_index_counter - i] *
                    hrtf_right_matrix[10][i]);
    }
  }
}