/*
Click track to MIDI clock converter. Version 1.0
For atmega328/168/88/48 with 16MHz clock.
Tempo change needs a reset.
Tempo range 40 - ca. 240 BPM.
Input click quarter note.
More info at: 
https://ambinaut.wordpress.com/2017/04/19/click-track-to-midi-clock-converter/
https://github.com/ambinaut/click-track-to-MIDI-clock-converter
*/

#define F_CPU 16000000UL
#define F_UART 31250

//Clear ACI Flag after e.g. 200mS: 2083 * 96uS = 200mS
//200 000 uS / 96uS = 2083 timer/counter(0) ticks
#define maxPulseLength 2083 

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "my_macros.h"
#include "my_defines.h"

inline void serialSend_byte(byte);
inline void blink_Led();

uint16_t counter0_OCF0A_flags = 0; //counter0 Output Capture Register flag events
byte F8_msgs_sent = 0; //how many MIDI CLKs have been sent from last click/pulse

int main(void)
{
	init_io();
		
beginning:

	counter0_OCF0A_flags = 0;
	TCNT0 = 0; // clear Timer0 Register
	TIFR0 = (1 << OCF0A);//Clear Timer0 Output Compare A Match Flag (logical 1)
	ACSR = (1 << ACI); //Clear • Bit 4 – ACI: Analog Comparator Interrupt Flag

	//wait for first audio/tap tempo click/pulse
	while (!readbit(ACSR, ACI)){} //Comparator Flag

	//pulse detected
	led_ON;
	TCNT0 = 0;
	TIFR0 = (1 << OCF0A);
	
	//wait for second pulse and count the interval
	while (1)
	{
		if (readbit(TIFR0, OCF0A)) //Timer0 OCF0A compare match flags (96uS)
			{	
				if (counter0_OCF0A_flags == maxPulseLength) 
					{
						//Clear ACI (Comparator) Flag after 200mS: 2083 * 96uS = 200mS
						ACSR = (1 << ACI);
						led_OFF;
					}
				else if (counter0_OCF0A_flags > maxPulseLength)			
					{
						//on next/second audio/tap tempo pulse exit current while loop and goto pulsed;
						if (readbit(ACSR, ACI)) //if 
							{
								if (!sw_MIDIstart_check) //if midi start switch pressed (inverted logic, pull-ups)
								{
									serialSend_byte(MIDI_Start); //MIDI START message 0xFA
								}	
								serialSend_byte(MIDI_Clock);							
								led_ON;
								TCNT1 = 0;
								F8_msgs_sent = 1;
								TIFR1 |= (1 << OCF1A); //clear OCF1A compare flag (16bit)
								//set value in the OCR1AH and OCR1AL– Output Compare Register 1 A
								OCR1AH = (byte)(counter0_OCF0A_flags >> 8);
								OCR1AL = (byte)(counter0_OCF0A_flags);					
								goto pulsed;
							} 
					} //end else if
			
			//if the second pulse is not detected, or if tempo is lower than 40 BPM
			//restart the process, go to beginning
			//15630 * 96 = 1.5 Seconds, i.e. 40 BPM. [96 = 24 * 4us.]
			if (counter0_OCF0A_flags > 15630){goto beginning;}
				
			counter0_OCF0A_flags++;
			TIFR0 = (1 << OCF0A);//Clear the compare flag	(8-bit)	
					
		} //END if (readbit(TIFR0, OCF0A)) - Timer0 OCA compare match flags
	} //END While(1)

pulsed:
  while(1)
		{		
			if (readbit(TIFR1, OCF1A)) //Compare Match Flag (16bit) - Send MIDI CLOCK message
				{					
					if (F8_msgs_sent < 23)
					{
						serialSend_byte(MIDI_Clock);
						blink_Led();						
					} 
					else if (F8_msgs_sent == 23)
					{
						serialSend_byte(MIDI_Clock); //send the 24th 0xF8 msg
						ACSR = (1 << ACI); //Clear ACI Flag, Tap Click/Pulse flag, Comparator						
						if (!sw_MIDIstart_check)
						{
							//send midi start after 24th midi clock
							//and before the next quarter note, i.e. b4 the next tap tempo pulse
							serialSend_byte(MIDI_Start);
						}
					}				
					else if (F8_msgs_sent == 25)
					{
						//if no more audio pulses are detected continue sending midi clock.
						//The tempo will shift one midi clock message.
						F8_msgs_sent = 1;
						goto freeRunning;
					}
					
					F8_msgs_sent++;
					setbit(TIFR1, OCF1A); //Clear the compare flag [16 bit timer]
					
				} //END if (readbit(TIFR1, OCF1A)) - Compare Flag (16bit)

			//• Bit 4 – ACI: Analog Comparator Interrupt Flag
			//i.e. Tap Pulse
		if (readbit(ACSR, ACI)) //next click detected
			{
				if (F8_msgs_sent > 23) //@120BPM = 10 x 20833uS = 208mS (*max. pulse length)
					{
						serialSend_byte(MIDI_Clock);
						TCNT1 = 0; //Reset Timer/Counter1 - 0xF8
						F8_msgs_sent = 1;		
						blink_Led();
						//ACSR = (1 << ACI); //Clear ACI Flag , (tap pulse); WRONG! 200ms Limit!
						//clear Bit 4 – OCF1A: Timer/Counter1, Output Compare A - Match Flag
						setbit(TIFR1, OCF1A); //Clear the compare flag [16 bit timer]
					}	
			} //end if (readbit(ACSR, ACI))
	} //End While(1)
	
freeRunning:
	while (1)
		{
			if (readbit(TIFR1, OCF1A)) //Compare Flag (16bit) - Send MIDI CLOCK message
			{
				serialSend_byte(MIDI_Clock);
				blink_Led();
				F8_msgs_sent++;
				setbit(TIFR1, OCF1A); //Clear the compare flag [16 bit timer]
			}
			
			if (F8_msgs_sent == 25)
			{				
				F8_msgs_sent = 1;
			}	
					
			if (readbit(ACSR, ACI)) //tap pulse
			{
				if (!sw_MIDIstart_check) //inverted logic, pull-ups
				{
					serialSend_byte(MIDI_Start);
				}
				setbit(TIFR1, OCF1A); //Clear the compare flag [16 bit timer]
				F8_msgs_sent = 0;
				goto pulsed;
			}
		} //END while (1)
	
}//END main void()

