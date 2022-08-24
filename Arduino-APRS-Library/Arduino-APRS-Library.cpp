/*
 *  Copyright (C) 2018 - Handiko Gesang - www.github.com/handiko
 *  
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

// converted to an Arduino Library by Alan B. Johnston, KU2Y
//
// To use, Sketch/Include Library/Add .ZIP Library then select this file

#include "Arduino-APRS-Library.h"
//#include <Arduino.h>



/*
 * SQUARE WAVE SIGNAL GENERATION
 * 
 * baud_adj lets you to adjust or fine tune overall baud rate
 * by simultaneously adjust the 1200 Hz and 2400 Hz tone,
 * so that both tone would scales synchronously.
 * adj_1200 determined the 1200 hz tone adjustment.
 * tc1200 is the half of the 1200 Hz signal periods.
 * 
 *      -------------------------                           -------
 *     |                         |                         |
 *     |                         |                         |
 *     |                         |                         |
 * ----                           -------------------------
 * 
 *     |<------ tc1200 --------->|<------ tc1200 --------->|
 *     
 * adj_2400 determined the 2400 hz tone adjustment.
 * tc2400 is the half of the 2400 Hz signal periods.
 * 
 *      ------------              ------------              -------
 *     |            |            |            |            |
 *     |            |            |            |            |            
 *     |            |            |            |            |
 * ----              ------------              ------------
 * 
 *     |<--tc2400-->|<--tc2400-->|<--tc2400-->|<--tc2400-->|
 *     
 */
const float baud_adj = 0.975;
const float adj_1200 = 1.0 * baud_adj;
const float adj_2400 = 1.0 * baud_adj;
unsigned int tc1200 = (unsigned int)(0.5 * adj_1200 * 1000000.0 / 1200.0);
unsigned int tc2400 = (unsigned int)(0.5 * adj_2400 * 1000000.0 / 2400.0);

/*
 * This strings will be used to generate AFSK signals, over and over again.
 */
const char *mycall_default = "MYCALL";

char mycall[7];
char myssid = 11;

char dest[7];

const char *digi = "WIDE2";
char digissid = 1;

//const char *mystatus = "Hello World !!";
char mystatus[160];

const char *lat_default = "0610.55S";
const char *lon_default = "10649.62E";
const char sym_ovl_default = 'H';
const char sym_tab_default = 'a';

char lat[10], lon[10], sym_ovl, sym_tab;

unsigned int tx_delay = 5000;
unsigned int str_len = 400;

char bit_stuff = 0;
unsigned short crc=0xffff;

bool nada = _2400;
byte output_pin;

void set_pin( byte pin) {
  output_pin = pin;
  pinMode(output_pin, OUTPUT);
}

void set_status(char *status) { 
//   strncpy(mystatus, status, (strlen(status) < 160) ? strlen(status): 160); 
   strncpy(mystatus, status, (strlen(status) < 160) ? strlen(status): 160);
   mystatus[159] = '\0';
  
}

void set_lat_lon_icon(char *latitude, char *longitude, char *icon) {
  strcpy(lat, latitude); 
  strcpy(lon, longitude);
  sym_ovl = icon[0];
  sym_tab = icon[1];
}

void set_callsign(char *call, char *destination) {
  strcpy(mycall, call);
  strcpy(dest, destination);
}

/*
 * 
 */
void set_nada_1200(void)
{
  digitalWrite(output_pin, HIGH);
  delayMicroseconds(tc1200);
  digitalWrite(output_pin, LOW);
  delayMicroseconds(tc1200);
}

void set_nada_2400(void)
{
  digitalWrite(output_pin, HIGH);
  delayMicroseconds(tc2400);
  digitalWrite(output_pin, LOW);
  delayMicroseconds(tc2400);
  
  digitalWrite(output_pin, HIGH);
  delayMicroseconds(tc2400);
  digitalWrite(output_pin, LOW);
  delayMicroseconds(tc2400);
}

void set_nada(bool nada)
{
  if(nada)
    set_nada_1200();
  else
    set_nada_2400();
}

/*
 * This function will calculate CRC-16 CCITT for the FCS (Frame Check Sequence)
 * as required for the HDLC frame validity check.
 * 
 * Using 0x1021 as polynomial generator. The CRC registers are initialized with
 * 0xFFFF
 */
void calc_crc(bool in_bit)
{
  unsigned short xor_in;
  
  xor_in = crc ^ in_bit;
  crc >>= 1;

  if(xor_in & 0x01)
    crc ^= 0x8408;
}

void send_crc(void)
{
  unsigned char crc_lo = crc ^ 0xff;
  unsigned char crc_hi = (crc >> 8) ^ 0xff;

  send_char_NRZI(crc_lo, HIGH);
  send_char_NRZI(crc_hi, HIGH);
}

