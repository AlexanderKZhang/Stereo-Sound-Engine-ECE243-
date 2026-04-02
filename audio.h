#ifndef AUDIO_H
#define AUDIO_H

#define AUDIO_BASE 0xFF203040
#define SCALE_FACTOR 1.5
#define AUDIO_WORD_COUNT (2171870 / 2) //Antila_Floriography
// #define AUDIO_WORD_COUNT (94920 / 2)

// Note: HRTF_LENGTH is used in convolve but wasn't defined. 
// Make sure it is defined either here or in hrtf_matrix.h!

struct audio_t {
  volatile unsigned int control;
  volatile unsigned int fifospace;
  volatile unsigned int left_fifo;
  volatile unsigned int right_fifo;
};

// Declare globals using 'extern' so they can be shared
extern struct audio_t* const audiop;
extern int left_index_counter;
extern int right_index_counter;
extern int left, right;

extern const short** fourty_five_deg_hrtf_left;
extern const short** fourty_five_deg_hrtf_right;

// Function prototypes
void audio_setup(void);
void handle_audio(void);
void convolve(const short* audio_word_array, int* result, int angle);
int calculateAngle(int x, int y);
void ILD(int result[2]);

#endif // AUDIO_H