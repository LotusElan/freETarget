/*----------------------------------------------------------------
 *
 * Compute_hit
 *
 * Determine the score
 *
 *---------------------------------------------------------------*/

#include "mechanical.h"
#include "compute_hit.h"
#include "freETarget.h"
#include "compute_hit.h"
#include "analog_io.h"
#include "json.h"

#define THRESHOLD (0.001)

#define PI      (3.14159269d)
#define PI_ON_4 (PI / 4.0d)
#define PI_ON_2 (PI / 2.0d)

#define R(x)  (((x)+location) % 4)    // Rotate the target by location points

/*
 *  Variables
 */

sensor_t s[4];

unsigned int bit_mask[] = {0x01, 0x02, 0x04, 0x08};
unsigned int timer_value[4];    // Array of timer values
double       length_c;          // length of side C

/*----------------------------------------------------------------
 *
 * double speed_of_sound(double temperature)
 *
 * Return the speed of sound (mm / us)
 *
 *----------------------------------------------------------------
 *
 *
 *--------------------------------------------------------------*/

double speed_of_sound(double temperature)
{
  double speed;

  speed = (331.3d + 0.606d * temperature) * 1000.0d / 1000000.0d; 
  
  if ( read_DIP() & (VERBOSE_TRACE | SELF_TEST) )
    {
    Serial.print("\n\rSpeed of sound: "); Serial.print(speed); Serial.print("mm/us");
    Serial.print("  Worst case delay: "); Serial.print(json_sensor_dia / speed * OSCILLATOR_MHZ); Serial.print(" counts");
    }

  return speed;  
}

/*----------------------------------------------------------------
 *
 * void init_sensors()
 *
 *Setup the constants in the strucure
 *
 *----------------------------------------------------------------
 *
 *                             N     (+,+)
 *                             
 *                             
 *                      W      0--R-->E
 *
 *
 *               (-,-)         S 
 * 
 *--------------------------------------------------------------*/
void init_sensors(void)
{
  s_of_sound = speed_of_sound(temperature_C());
  s_of_sound = speed_of_sound(temperature_C());
  length_c = sqrt(2.0d) * (json_sensor_dia / 2.0) / s_of_sound * OSCILLATOR_MHZ;
  
  s[N].index = N;
  s[N].x = 0;
  s[N].y = (json_sensor_dia / 2) / s_of_sound * OSCILLATOR_MHZ;

  s[E].index = E;
  s[E].x = s[N].y;
  s[E].y = 0;

  s[S].index = S;
  s[S].x = 0;
  s[S].y = -s[N].y;

  s[W].index = W;
  s[W].x = -s[E].x;
  s[W].y = 0;
  
  return;
}
/*----------------------------------------------------------------
 *
 * void compute_hit()
 *
 * determine the location of the hit
 *
 *----------------------------------------------------------------
 *
 * See freETarget documentaton for algorithm
 *--------------------------------------------------------------*/

