#ifndef Defines_h
#define Defines_h

//#define DEBUG if(true)
#define DEBUG if(false)

//#define DEBUG2 if(true)
#define DEBUG2 if(false)

// set this define to be able to force controllers to a specific level without working sensors.
#define DEBUG_IGNORE_SENSORS

#define BEGINMSG if(true){ Udp.beginPacket(0xffffffff,12888);Udp <<
#define ENDMSG ; Udp.endPacket(); }

// defines the number 1-wire busses
#define BUS_COUNT 24

// defines the number of sensors connected to 1-wire busses
#define SENSOR_COUNT 25  

// defines the number of actors (relais) connected
#define ACTOR_COUNT 24

// define the cycle time of each actor in seconds (=PWM frequence)
#define ACTOR_CYCLE_INTERVAL 60ul*20
//#define ACTOR_CYCLE_INTERVAL 5

// define the number of temperature controllers (usually the number of rooms)
#define CONTROLLER_COUNT 24

// defines the time interval the controller updates its logic in milliseconds
#define CONTROLLER_UPDATE_INTERVAL 5000ul

// set this define if the relais are "on" using a low input level
//#define INVERT_RELAIS_LEVEL

#define lz(i) (i<10?"0":"") << i

#define FTP_RECEIVE "RETR"
#define FTP_STORE "STOR"
#define FTP_APPEND "APPE"
#define FTP_MAKEDIR "MKD"  

#define ACTOR_SWITCH_ON_SURGE_AMPERAGE 0.2f
#define ACTOR_SWITCH_ON_SURGE_DURATION 60000
#define ACTOR_WORKING_AMPERAGE 0.075f
#define ACTORS_MAX_AMPERAGE 5.f

// SER: yellow wire
// RCLK: black wire
// SRCLK: red wire
#define ACTORS_SER_PIN 5
#define ACTORS_RCLK_PIN 6
#define ACTORS_SRCLK_PIN 7

#endif // Defines_h

