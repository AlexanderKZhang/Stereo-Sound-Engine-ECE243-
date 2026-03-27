#include "hrtf_matrix.h"
#include "Antila_Floriography.h"
#include "audio.h"
#include "greyCircle.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 1. Define the actual global variables here (no 'extern')
extern int cursorX, cursorY, angle;
struct audio_t* const audiop = ((struct audio_t*)AUDIO_BASE);

int left_index_counter = 0;
int right_index_counter = 0;
int left = 0;
int right = 0;

const short** fourty_five_deg_hrtf_left = (const short**)hrtf_left_matrix;
const short** fourty_five_deg_hrtf_right = (const short**)hrtf_right_matrix;


// Set up the audio
void audio_setup(void) {
  // setting 1 to the WE bit to generate an interrupt when either of the Write
  // FIFOs are less that 25% full
  audiop->control = 0x2;
}

void handle_audio(void) {
  // THE MAGIC TRICK:
  // Cast the 8-bit byte array into a 16-bit integer array.
  const short* audio_word_array = (const short*)Antila_Floriography;

  unsigned int space = audiop->fifospace;
    
  int wsrc = (space & 0x00FF0000) >> 16;
  int wslc = (space & 0xFF000000) >> 24;
  while ((wsrc > 0) && (wslc > 0)) {
    int left_raw_sample_word = audio_word_array[left_index_counter];
    int right_raw_sample_word = audio_word_array[right_index_counter];

    // Apply the volume scale factor if needed later
    left = (int)(left_raw_sample_word);
    right = right_raw_sample_word;

    // convolve
    int result[2] = {0};
    convolve(audio_word_array, result, angle);

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

    space = audiop->fifospace;

    wsrc = (space & 0x00FF0000) >> 16;
    wslc = (space & 0xFF000000) >> 24;
  }
}


void convolve(const short* audio_word_array, int* result, int angle) {
  int hrtfIdx = (angle/5);
  if (cursorX >= 0) {
    if (left_index_counter < HRTF_LENGTH) {
      for (int i = 0; i <= left_index_counter; i++) {
        result[0] += (int) (audio_word_array[left_index_counter - i] * hrtf_left_matrix[hrtfIdx][i]);
        result[1] += (int) (audio_word_array[right_index_counter - i] * hrtf_right_matrix[hrtfIdx][i]);
      }
    } else {
      for (int i = 0; i <= (HRTF_LENGTH - 1); i++) {
        result[0] += (int) (audio_word_array[left_index_counter - i] * hrtf_left_matrix[hrtfIdx][i]);
        result[1] += (int) (audio_word_array[left_index_counter - i] * hrtf_right_matrix[hrtfIdx][i]);
      }
    }
  } else {
    if (left_index_counter < HRTF_LENGTH) {
      for (int i = 0; i <= left_index_counter; i++) {
        result[0] += (int) (audio_word_array[left_index_counter - i] * hrtf_right_matrix[hrtfIdx][i]);
        result[1] += (int) (audio_word_array[right_index_counter - i] * hrtf_left_matrix[hrtfIdx][i]);
      }
    } else {
      for (int i = 0; i <= (HRTF_LENGTH - 1); i++) {
        result[0] += (int) (audio_word_array[left_index_counter - i] * hrtf_right_matrix[hrtfIdx][i]);
        result[1] += (int) (audio_word_array[left_index_counter - i] * hrtf_left_matrix[hrtfIdx][i]);
      }
    }
  }
}

int calculateAngle(int x, int y) {
  double angle = atan2((double) y, (double) x);

  // convert to degrees
  angle *= 180/M_PI;
  
  // round angle to nearest multiple of 5
  angle = round(angle/5) * 5;

  return angle;
}