/*-------------------------------------------------------
 * 
 * JSON.ino
 * 
 * JSON driver
 * 
 * ----------------------------------------------------*/

static char input_JSON[128];

unsigned int json_dip_switch;               // DIP switch overwritten by JSON message
double       json_sensor_dia = DIAMETER;    // Sensor daiamter overwitten by JSON message
unsigned int json_echo;                     // Test String 
double       json_d_echo;                   // Test String

#define IS_INT16   1
#define IS_FLOAT   2
#define IS_DOUBLE  3

typedef struct  {
  char*           token;  // JSON token string, ex "RADIUS": 
  unsigned int*   value;  // Where value is stored 
  double*       d_value;  // Double
  unsigned int    convert;// Conversion type
} json_message;

static json_message JSON[] = {
  {"\"SENSOR\":",     0,                &json_sensor_dia, IS_DOUBLE},
  {"\"ECHO\":"  ,     &json_echo,       0,                IS_INT16 },
  {"\"DIP\":",        &json_dip_switch, 0,                IS_INT16 },
  { 0, 0}
};

/*-----------------------------------------------------
 * 
 * function: read_JSON()
 * 
 * brief: Accumulate input from the serial port
 * 
 * return: None
 * 
 *-----------------------------------------------------
 *
 * The format of the JSON stings used here is
 * 
 * { "LABLE":value }
 * 
 *-----------------------------------------------------*/
static uint16_t in_JSON = 0;

void read_JSON(void)
{
int16_t got_left, got_right;
int     i, j, k, x;
char    ch;

/*
 * See if anything is waiting and if so, add it in
 */
  while ( Serial.available() != 0 )
  {
    ch = Serial.read();
    if ( ch != ' ' )
    {
      input_JSON[in_JSON] = ch;            // Add in the latest
      if ( in_JSON < (sizeof(input_JSON)-1) )
      {
      in_JSON++;
      }
    }
    input_JSON[in_JSON] = 0;              // Null terminate
  }
  
/* 
 *  Check for a complete payload { }
 */
  got_right = -1;
  got_left  = -1;
  for (i=0; i != in_JSON; i++ )
  {
    if ( input_JSON[i] == '{' )
    {
      got_left = i;
    }
    if ( input_JSON[i] == '}' )
    {
      got_right = i;
    }
  }

/*
 * Found out where the braces are, extract the contents.
 */
  if ( (got_left >= 0) && (got_right >= 0 ) )
  {
    for ( i=got_left; i != got_right; i++)
    {
      j = 0;
      while ( JSON[j].token != 0 )
      {
        k = instr(&input_JSON[i], JSON[j].token );
        if ( k > 0 )
        {
          k++;
          switch ( JSON[j].convert )
          {
            default:
            case IS_INT16:
              *JSON[j].value   = atoi(&input_JSON[k]);
              break;

            case IS_FLOAT:
            case IS_DOUBLE:
              *JSON[j].d_value = (double)atof(&input_JSON[k]);
              break;
          }
        } 
        j++;
      }
      in_JSON = 0;
      input_JSON[i] = 0;
    }
  }

/*
 * Test Program {"ECHO":123.45}
 */
  if ( json_echo != 0 )
  {
    Serial.print("\n\r{ ");

    i=0;
    while (JSON[i].token != 0 )
    {
      Serial.print(JSON[i].token);
      switch ( JSON[i].convert )
      {
        default:
        case IS_INT16:
          Serial.print(*JSON[i].value); Serial.print(", ");
          break;

        case IS_FLOAT:
        case IS_DOUBLE:
          Serial.print(*JSON[i].d_value); Serial.print(", ");
          break;
      }
      i++;
    }
    
    Serial.print(" \"VER\": "); Serial.print(SOFTWARE_VERSION); Serial.print("}\n\r"); 
    json_echo = 0;
  }

  
/*
 * All done
 */
return;
}

// Compare two strings.  Return -1 if not equal, end of string if equal
// S1 Long String, S2 Short String . if ( instr("CAT Sam", "CAT") == 3)
int instr(char* s1, char* s2)
{
  int return_value = -1;
  int i;

  i=0;
  while ( (*s1 != 0) && (*s2 != 0) )
  {
    if ( *s1 != *s2 )
    {
      return -1;
    }
    s1++;
    s2++;
    i++;
  }

/*
 * Reached the end of the comparison string
 */
  if ( *s2 == 0 )
  {
    return i;
  }
  return -1;
}
