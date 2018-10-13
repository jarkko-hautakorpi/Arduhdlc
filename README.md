# Arduhdlc

ArduHDLC is a *Simple Arduino HDLC library*. It can be used over any interface like USART, SPI, CAN or I2C.
Library only has two functions, validate incoming HDLC frame and wrap data in HDLC packet.
The HDLC frame structure is up to the user to define and decide.

**NOTICE!** *ATM this library only constructs the HDLC frame wrapping the payload and CRC,
and does not implement the full HDLC protocol itself. So this library does not work out of the box with*
https://github.com/SkypLabs/python-hdlc-controller

## Minimum requirements
To use the library, user must pass all incoming data to charReceiver() function, and define two functions:
 1. Function to send HDLC frame out, one byte at a time.
 2. Function to handle/receive a valid HDLC frame
 
When HDLC frame is being built around user data (say, "ABCD"), user defined function is called for each byte of the frame. For "ABCD" 8 bytes are sent out, frame being "\~ABCD??~" where the two characters before end flag ~ are two non printable bytes/characters from 16bit CRC function. CRC for "ABCD" is 53965, or 0xCD and 0xD2 in Hexadecimal representation.

HDLC frame containing "ABCD":

```cpp
		packet	"~ABCD��~"	char
			[0]	126 '~'	char
			[1]	65 'A'	char
			[2]	66 'B'	char
			[3]	67 'C'	char
			[4]	68 'D'	char
			[5]	205 / 205	char
			[6]	210 / 210	char
			[7]	126 '~'	char
```

When valid HDLC frame is found from input stream, received data and length of the data is passed to user defined function. It is up to the user what to do with this data. For receiving "ABCD" user function gets pointer to the data, and integer 4 indicating that data is 4 bytes long.

## Example usage (serial port)

```cpp

#include "Arduhdlc.h"
#define MAX_HDLC_FRAME_LENGTH 32
/* Function to send out byte/char */
void send_character(uint8_t data);

/* Function to handle a valid HDLC frame */
void hdlc_frame_handler(const uint8_t *data, uint16_t length);

/* Initialize Arduhdlc library with three parameters.
1. Character send function, to send out HDLC frame one byte at a time.
2. HDLC frame handler function for received frame.
3. Length of the longest frame used, to allocate buffer in memory */
Arduhdlc hdlc(&send_character, &hdlc_frame_handler, MAX_HDLC_FRAME_LENGTH);

/* Function to send out one 8bit character */
void send_character(uint8_t data) {
    Serial.print((char)data);
}

/* Frame handler function. What to do with received data? */
void hdlc_frame_handler(const uint8_t *data, uint16_t length) {
    // Do something with data that is in framebuffer
}

void setup() {
    pinMode(1,OUTPUT); // Serial port TX to output
    // initialize serial port to 9600 baud
    Serial.begin(9600);
}

void loop() {
    
}

/*
SerialEvent occurs whenever a new data comes in the
hardware serial RX.  This routine is run between each
time loop() runs, so using delay inside loop can delay
response.  Multiple bytes of data may be available.
*/
void serialEvent() {
    while (Serial.available()) {
        // get the new byte:
        char inChar = (char)Serial.read();
        // Pass all incoming data to hdlc char receiver
        hdlc.charReceiver(inChar);
    }
}
```


Complete example project on sending commands and data, through the serial port from Qt GUI program on PC, to Arduino UNO over HDLC protocol, is available on GitHub.
See: [Qt GUI to Arduino HDLC example](https://github.com/jarkko-hautakorpi/arduhdlc-qt-gui-example)
