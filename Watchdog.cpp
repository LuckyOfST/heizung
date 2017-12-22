#include "avr/interrupt.h"
#include "avr/wdt.h"

/// BRICKING PREVENTION
// Note that for newer devices( ATmega88 and newer, effectively any AVR that has the option to also generate interrupts ), 
// the watchdog timer remains active even after a system reset( except a power - on condition ), using the fastest prescaler 
// value( approximately 15 ms ).It is therefore required to turn off the watchdog early during program startup, the datasheet 
// recommends a sequence like the following :


uint8_t mcusr_mirror __attribute__( (section( ".noinit" )) );

void get_mcusr( void ) \
__attribute__( (naked) ) \
__attribute__( (section( ".init3" )) );
void get_mcusr( void ){
  mcusr_mirror = MCUSR;
  MCUSR = 0;
  wdt_disable();
}

/// LONG WATCHDOG TIMER
volatile int counter;
#define counterMax 113  //Number of times of ISR(WDT_vect) to autoreset the board. I will autoreset the board after 8 seconds x counterMax

ISR ( WDT_vect )
{
  counter += 1;
  if ( counter < counterMax - 1 ) {
    wdt_reset(); // Reset timer, still in interrupt mode
    // Next time watchdogtimer complete the cycle, it will generate another ISR interrupt
  } else {
    MCUSR = 0;
    WDTCSR |= 0b00011000;    //WDCE y WDE = 1 --> config mode
    WDTCSR = 0b00001000 | 0b100001;    //clear WDIE (interrupt mode disabled), set WDE (reset mode enabled) and set interval to 8 seconds
    //Next time watchdog timer complete the cycle will reset the IC
  }
}

void startWatchdog(){
  counter = 0;
  cli();    //disabled ints
  MCUSR = 0;   //clear reset status
  WDTCSR |= 0b00011000;    //WDCE y WDE = 1 --> config mode
  WDTCSR = 0b01000000 | 0b100001;    //set WDIE (interrupt mode enabled), clear WDE (reset mode disabled) and set interval to 8 seconds
  sei();   //enable ints
  wdt_reset();
}

void stopWatchdog(){
  wdt_disable();
}