void send_header(void)
{
  char temp;

  /*
   * APRS AX.25 Header 
   * ........................................................
   * |   DEST   |  SOURCE  |   DIGI   | CTRL FLD |    PID   |
   * --------------------------------------------------------
   * |  7 bytes |  7 bytes |  7 bytes |   0x03   |   0xf0   |
   * --------------------------------------------------------
   * 
   * DEST   : 6 byte "callsign" + 1 byte ssid
   * SOURCE : 6 byte your callsign + 1 byte ssid
   * DIGI   : 6 byte "digi callsign" + 1 byte ssid
   * 
   * ALL DEST, SOURCE, & DIGI are left shifted 1 bit, ASCII format.
   * DIGI ssid is left shifted 1 bit + 1
   * 
   * CTRL FLD is 0x03 and not shifted.
   * PID is 0xf0 and not shifted.
   */

  /********* DEST ***********/
  temp = strlen(dest);
  for(int j=0; j<temp; j++)
    send_char_NRZI(dest[j] << 1, HIGH);
  if(temp < 6)
  {
    for(int j=0; j<(6 - temp); j++)
      send_char_NRZI(' ' << 1, HIGH);
  }
  send_char_NRZI('0' << 1, HIGH);

  
  /********* SOURCE *********/
  temp = strlen(mycall);
  for(int j=0; j<temp; j++)
    send_char_NRZI(mycall[j] << 1, HIGH);
  if(temp < 6)
  {
    for(int j=0; j<(6 - temp); j++)
      send_char_NRZI(' ' << 1, HIGH);
  }
  send_char_NRZI((myssid + '0') << 1, HIGH);

  
  /********* DIGI ***********/
  temp = strlen(digi);
  for(int j=0; j<temp; j++)
    send_char_NRZI(digi[j] << 1, HIGH);
  if(temp < 6)
  {
    for(int j=0; j<(6 - temp); j++)
      send_char_NRZI(' ' << 1, HIGH);
  }
  send_char_NRZI(((digissid + '0') << 1) + 1, HIGH);

  /***** CTRL FLD & PID *****/
  send_char_NRZI(_CTRL_ID, HIGH);
  send_char_NRZI(_PID, HIGH);
}

void send_payload(char type)
{
  /*
   * APRS AX.25 Payloads
   * 
   * TYPE : POSITION
   * ........................................................
   * |DATA TYPE |    LAT   |SYMB. OVL.|    LON   |SYMB. TBL.|
   * --------------------------------------------------------
   * |  1 byte  |  8 bytes |  1 byte  |  9 bytes |  1 byte  |
   * --------------------------------------------------------
   * 
   * DATA TYPE  : !
   * LAT        : ddmm.ssN or ddmm.ssS
   * LON        : dddmm.ssE or dddmm.ssW
   * 
   * 
   * TYPE : STATUS
   * ..................................
   * |DATA TYPE |    STATUS TEXT      |
   * ----------------------------------
   * |  1 byte  |       N bytes       |
   * ----------------------------------
   * 
   * DATA TYPE  : >
   * STATUS TEXT: Free form text
   * 
   * 
   * TYPE : POSITION & STATUS
   * ..............................................................................
   * |DATA TYPE |    LAT   |SYMB. OVL.|    LON   |SYMB. TBL.|    STATUS TEXT      |
   * ------------------------------------------------------------------------------
   * |  1 byte  |  8 bytes |  1 byte  |  9 bytes |  1 byte  |       N bytes       |
   * ------------------------------------------------------------------------------
   * 
   * DATA TYPE  : !
   * LAT        : ddmm.ssN or ddmm.ssS
   * LON        : dddmm.ssE or dddmm.ssW
   * STATUS TEXT: Free form text
   * 
   * 
   * All of the data are sent in the form of ASCII Text, not shifted.
   * 
   */
  if(type == _FIXPOS)
  {
    send_char_NRZI(_DT_POS, HIGH);
    send_string_len(lat, strlen(lat));
    send_char_NRZI(sym_ovl, HIGH);
    send_string_len(lon, strlen(lon));
    send_char_NRZI(sym_tab, HIGH);
  }
  else if(type == _STATUS)
  {
    send_char_NRZI(_DT_STATUS, HIGH);
    send_string_len(mystatus, strlen(mystatus));
  }
  else if(type == _FIXPOS_STATUS)
  {
    send_char_NRZI(_DT_POS, HIGH);
    send_string_len(lat, strlen(lat));
    send_char_NRZI(sym_ovl, HIGH);
    send_string_len(lon, strlen(lon));
    send_char_NRZI(sym_tab, HIGH);

    send_char_NRZI(' ', HIGH);
    send_string_len(mystatus, strlen(mystatus));
  }
}

