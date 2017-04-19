/*
 * my_defines.h
Pulse to MIDI - A.Comp.cpp
 */ 

#ifndef MY_DEFINES_H_
#define MY_DEFINES_H_

//PC3 - Arduino Pin A3
#define sw_MIDIstart_number 3
#define DDR_MIDIstart DDR ## C
#define PORT_MIDIstart PORT ## C
#define PIN_MIDIstart PIN ## C
#define sw_MIDIstart_check readbit(PIN_MIDIstart, sw_MIDIstart_number)

//PB5 - Arduino Pin 13
#define led 5
#define DDR_led DDR ## B
#define PORT_led PORT ## B
#define PIN_led PIN ## B
#define led_ON setbit(PORT_led, led)
#define led_OFF clearbit(PORT_led, led)

#define MIDI_Start 0xFA
#define MIDI_Clock 0xF8

#endif /* MY_DEFINES_H_ */