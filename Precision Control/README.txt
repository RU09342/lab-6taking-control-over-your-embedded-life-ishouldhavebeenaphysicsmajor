Authors: Ben Jukus, Ryan Drexel
Intro to Embedded Systems: Lab 6- Precision Control
	Russell Trafford, Dr. Tang

***CONTENTS***
-PWM part II
-R2R DAC

=============
|PWM part II|
=============
## Hardware
In order to PWM an active Low Pass Filter with cutoff frequency of approximately 7.2 kHz, this circuit with a transient analysis can be seen below. 
![TransientLowPass](TransientLowPass.PNG) 
The point of making this low pass filter is basically to see that the PWM is just like an other signal generator. This circuit simply proves its ability to perform as such. 
## Software
The code for this portion of the lab was taken from previous labs, PWM from Lab 4 and UART from Milestone 1. These concepts were merged together so that the PWM of the fan can be manipulated over UART, which was done simply by making TA0CCR1 equal to UCA0RXBUF in the UART interrupt. 

=========
|R2R DAC|
=========
The R2R DAC is the simplest way to make a digital to analog converter. All that is required is a handful of resistors. The ladder circuit is shown below.
Inputs B8 : B1 are the digital inputs- with B1 being the most significant

	
	let === be a resistor of value R, and let ### be a resistor of value 2R

	
B8in	|	B7in	|	B6in   |				B2in	|	B1in	|
	#		#	       #					#		#
	#		#	       #					#		#
	#		#	       #				     	#		#
	|_______===_____|_____===______|	   ........     	===_____|_______===_____|______Vout
	|
	#
	#
	#
	|		
	grnd

If you were to solve this circuit for equivalent resistances, you would find that each input bit is contributing half of the preceding bit.
	
	if Bxin is a binary 1 or 0, and Vin is the voltage of that signal, the following equation yields Vout

		Vout = (B1in * Vin / 2) + (B2in * Vin / 4)  (B3in * Vin / 8) + (B4in * Vin / 16) + (B5in * Vin / 32) + (B6in * Vin / 64) + (B7in * Vin / 128) + (B8in * Vin / 256)

	this means the max value of Vout is (Vin- (Vin/256)) and a resolution of Vin/256