void init_io(){
	
	setbit(DDR_led, led);
	clearbit(PORT_led, led);
	
	//tap tempo input, pull-up enabled
	clearbit(DDR_MIDIstart, sw_MIDIstart_number);
	setbit(PORT_MIDIstart, sw_MIDIstart_number);	
	
	//Disable digital buffers on AIN0/AIN1
	//DIDR1 – Digital Input Disable Register 1
	//Optional, reduces power consumption.
	setbit(DIDR1, AIN0D);
	setbit(DIDR1, AIN1D);
	/*
	• Bit 1, 0 – AIN1D, AIN0D: AIN1, AIN0 Digital Input Disable
	When this bit is written logic one, the digital input buffer on the AIN1/0 pin is disabled. The corresponding
	PIN Register bit will always read as zero when this bit is set. When an analog signal is
	applied to the AIN1/0 pin and the digital input from this pin is not needed, this bit should be written
	logic one to reduce power consumption in the digital input buffer.
	*/
	
	//ACSR – Analog Comparator Control and Status Register
	setbit(ACSR, ACIS1); //Comparator Interrupt on Falling Output Edge.
	/*
	• Bits 1, 0 – ACIS1, ACIS0: Analog Comparator Interrupt Mode Select
	These bits determine which comparator events that trigger the Analog Comparator 
	interrupt. The different settings are shown in Table 22-2.
	*/	
	
	//Timer/Counter 0 - (8 Bit)
	//TCCR0A – Timer/Counter Control Register A
	setbit(TCCR0A, WGM01); //CTC OCRA Immediate MAX
	//TCCR0B – Timer/Counter Control Register B
	TCCR0B |= (1<<CS01) | (1<<CS00); // clk/64 - one tick = 4uS @16MHz system clock
	//OCR0A – Output Compare Register A
	OCR0A = 23;  //Counting from 0 to 23, i.e. 24 * 4uS @ F_osc(16MHz)/64	
	//TIFR0 – Timer/Counter 0 Interrupt Flag Register
	//• Bit 1 – OCF0A: Timer/Counter 0 Output Compare A Match Flag
	TIFR0 = (1 << OCF0A);//Clear the compare flag (inverted latch)
	//One MIDI Clock interval = OCF0A matches * 4uS
	
	//Timer/Counter 1 - 16bit 
	TCCR1A = 0;     // clear Timer1 TCCR1A register
	TCCR1B = 0;     // clear Timer1 TCCR1B register
	//TCCR1B – Timer/Counter 1 Control Register B
	//setbit(TCCR1B, ICES1); //Input Capture Pin (ICP1/PB0) trigger capture event on rising edge
	setbit(TCCR1B, WGM12); //Clear Timer on Compare Match (CTC) mode - CTC OCR1A Immediate MAX
	setbit(TCCR1B, CS11); // must set to pre-scaler 64 (4uS/tick) otherwise overflow on output compare register
	setbit(TCCR1B, CS10); //
	TCNT1 = 0; //Reset Timer1
	
	//UART init
	//UBRR calculation
	UBRR0 = 0; //Clear register, URSEL bit must = 0 to write to UBRRH
	const uint16_t reg_value = (F_CPU/F_UART/16UL)-1;
	UBRR0H = (uint8_t)((reg_value & 0xff00) >> 8);
	UBRR0L = (uint8_t)(reg_value & 0x00ff);
	//Enable transmitter
	setbit(UCSR0B, TXEN0);
	//UCSR0B = (1<<RXEN0) | (1<<TXEN0);
	//Set frame format: 8 bit data, 1 stop bit
	//Initial Register Values = 8 bit data, 1 stop bit
			
	//Timer Counter Control Register 2		
}

inline void serialSend_byte(byte s){
	while (! (UCSR0A & (1<<UDRE0)) ); //wait for empty transmit buffer
	UDR0 = s; //Put data into buffer, write.
}

inline void blink_Led(){
	switch (F8_msgs_sent)
	{
		case 1:
		led_ON;
		break;
		
		case 8:
		led_OFF;
		break;
	}
}

