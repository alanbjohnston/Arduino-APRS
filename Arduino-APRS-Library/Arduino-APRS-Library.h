#ifndef ARDUINO_APRS_Library_H
#define ARDUINO_APRS_Library_H

#include <Arduino.h>

#include <math.h>
#include <stdio.h>

// Defines the Square Wave Output Pin
#define OUT_PIN 26

#define _1200   1
#define _2400   0

#define _FLAG       0x7e
#define _CTRL_ID    0x03
#define _PID        0xf0
#define _DT_EXP     ','
#define _DT_STATUS  '>'
#define _DT_POS     '!'

#define _FIXPOS         1
#define _STATUS         2
#define _FIXPOS_STATUS  3

    
/*
 * 
 */
void set_pin(byte pin);
void set_callsign(char *call, char *destination);
void set_status(char *status);
void set_lat_lon_icon(char *latitude, char *longitude, char *icon);
void set_nada_1200(void);
void set_nada_2400(void);
void set_nada(bool nada);

void send_char_NRZI(unsigned char in_byte, bool enBitStuff);
void send_string_len(const char *in_string, int len);

void calc_crc(bool in_bit);
void send_crc(void);

void send_packet(char packet_type, bool debug);
void send_flag(unsigned char flag_len);
void send_header(void);
void send_payload(char type);

void set_io(void);
void print_code_version(void);
void print_debug(char type);

#endif
