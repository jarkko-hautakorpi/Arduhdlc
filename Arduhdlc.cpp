/*
Arduhdlc is Arduino HDLC library, which can be used over any interface.
Copyright (C) 2015 Jarkko Hautakorpi et al. see LICENSE.txt

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "Arduino.h"
#include "Arduhdlc.h"

/* HDLC Asynchronous framing */
/* The frame boundary octet is 01111110, (7E in hexadecimal notation) */
#define FRAME_BOUNDARY_OCTET 0x7E

/* A "control escape octet", has the bit sequence '01111101', (7D hexadecimal) */
#define CONTROL_ESCAPE_OCTET 0x7D

/* If either of these two octets appears in the transmitted data, an escape octet is sent, */
/* followed by the original data octet with bit 5 inverted */
#define INVERT_OCTET 0x20

/* The frame check sequence (FCS) is a 16-bit CRC-CCITT */
/* AVR Libc CRC function is _crc_ccitt_update() */
/* Corresponding CRC function in Qt (www.qt.io) is qChecksum() */
#define CRC16_CCITT_INIT_VAL 0xFFFF

/* 16bit low and high bytes copier */
#define low(x)    ((x) & 0xFF)
#define high(x)   (((x)>>8) & 0xFF)

Arduhdlc::Arduhdlc (sendchar_type put_char, frame_handler_type hdlc_command_router, uint16_t max_frame_length) : sendchar_function(put_char), frame_handler(hdlc_command_router)
{
    this->frame_position = 0;
	this->max_frame_length = max_frame_length;
	this->receive_frame_buffer = (uint8_t *)malloc(max_frame_length+1); // char *ab = (char*)malloc(12);
    this->frame_checksum = CRC16_CCITT_INIT_VAL;
    this->escape_character = false;
}

/* Function to send a byte throug USART, I2C, SPI etc.*/
void Arduhdlc::sendchar(uint8_t data)
{
    (*this->sendchar_function)(data);
}

/* Function to find valid HDLC frame from incoming data */
void Arduhdlc::charReceiver(uint8_t data)
{
    /* FRAME FLAG */
    if(data == FRAME_BOUNDARY_OCTET)
    {
        if(this->escape_character == true)
        {
            this->escape_character = false;
        }
        /* If a valid frame is detected */
        else if( (this->frame_position >= 2) &&
                  ( this->frame_checksum == ((this->receive_frame_buffer[this->frame_position-1] << 8 ) | (this->receive_frame_buffer[this->frame_position-2] & 0xff)) ) )  // (msb << 8 ) | (lsb & 0xff)
        {
            /* Call the user defined function and pass frame to it */
            (*frame_handler)(receive_frame_buffer,(uint8_t)(this->frame_position-2));
        }
        this->frame_position = 0;
        this->frame_checksum = CRC16_CCITT_INIT_VAL;
        return;
    }

    if(this->escape_character)
    {
        this->escape_character = false;
        data ^= INVERT_OCTET;
    }
    else if(data == CONTROL_ESCAPE_OCTET)
    {
        this->escape_character = true;
        return;
    }

    receive_frame_buffer[this->frame_position] = data;

    if(this->frame_position-2 >= 0) {
        this->frame_checksum = _crc_ccitt_update(this->frame_checksum, receive_frame_buffer[this->frame_position-2]);
    }

    this->frame_position++;

    if(this->frame_position == this->max_frame_length)
    {
        this->frame_position = 0;
        this->frame_checksum = CRC16_CCITT_INIT_VAL;
    }
}

/* Wrap given data in HDLC frame and send it out byte at a time*/
void Arduhdlc::frameDecode(const char *framebuffer, uint8_t frame_length)
{
    uint8_t data;
    uint16_t fcs = CRC16_CCITT_INIT_VAL;

    this->sendchar((uint8_t)FRAME_BOUNDARY_OCTET);

    while(frame_length)
    {
        data = *framebuffer++;
        fcs = _crc_ccitt_update(fcs, data);
        if((data == CONTROL_ESCAPE_OCTET) || (data == FRAME_BOUNDARY_OCTET))
        {
            this->sendchar((uint8_t)CONTROL_ESCAPE_OCTET);
            data ^= INVERT_OCTET;
        }
        this->sendchar((uint8_t)data);
        frame_length--;
    }
    data = low(fcs);
    if((data == CONTROL_ESCAPE_OCTET) || (data == FRAME_BOUNDARY_OCTET))
    {
        this->sendchar((uint8_t)CONTROL_ESCAPE_OCTET);
        data ^= (uint8_t)INVERT_OCTET;
    }
    this->sendchar((uint8_t)data);
    data = high(fcs);
    if((data == CONTROL_ESCAPE_OCTET) || (data == FRAME_BOUNDARY_OCTET))
    {
        this->sendchar(CONTROL_ESCAPE_OCTET);
        data ^= INVERT_OCTET;
    }
    this->sendchar(data);
    this->sendchar(FRAME_BOUNDARY_OCTET);
}


