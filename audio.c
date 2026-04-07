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

// The true starting index for the true audio,  319802/2 = 159901 is the sample size (in short type) of the trash audio we added to the real audio to avoid sound distortion caused by the overlapping audio file and vga
#define audio_start_index 159901

struct audio_t* const audiop = ((struct audio_t*)AUDIO_BASE);
int left_index_counter = audio_start_index;
int right_index_counter = audio_start_index;

// Define the actual global variables that are defined in main.c so they can be used in here
extern int cursorX, cursorY, angle, elevation, current_left_sample, current_right_sample;


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

  int result[2] = {0};

  int tempAngle;

  while ((wsrc > 0) && (wslc > 0)) {
    // reset output sample
    result[0] = 0;
    result[1] = 0;

    tempAngle = angle;

    // convolve
    convolve(audio_word_array, result, tempAngle);

    ILD(result);

    // Update the globals for VGA display
    current_left_sample = result[0];
    current_right_sample = result[1];

    // Write to the hardware FIFOs
    audiop->left_fifo = result[0];
    audiop->right_fifo = result[1];

    // Advance the index. (Change to += 11 if you still need to speed it up)
    left_index_counter++;
    right_index_counter++;

    // Wrap around using the WORD count, not the BYTE size
    if (left_index_counter >= AUDIO_WORD_COUNT) {
      left_index_counter = audio_start_index;
    }
    if (right_index_counter >= AUDIO_WORD_COUNT) {
      right_index_counter = audio_start_index;
    }

    space = audiop->fifospace;

    wsrc = (space & 0x00FF0000) >> 16;
    wslc = (space & 0xFF000000) >> 24;
  }
}


void convolve(const short* audio_word_array, int* result, int tempAngle) {
  int elevationIdx = (elevation + 40) / 10;
  if (elevationIdx < 0) elevationIdx = 0;
  if (elevationIdx > 13) elevationIdx = 13;

  int hrtfIdx = (tempAngle > 0) ? (tempAngle/5) : (-tempAngle/5);
  if (hrtfIdx > 36) hrtfIdx = 36;

  const short *left_ir, *right_ir;


  if (tempAngle >= 0) {
      left_ir = leftMatrices[elevationIdx][hrtfIdx];
      right_ir = rightMatrices[elevationIdx][hrtfIdx];
  } else {
      // Negative angles: Swap the matrices to mirror KEMAR's right side to the left
      left_ir = rightMatrices[elevationIdx][hrtfIdx];
      right_ir = leftMatrices[elevationIdx][hrtfIdx];
  }

  // perform convolution
  if (left_index_counter < HRTF_LENGTH) {
    for (int i = 0; i <= left_index_counter; i++) {
      result[0] += (int) (audio_word_array[left_index_counter - i] * left_ir[i]);
      result[1] += (int) (audio_word_array[right_index_counter - i] * right_ir[i]);
    }
  } else {
    for (int i = 0; i <= (HRTF_LENGTH - 1); i++) {
      result[0] += (int) (audio_word_array[left_index_counter - i] * left_ir[i]);
      result[1] += (int) (audio_word_array[right_index_counter - i] * right_ir[i]);
    }
  }
}

//return angle in degree, using atan2 so angle is (0, 180) from positive x axis to negative x axis in quadrant 1, 2; (0, -180) from quadrant 4 to quadrant 3
int calculateAngle(int x, int y) {

  if ((x*x + y*y) < 25) {
    return 0;
  }

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
  dist = dist >> 14;
  // to avoid zero division, choose lower bound for distance
  if (dist == 0) dist = 1;
  
  result[0] /= dist;
  result[1] /= dist;

  if (result[0] > MAX_AMP) result[0] = MAX_AMP;
  if (result[1] > MAX_AMP) result[1] = MAX_AMP;

  if (result[0] < -MAX_AMP) result[0] = -MAX_AMP;
  if (result[1] < -MAX_AMP) result[1] = -MAX_AMP;
}