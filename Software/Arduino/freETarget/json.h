/*
 * Global functions
 */

void read_JSON(void);         // Scan the serial port looking for JSON input

extern unsigned int  json_dip_switch; // DIP switch overwritten by JSON message
extern double        json_sensor_dia; // Sensor radius overwitten by JSON message
extern unsigned int  json_echo;       // Value to ech
