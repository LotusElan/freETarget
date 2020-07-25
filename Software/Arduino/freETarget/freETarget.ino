
/*----------------------------------------------------------------
 * 
 * freETarget
 * 
 * Software to run the Air-Rifle / Small Bore Electronic Target
 * 
 *--------------------------------------------------------------
 *--
 */
#include "freETarget.h"
#include "gpio.h"
#include "compute_hit.h"
#include "analog_io.h"

history_t history;
double        s_of_sound;        // Speed of sound
unsigned int shot = 0;

/*----------------------------------------------------------------
 * 
 * void setup()
 * 
 * Initialize the board and prepare to run
 * 
 *----------------------------------------------------------------
 */
void setup() 
{
  int i;
/*
 *  Setup the serial port
 */
  Serial.begin(115200);
  Serial.println("\n\rfreETarget V2.1");

/*
 *  Set up the port pins
 */
  init_gpio();
  init_sensors();
  init_analog_io();

/*
 * All done, begin the program
 */
 return;
}

/*----------------------------------------------------------------
 * 
 * void loop()
 * 
 * Main control loop
 * 
 *----------------------------------------------------------------
 */
#define SET_MODE      0         // Set the operating mode
#define ARM    (SET_MODE+1)     // State is ready to ARM
#define WAIT    (ARM+1)         // ARM the circuit
#define AQUIRE (WAIT+1)         // Aquire the shot
#define REDUCE (AQUIRE+1)       // Reduce the data
#define WASTE  (REDUCE+1)       // Wait for the shot to end
#define SHOW_ERROR (WASTE+1)    // Got a trigger, but was defective

unsigned int state = SET_MODE;
unsigned long now;
double x_time, y_time;        // Location in time
unsigned int running_mode;
unsigned int sensor_status;   // Record which sensors contain valid data
unsigned int location;        // Sensor location 

void loop() 
{
    read_JSON();

#if ( SAMPLE_CALCULATIONS != 0 )
  unit_test();
#endif
 
/*
 * Cycle through the state machine
 */
  switch (state)
  {

/*
 *  Check for special operating modes
 */
  default:
  case SET_MODE:
    while (read_DIP() & SELF_TEST)
    {
      self_test();
    }
    state = ARM;
    break;
/*
 * Arm the circuit
 */
  case ARM:
    arm_counters();
    set_LED(LED_S, true);     // Show we are waiting
    set_LED(LED_X, false);    // No longer processing
    set_LED(LED_Y, false);   
    state = WAIT;
    sensor_status = 0;
    break;
    
/*
 * Wait for the shot
 */
  case WAIT:
    sensor_status |= is_running();
    if ( sensor_status != 0 )    // Shot detected
      {
      state = AQUIRE;
      now = micros();           // Remember the starting time
      }
     break;

/*
 *  Aquire the shot              
 */  
  case AQUIRE:
    sensor_status |= is_running();        // Remember all of the running timers
    if ( (micros() - now) > SHOT_TIME )   // Enough time already
    { 
      if ( sensor_status == 0x0f )        // Need to have all 4 counters trip
      {
        state = REDUCE;                   // Before we can decode all of the data.
      }
      else
      {
        state = SHOW_ERROR;
      }
      stop_counters();
    }
    break;

/*
 *  Reduce the data to a score
 */
  case REDUCE:
    set_LED(LED_S, false);     // 
    set_LED(LED_X, false);     // No longer processing
    set_LED(LED_Y, true);      // Reducing the shot
    location = compute_hit(shot, &history);
    send_score(&history, shot);
    state = WASTE;
    shot++;                   
    break;

/*
 *  Wait here to make sure the RUN lines are no longer set
 */
  case WASTE:
    delay(1000);
    state = SET_MODE;
    break;
    }

/*
 * All done, exit for now
 */
  return;
}
