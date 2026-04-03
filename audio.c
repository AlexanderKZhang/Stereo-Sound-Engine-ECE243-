#include <math.h>

#include "audio.h"
#include "greyCircle.h"
#include "Antila_Floriography.h"
#include "korg_mono_signed_sixteen_bit_PCM.h"
#include "hrtf_matrix_with_elev.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef MAX_AMP
#define MAX_AMP 2147483647
#endif

// Define the actual global variables here (no 'extern')
extern int cursorX, cursorY, angle, elevation;
struct audio_t* const audiop = ((struct audio_t*)AUDIO_BASE);

int left_index_counter = 319802/2;
int right_index_counter = 319802/2;

const short** fourty_five_deg_hrtf_left = (const short**)hrtf_left_matrix;
const short** fourty_five_deg_hrtf_right = (const short**)hrtf_right_matrix;

short zero_array[210000] = {0};

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
    // convolve
    int result[2] = {0};
    convolve(audio_word_array, result, angle);

    ILD(result);

    // Write to the hardware FIFOs
    audiop->left_fifo = result[0];
    audiop->right_fifo = result[1];

    // Advance the index. (Change to += 11 if you still need to speed it up)
    left_index_counter++;
    right_index_counter++;

    // Wrap around using the WORD count, not the BYTE size
    if (left_index_counter >= AUDIO_WORD_COUNT) {
      left_index_counter = 319802/2;
    }
    if (right_index_counter >= AUDIO_WORD_COUNT) {
      right_index_counter = 319802/2;
    }

    space = audiop->fifospace;

    wsrc = (space & 0x00FF0000) >> 16;
    wslc = (space & 0xFF000000) >> 24;
  }
}


void convolve(const short* audio_word_array, int* result, int angle) {
  int elevationIdx = (elevation + 40) / 10;
  int hrtfIdx = (angle > 0) ? (angle/5) : (-angle/5);
  if (angle > 0) {
    if (left_index_counter < HRTF_LENGTH) {
      for (int i = 0; i <= left_index_counter; i++) {
        result[0] += (int) (audio_word_array[left_index_counter - i] * hrtf_left_matrix[elevationIdx][hrtfIdx][i]);
        result[1] += (int) (audio_word_array[right_index_counter - i] * hrtf_right_matrix[elevationIdx][hrtfIdx][i]);
      }
    } else {
      for (int i = 0; i <= (HRTF_LENGTH - 1); i++) {
        result[0] += (int) (audio_word_array[left_index_counter - i] * hrtf_left_matrix[elevationIdx][hrtfIdx][i]);
        result[1] += (int) (audio_word_array[right_index_counter - i] * hrtf_right_matrix[elevationIdx][hrtfIdx][i]);
      }
    }
  } else {
    if (left_index_counter < HRTF_LENGTH) {
      for (int i = 0; i <= left_index_counter; i++) {
        result[0] += (int) (audio_word_array[left_index_counter - i] * hrtf_right_matrix[elevationIdx][hrtfIdx][i]);
        result[1] += (int) (audio_word_array[right_index_counter - i] * hrtf_left_matrix[elevationIdx][hrtfIdx][i]);
      }
    } else {
      for (int i = 0; i <= (HRTF_LENGTH - 1); i++) {
        result[0] += (int) (audio_word_array[left_index_counter - i] * hrtf_right_matrix[elevationIdx][hrtfIdx][i]);
        result[1] += (int) (audio_word_array[right_index_counter - i] * hrtf_left_matrix[elevationIdx][hrtfIdx][i]);
      }
    }
  }
}

//return angle in degree, using atan2 so angle is (0, 180) from positive x axis to negative x axis in quadrant 1, 2; (0, -180) from quadrant 4 to quadrant 3
int calculateAngle(int x, int y) {
  double angle = atan2((double) y, (double) x);

  // convert to degrees
  angle *= 180/M_PI;
  
  // round angle to nearest multiple of 5
  angle = round(angle/5) * 5;

  return angle;
}

void ILD(int result[2]) {
  int distX = cursorX - (GREYCIRCLE_WIDTH >> 1);
  int distY = cursorY - (GREYCIRCLE_HEIGHT >> 1);

  // compute distance from centre of the screen (inverse distance law)
  // I ~ 1/r^2 -> I ~ p^2 -> p ~ 1/r
  // I: sound intensity
  // p: sound pressure (this is apparently what headphones output)
  int dist = (distX*distX) + (distY*distY);
  
  // scale down the avoid huge divisor to be applied to the audio (to avoid heavy computation of square-root)
  dist = dist >> 12;
  // to avoid zero division, choose lower bound for distance
  if (dist == 0) dist = 1;
  
  result[0] /= dist;
  result[1] /= dist;

  if (result[0] > MAX_AMP) {
    result[0] = MAX_AMP;
  }

  if (result[1] > MAX_AMP) {
    result[1] = MAX_AMP;
  }
}