unsigned int compute_hit
  (
  unsigned int shot,                // Shot being processed
  history_t* h                      // Storing the results
  )
{
  double        reference;         // Time of reference counter
  int           location;          // Sensor chosen for reference location
  int           i, j, count;
  double        estimate;          // Estimated position
  double        last_estimate, error; // Location error
  double        r1, r2;            // Distance between points
  double        x, y;              // Computed location
  double        x_avg, y_avg;      // Running average location
  double        smallest;          // Smallest non-zero value measured
  unsigned int  sensor_status;     // Sensor collection status

/*
 *  Compute the current geometry based on the speed of sound
 */
  init_sensors();
  
/*
 *  Read in the counter values 
 */
#if ( SAMPLE_CALCULATIONS == false )
 for (i=N; i <= W; i++)
 {
   timer_value[i] = read_counter(i);
 }
#endif

 if ( read_DIP() & VERBOSE_TRACE )
   {
   Serial.print("\n\rNorth: 0x"); Serial.print(timer_value[N], HEX); Serial.print("  us:"); Serial.print(timer_value[N] / OSCILLATOR_MHZ);
   Serial.print("  East: 0x");    Serial.print(timer_value[E], HEX); Serial.print("  us:"); Serial.print(timer_value[E] / OSCILLATOR_MHZ);
   Serial.print("  South: 0x");   Serial.print(timer_value[S], HEX); Serial.print("  us:"); Serial.print(timer_value[S] / OSCILLATOR_MHZ);
   Serial.print("  West: 0x");    Serial.print(timer_value[W], HEX); Serial.print("  us:"); Serial.print(timer_value[W] / OSCILLATOR_MHZ);
   Serial.print(" length_c: "); Serial.print(length_c);     Serial.print(" cycles");
   }
   
/*
 * Determine the location of the reference counter (longest time)
 */
  reference = timer_value[N];
  location = N;
  for (i=E; i <= W; i++)
  {
    if ( timer_value[i] > reference )
    {
      reference = timer_value[i];
      location = i;
    }
  }
  
 if ( read_DIP() & VERBOSE_TRACE )
   {
   Serial.print("\n\rReference: "); Serial.print(reference); Serial.print("  location:"); Serial.print(location);
   }
   
/*
 * Correct the time to remove the shortest distance
 * Also rotate the reference sensor into the NORTH location
 */
  for (i=N; i <= W; i++)
  {
    s[i].count = reference - timer_value[i];
    s[i].is_valid = ((sensor_status & bit_mask[i]) != 0);
  }

 if ( read_DIP() & VERBOSE_TRACE )
   {
   Serial.print("\n\rCounts ");
   Serial.print(" North: "); Serial.print(s[N].count); Serial.print("  East: "); Serial.print(s[E].count);
   Serial.print(" South: ");    Serial.print(s[S].count); Serial.print("  West: "); Serial.print(s[W].count);
   }
/*
 * Find the smallest non-zero value
 */
  smallest = 1.0e10;
  for (i=N+1; i <= W; i++)
  {
    if ( s[i].count < smallest )
    {
      smallest = s[i].count;
    }
  }

/*
 *  Prime the estimate based on the smallest identified time.
 */
 estimate = length_c - smallest + 1.0d;
 
 if ( read_DIP() & VERBOSE_TRACE )
   {
   Serial.print("\n\restimate: "); Serial.print(estimate);
   }

/*
 * Fill up the structure with the counter geometry
 * Rotated so that the longest time points north
 */
  for (i=N; i <= W; i++)
  {
    s[i].b = s[i].count;
    s[i].c = length_c;
  }
  
  for (i=N; i <= W; i++)
  {
    s[i].a = s[(i+1) % 4].b;
  }

/*  
 *  Loop and calculate the unknown radius (estimate)
 */
  error = 999999;               // Start with a big error
  count = 0;
  
  while (error > THRESHOLD )
  {
    x_avg = 0;                     // Zero out the average values
    y_avg = 0;
    last_estimate = estimate;
    
    for (i=N; i <= W; i++)        // Calculate X/Y for each sensor
    {
      find_xy(&s[i], estimate);// Locate the shot
      x_avg += s[i].xs;        // Keep the running average
      y_avg += s[i].ys;
    }

    x_avg /= 4.0d;                // Work out the average intercept
    y_avg /= 4.0d;

    estimate = sqrt(sq(s[location].x - x_avg) + sq(s[location].y - y_avg));
    error = abs(last_estimate - estimate);

    if ( read_DIP() & VERBOSE_TRACE )
    {
      Serial.print("\n\rx_avg:");  Serial.print(x_avg);   Serial.print("  y_avg:"); Serial.print(y_avg); Serial.print(" estimate:"),  Serial.print(estimate);  Serial.print(" error:"); Serial.print(error);
      Serial.println();
    }
    count++;
    if ( count > 20 )
    {
      break;
    }
  }
  
 /*
  * All done return
  */
  h->shot = shot;
  h->x = x_avg;
  h->y = y_avg;
  return location;
}

/*----------------------------------------------------------------
 *
 * find_xy
 *
 * Calaculate where the shot seems to lie
 *
 *----------------------------------------------------------------
 *
 *  Using the law of Cosines
 *  
 *                    C
 *                 /     \   
 *             b             a  
 *          /                   \
 *     A ------------ c ----------  B
 *  
 *  a^2 = b^2 + c^2 - 2(bc)cos(A)
 *  
 *  Rearranging terms
 *            ( a^2 - b^2 - c^2 )
 *  A = arccos( ----------------)
 *            (      -2bc       )
 *            
 *  In our system, a is the estimate for the shot location
 *                 b is the measured time + estimate of the shot location
 *                 c is the fixed distance between the sensors
 *                 
 *--------------------------------------------------------------*/