/*
 * This function will send one byte input and convert it
 * into AFSK signal one bit at a time LSB first.
 * 
 * The encode which used is NRZI (Non Return to Zero, Inverted)
 * bit 1 : transmitted as no change in tone
 * bit 0 : transmitted as change in tone
 */
void send_char_NRZI(unsigned char in_byte, bool enBitStuff)
{
  bool bits;
  
  for(int i = 0; i < 8; i++)
  {
    bits = in_byte & 0x01;

    calc_crc(bits);

    if(bits)
    {
      set_nada(nada);
      bit_stuff++;

      if((enBitStuff) && (bit_stuff == 5))
      {
        nada ^= 1;
        set_nada(nada);
        
        bit_stuff = 0;
      }
    }
    else
    {
      nada ^= 1;
      set_nada(nada);

      bit_stuff = 0;
    }

    in_byte >>= 1;
  }
}

void send_string_len(const char *in_string, int len)
{
  for(int j=0; j<len; j++)
    send_char_NRZI(in_string[j], HIGH);
}

void send_flag(unsigned char flag_len)
{
  for(int j=0; j<flag_len; j++)
    send_char_NRZI(_FLAG, LOW); 
}

/*
 * In this preliminary test, a packet is consists of FLAG(s) and PAYLOAD(s).
 * Standard APRS FLAG is 0x7e character sent over and over again as a packet
 * delimiter. In this example, 100 flags is used the preamble and 3 flags as
 * the postamble.
 */
void send_packet(char packet_type, bool debug)
{
  if (debug)
    print_debug(packet_type);

  digitalWrite(LED_BUILTIN, 1);

  /*
   * AX25 FRAME
   * 
   * ........................................................
   * |  FLAG(s) |  HEADER  | PAYLOAD  | FCS(CRC) |  FLAG(s) |
   * --------------------------------------------------------
   * |  N bytes | 22 bytes |  N bytes | 2 bytes  |  N bytes |
   * --------------------------------------------------------
   * 
   * FLAG(s)  : 0x7e
   * HEADER   : see header
   * PAYLOAD  : 1 byte data type + N byte info
   * FCS      : 2 bytes calculated from HEADER + PAYLOAD
   */
  
  send_flag(100);
  crc = 0xffff;
  send_header();
  send_payload(packet_type);
  send_crc();
  send_flag(3);

  digitalWrite(LED_BUILTIN, 0);
}

/*
 * Function to randomized the value of a variable with defined low and hi limit value.
 * Used to create random AFSK pulse length.
 */
void randomize(unsigned int &var, unsigned int low, unsigned int high)
{
  var = random(low, high);
}

/*
 * 
 */
void set_io(void)
{
  pinMode(LED_BUILTIN, OUTPUT);
//  pinMode(OUT_PIN, OUTPUT);

  Serial.begin(115200);
}

void print_code_version(void)
{
  Serial.println(" ");
  Serial.print("Sketch:   ");   Serial.println(__FILE__);
  Serial.print("Uploaded: ");   Serial.println(__DATE__);
  Serial.println(" ");
  
  Serial.println("Random String Pulsed AFSK Generator - Started \n");
}

void print_debug(char type)
{
  int temp;
  
  /*
   * PROTOCOL DEBUG.
   * 
   * Will outputs the transmitted data to the serial monitor
   * in the form of TNC2 string format.
   * 
   * MYCALL-N>APRS,DIGIn-N:<PAYLOAD STRING> <CR><LF>
   */

  /****** MYCALL ********/
  temp = strlen(mycall);
  for(int j=0; j<temp; j++)
    Serial.print(mycall[j]);
  Serial.print('-');
  Serial.print(myssid, DEC);
  Serial.print('>');
  
  /******** DEST ********/
  temp = strlen(dest);
  for(int j=0; j<temp; j++)
    Serial.print(dest[j]);
  Serial.print(',');

  /******** DIGI ********/
  temp = strlen(digi);
  for(int j=0; j<temp; j++)
    Serial.print(digi[j]);
  Serial.print('-');
  Serial.print(digissid, DEC);
  Serial.print(':');

  /******* PAYLOAD ******/
  if(type == _FIXPOS)
  {
    Serial.print(_DT_POS);
    Serial.print(lat);
    Serial.print(sym_ovl);
    Serial.print(lon);
    Serial.print(sym_tab);
  }
  else if(type == _STATUS)
  {
    Serial.print(_DT_STATUS);
    Serial.print(mystatus);
  }
  else if(type == _FIXPOS_STATUS)
  {
    Serial.print(_DT_POS);
    Serial.print(lat);
    Serial.print(sym_ovl);
    Serial.print(lon);
    Serial.print(sym_tab);

    Serial.print(' ');
    Serial.print(mystatus);
  }
  
  Serial.println(' ');
}

/*
 * 
 */
