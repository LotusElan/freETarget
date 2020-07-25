
/*----------------------------------------------------------------
 * 
 * freETarget.h
 * 
 * Software to run the Air-Rifle / Small Bore Electronic Target
 * 
 *----------------------------------------------------------------
 *
 */
#ifndef _FREETARGET_H
#define _FREETARGET_H

#define SOFTWARE_VERSION "2.0"

/*
 * Compilation Flags
 */
#define SAMPLE_CALCULATIONS false    // Trace the COUNTER values
bool sample_calculations (unsigned int sample, unsigned int rotation);

/*
 * Oscillator Features
 */
#define OSCILLATOR_MHZ   8.0    // 8000 cycles in 1 ms
#define CLOCK_PERIOD  (1.0/OSCILLATOR_MHZ)          // Seconds per bit
#define ONE_SECOND      1000    // 1000 ms delay
#define SHOT_TIME       1000    // Wait 1 ms for the shot to end

#define SELF_TEST       1 + (0 << 4)
#define VERBOSE_TRACE   2 + (0 << 4)
#define PISTOL          4 + (0 << 4)
#define SPIRAL          8 + (0 << 4)

#define N 0
#define E 1
#define S 2
#define W 3

struct history
{
  unsigned int shot;    // Current shot number
  double       x;       // X location of shot
  double       y;       // Y location of shot
};

typedef struct history history_t;

extern double     s_of_sound;


#endif