void find_xy
    (
     sensor_t* s,           // Sensor to be operatated on
     double estimate        // Estimated position   
     )
{
  double ae, be;            // Locations with error added
  double rotation;          // Angle shot is rotated through
  
  ae = s->a + estimate;     // Dimenstion with error included
  be = s->b + estimate;

  if ( (ae + be) < s->c )   // Check for an accumulated round off error
    {
    s->angle_A = 0;         // Yes, then force to zero.
    }
  else
    {  
    s->angle_A = acos( (sq(ae) - sq(be) - sq(s->c))/(-2.0d * be * s->c));
    }
  
/*
 *  Compute the X,Y based on the detection sensor
 */
  switch (s->index)
  {
    case (N): 
      rotation = PI_ON_2 - PI_ON_4 - s->angle_A;
      s->xs = s->x + ((be) * sin(rotation));
      s->ys = s->y - ((be) * cos(rotation));
      break;
      
    case (E): 
      rotation = s->angle_A - PI_ON_4;
      s->xs = s->x - ((be) * cos(rotation));
      s->ys = s->y + ((be) * sin(rotation));
      break;
      
    case (S): 
      rotation = s->angle_A + PI_ON_4;
      s->xs = s->x - ((be) * cos(rotation));
      s->ys = s->y + ((be) * sin(rotation));
      break;
      
    case (W): 
      rotation = PI_ON_2 - PI_ON_4 - s->angle_A;
      s->xs = s->x + ((be) * cos(rotation));
      s->ys = s->y + ((be) * sin(rotation));
      break;

    default:
      Serial.print("\n\nUnknown Rotation:"); Serial.print(s->index);
  }

/*
 * Debugging
 */
  if ( read_DIP() & VERBOSE_TRACE )
    {
    Serial.print("\n\rindex:"); Serial.print(s->index) ; 
    Serial.print(" a:");        Serial.print(s->a);       Serial.print("  b:");  Serial.print(s->b);
    Serial.print(" ae:");       Serial.print(ae);         Serial.print("  be:"); Serial.print(be);    Serial.print(" c:"),  Serial.print(s->c);
    Serial.print(" cos:");      Serial.print(cos(rotation)); Serial.print(" sin: "); Serial.print(sin(rotation));
    Serial.print(" angle_A:");  Serial.print(s->angle_A); Serial.print("  x:");  Serial.print(s->x);  Serial.print(" y:");  Serial.print(s->y);
    Serial.print(" rotation:"); Serial.print(rotation);   Serial.print("  xs:"); Serial.print(s->xs); Serial.print(" ys:"); Serial.print(s->ys);
    }
 
/*
 *  All done, return
 */
  return;
}
  
/*----------------------------------------------------------------
 *
 * void send_score(void)
 *
 * Send the score out over the serial port
 *
 *----------------------------------------------------------------
 * 
 * The score is sent as:
 * 
 * {"shot":"freETarget", "value":{shot, x, y}}
 *    
 *--------------------------------------------------------------*/

void send_score
  (
  history_t* h,                   // History record
  int shot                        // Current shot
  )
{
  double x, y;                   // Shot location in mm X, Y
  double radius;
  double angle;
  double volts;
  
  x = h->x * s_of_sound * CLOCK_PERIOD;
  y = h->y * s_of_sound * CLOCK_PERIOD;
  radius = sqrt(sq(x) + sq(y));
  angle = atan2(h->y, h->x) / PI * 180.0d;

#if ( S_SHOT )
  Serial.print("{\"shot\":");   Serial.print(shot); Serial.print(", ");
#endif

#if ( S_XY )
  Serial.print("\"x\":");     Serial.print(x);  Serial.print(", ");
  Serial.print("\"y\":");     Serial.print(y);  Serial.print(", ");
#endif

#if ( S_RA )
  Serial.print("\"r\":");     Serial.print(radius); Serial.print(", ");
  Serial.print("\"a\":");     Serial.print(angle);  Serial.print(", ");
#endif

#if ( S_COUNTERS )
  Serial.print("\"N\":");     Serial.print(timer_value[N]); Serial.print(", ");
  Serial.print("\"E\":");     Serial.print(timer_value[E]); Serial.print(", ");
  Serial.print("\"S\":");     Serial.print(timer_value[S]); Serial.print(", ");
  Serial.print("\"W\":");     Serial.print(timer_value[W]); Serial.print(", ");
#endif

#if ( S_MISC )
  Serial.print("\"V\":");     volts = analogRead(V_REFERENCE); Serial.print(TO_VOLTS(volts)); Serial.print(", ");
  Serial.print("\"T\":");     Serial.print(temperature_C());   Serial.print(", ");
  Serial.print("\"I\":");     Serial.print(SOFTWARE_VERSION);
#endif

#if ( S_SHOT )
  Serial.print("}");
#endif
  Serial.println();

  return;
}
