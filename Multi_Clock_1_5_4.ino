/***********************************************************************
Multi Clock v 1.0, Jan 2016 by SRDWA
Distributed under the terms of the GPL.

Based on Pong Clock v 5.1 by Nick Hall

For more informations and original Release Notes visit:
http://123led.wordpress.com/
-----------------------------------------------------------------------
Release Notes for v1.0 , updated January 2016 by SRDWA:
  *rename from Prong Clock tu Multi Clock and initial release of Multi Clock
  *lot of changes which SRDWA does, and i can't restore because
  *start translating the Menus to german
-----------------------------------------------------------------------
Release Notes for v1.1 , updated April 2016 by NeoRame:
  *add new german translatings
-----------------------------------------------------------------------
Release Notes for v1.2 , updated April 2016 by NeoRame:
  * adding the "Special Event" function from Matock to display a welcome message every 30 min on specific
    days (ex: New Year Day, Christmas, etc..., cf. is_special_event() function)
  * rename some Settings
  * remove Word Clock
  * add Temperature to "Print Day of Week" Screen
  * add new tiny chars
  * tiny Font for Menu and Settings
-----------------------------------------------------------------------
Release Notes for v1.4 , updated April 2016 by NeoRame:
  * edit Random Menu
  * edit Invaders Watchface
  * edit Normal Watchface
  * edit Temperature Watchface
  * remove Matrix Watchface
  * add new icons to Temperature
-----------------------------------------------------------------------
Release Notes for v1.5 , updated April 2016 by NeoRame:
  * add new created Icon Font
  * edit the way of showing icons on Weather Watchface
  * add new icons for Special Events ;)
  * add Special Events on Weather Watchface
  * delete 12/24h Menu
-----------------------------------------------------------------------
Release Notes for v1.5.3 , updated April 2016 by NeoRame:
  * add new created Invaders Font
  * edit the way the invaders are created (new font)
  * edit the 7px invaders spaceschips to 8px original design
  * change the big font to a new 6x14 big font
  * add Binary Watchface [BCD Style (Binary Coded Decimal)]
  * add BCD Font
-----------------------------------------------------------------------
Release Notes for v1.5.4 , updated May 2016 by NeoRame:
  * change Digits Watchface
-----------------------------------------------------------------------

Uses 2x Sure 2416 LED modules, arduino and DS1307 clock chip.
Distributed under the terms of the GPL.
             
Holtek HT1632 LED driver chip code:
As implemented on the Sure Electronics DE-DP016 display board (16*24 dot matrix LED module.)
Nov, 2008 by Bill Westfield ("WestfW")
Copyrighted and distributed under the terms of the Berkely license (copy freely, but include this notice of original author.)
***********************************************************************/

//include libraries
#include <ht1632c.h>                     // Holtek LED driver by WestFW - updated to HT1632C by Nick Hall
#include <avr/pgmspace.h>                // Enable data to be stored in Flash Mem as well as SRAM              
#include <Font.h>                        // Font library
#include <Wire.h>                        // DS1307 clock
#include "RTClib.h"                      // DS1307 clock
#include <Button.h>                      // Button library by Alexander Brevig

//define constants
#define ASSERT(condition)                // Nothing
#define X_MAX 47                         // Matrix X max LED coordinate (for 2 displays placed next to each other)
#define Y_MAX 15                         // Matrix Y max LED coordinate (for 2 displays placed next to each other)
#define NUM_DISPLAYS 2                   // Num displays for shadow ram data allocation
#define FADEDELAY 30                     // Time to fade display to black
#define NUM_MODES 10                     // Number of clock & display modes (conting zero as the first mode)
#define NUM_SETTINGS_MODES 4             // Number settings modes = 6 (conting zero as the first mode)
#define NUM_DISPLAY_MODES 6              // Number display modes = 5 (conting zero as the first mode)
#define BAT1_X 2                         // Pong left bat x pos (this is where the ball collision occurs, the bat is drawn 1 behind these coords)
#define BAT2_X 45                        // Pong right bat x pos (this is where the ball collision occurs, the bat is drawn 1 behind these coords)
#define plot(x,y,v)  ht1632_plot(x,y,v)  // Plot LED
#define cls          ht1632_clear        // Clear display
#define SLIDE_DELAY 20                   // The time in milliseconds for the slide effect per character in slide mode. Make this higher for a slower effect
#define NEXT_DATE_MIN 10                 // After the date has been displayed automatically, the next time it's displayed is at least 10 mintues later
#define NEXT_DATE_MAX 15                 // After the date has been displayed automatically, the next time it's displayed is at most 15 mintues later

//global variables
static const byte ht1632_data = 10;      // Data pin for sure module
static const byte ht1632_wrclk = 11;     // Write clock pin for sure module
static const byte ht1632_cs[2] = {4, 5}; // Chip_selects one for each sure module. Remember to set the DIP switches on the modules too.

Button buttonA = Button(2, BUTTON_PULLUP);      // Setup button A (using button library)
Button buttonB = Button(3, BUTTON_PULLUP);      // Setup button B (using button library)

RTC_DS1307 ds1307;                       //RTC object

int rtc[7];                              // Holds real time clock output
byte clock_mode = 0;                     // Default clock mode. Default = 0 (Invaders)
byte old_mode = clock_mode;              // Stores the previous clock mode, so if we go to date or whatever, we know what mode to go back to after.
bool ampm = 0;                           // Define 12 or 24 hour time. 0 = 24 hour. 1 = 12 hour
bool random_mode = 1;                    // Define random mode - changes the display type every hours. Default = 0 (on)
bool daylight_mode = 0;                  // Define if DST is on or off. Default = 0 (off).
byte change_mode_time = 0;               // Holds hour when clock mode will next change if in random mode.
byte brightness = 9;                     // Screen brightness - default is 15 which is brightest.
byte next_display_date;                  // Holds the minute at which the date is automatically next displayed
byte old_mins = 99;                      // holds minutes when the last special event was displayed

char days[7][4] = {
  "So.", "Mo.", "Di.", "Mi.", "Do.", "Fr.", "Sa."
}; //day array - used in slide, normal and matrix modes (The DS1307 outputs 1-7 values for day of week)
char daysfull[7][9] = {
  "Sonntag", "Montag", "Dienstag", "Mittwoch", "Donner.", "Freitag", "Samstag"
};
int TMP36 = A0;
int temperatur = 0;
int temp[10];
int tempcorr = 0;                        // If your Temerature are incorecct adjust here


//intial setup

void setup ()
{
  digitalWrite(2, HIGH);                 // turn on pullup resistor for button on pin 2
  digitalWrite(3, HIGH);                 // turn on pullup resistor for button on pin 3

  Serial.begin(57600);                   // Setup serial output
  ht1632_setup();                        // Setup display (uses flow chart from page 17 of sure datasheet)
  randomSeed(analogRead(1));             // Setup random number generator
  ht1632_sendcmd(0, HT1632_CMD_PWM + brightness);
  ht1632_sendcmd(1, HT1632_CMD_PWM + brightness);

  //Setup DS1307 RTC
#ifdef AVR
  Wire.begin();
#else
  Wire1.begin(); // Shield I2C pins connect to alt I2C bus on Arduino
#endif
  ds1307.begin(); //start RTC Clock

  if (! ds1307.isrunning()) {
    Serial.println(F("RTC is NOT running!"));
    ds1307.adjust(DateTime(__DATE__, __TIME__));  // sets the RTC to the date & time this sketch was compiled
  }
  printver(); // Display clock software version
}


// ****** MAIN LOOP ******

void loop ()
{
  //run the clock with whatever mode is set by clock_mode - the default is set at top of code.
  switch (clock_mode) {

    case 0:
      invaders();
      break;
    case 1:
      pong();
      break;
    case 2:
      digits();
      break;
    case 3:
      normal();
      break;
    case 4:
      thermo();
      break;
    case 5:
      slide();
      break;
    case 6:
      binary();
      break;
    case 7:
      setup_menu();
      break;

  }
}


// ****** DISPLAY-ROUTINES ******


//ht1632_chipselect / ht1632_chipfree
//Select or de-select a particular ht1632 chip. De-selecting a chip ends the commands being sent to a chip.
//CD pins are active-low; writing 0 to the pin selects the chip.
void ht1632_chipselect(byte chipno)
{
  DEBUGPRINT("\nHT1632(%d) ", chipno);
  digitalWrite(chipno, 0);
}

void ht1632_chipfree(byte chipno)
{
  DEBUGPRINT(" [done %d]", chipno);
  digitalWrite(chipno, 1);
}


//ht1632_writebits
//Write bits (up to 8) to h1632 on pins ht1632_data, ht1632_wrclk Chip is assumed to already be chip-selected
//Bits are shifted out from MSB to LSB, with the first bit sent being (bits & firstbit), shifted till firsbit is zero.
void ht1632_writebits (byte bits, byte firstbit)
{
  DEBUGPRINT(" ");
  while (firstbit) {
    DEBUGPRINT((bits & firstbit ? "1" : "0"));
    digitalWrite(ht1632_wrclk, LOW);
    if (bits & firstbit) {
      digitalWrite(ht1632_data, HIGH);
    }
    else {
      digitalWrite(ht1632_data, LOW);
    }
    digitalWrite(ht1632_wrclk, HIGH);
    firstbit >>= 1;
  }
}


// ht1632_sendcmd
// Send a command to the ht1632 chip. A command consists of a 3-bit "CMD" ID, an 8bit command, and one "don't care bit".
//   Select 1 0 0 c7 c6 c5 c4 c3 c2 c1 c0 xx Free
static void ht1632_sendcmd (byte d, byte command)
{
  ht1632_chipselect(ht1632_cs[d]);        // Select chip
  ht1632_writebits(HT1632_ID_CMD, 1 << 2); // send 3 bits of id: COMMMAND
  ht1632_writebits(command, 1 << 7);      // send the actual command
  ht1632_writebits(0, 1);             // one extra dont-care bit in commands.
  ht1632_chipfree(ht1632_cs[d]);          //done
}


//ht1632_senddata
//send a nibble (4 bits) of data to a particular memory location of the
//ht1632.  The command has 3 bit ID, 7 bits of address, and 4 bits of data.
//   Select 1 0 1 A6 A5 A4 A3 A2 A1 A0 D0 D1 D2 D3 Free
//Note that the address is sent MSB first, while the data is sent LSB first!
//This means that somewhere a bit reversal will have to be done to get
//zero-based addressing of words and dots within words.
static void ht1632_senddata (byte d, byte address, byte data)
{
  ht1632_chipselect(ht1632_cs[d]);      // Select chip
  ht1632_writebits(HT1632_ID_WR, 1 << 2); // Send ID: WRITE to RAM
  ht1632_writebits(address, 1 << 6);    // Send address
  ht1632_writebits(data, 1 << 3);       // Send 4 bits of data
  ht1632_chipfree(ht1632_cs[d]);        // Done.
}


//ht1632_setup
//setup the ht1632 chips
void ht1632_setup()
{
  for (byte d = 0; d < NUM_DISPLAYS; d++) {
    pinMode(ht1632_cs[d], OUTPUT);

    digitalWrite(ht1632_cs[d], HIGH);  // Unselect (active low)

    pinMode(ht1632_wrclk, OUTPUT);
    pinMode(ht1632_data, OUTPUT);

    ht1632_sendcmd(d, HT1632_CMD_SYSON);    // System on
    ht1632_sendcmd(d, HT1632_CMD_LEDON);    // LEDs on
    ht1632_sendcmd(d, HT1632_CMD_COMS01);   // NMOS Output 24 row x 24 Com mode

    for (byte i = 0; i < 128; i++)
      ht1632_senddata(d, i, 0);  // clear the display!
  }
}


//we keep a copy of the display controller contents so that we can know which bits are on without having to (slowly) read the device.
//Note that we only use the low four bits of the shadow ram, since we're shadowing 4-bit memory.  This makes things faster, and we
//use the other half for a "snapshot" when we want to plot new data based on older data...
byte ht1632_shadowram[NUM_DISPLAYS * 96];  // our copy of the display's RAM


//plot a point on the display, with the upper left hand corner being (0,0).
//Note that Y increases going "downward" in contrast with most mathematical coordiate systems, but in common with many displays
//No error checking; bad things may happen if arguments are out of bounds!  (The ASSERTS compile to nothing by default
void ht1632_plot (char x, char y, char val)
{

  char addr, bitval;

  ASSERT(x >= 0);
  ASSERT(x <= X_MAX);
  ASSERT(y >= 0);
  ASSERT(y <= y_MAX);

  byte d;
  //select display depending on plot values passed in
  if (x >= 0 && x <= 23 ) {
    d = 0;
  }
  if (x >= 24 && x <= 47) {
    d = 1;
    x = x - 24;
  }

  /*
     The 4 bits in a single memory word go DOWN, with the LSB (first transmitted) bit being on top.  However, writebits()
     sends the MSB first, so we have to do a sort of bit-reversal somewhere.  Here, this is done by shifting the single bit in
     the opposite direction from what you might expect.
  */

  bitval = 8 >> (y & 3); // compute which bit will need set

  addr = (x << 2) + (y >> 2); // compute which memory word this is in

  if (val) {  // Modify the shadow memory
    ht1632_shadowram[(d * 96)  + addr] |= bitval;
  }
  else {
    ht1632_shadowram[(d * 96) + addr] &= ~bitval;
  }
  // Now copy the new memory value to the display
  ht1632_senddata(d, addr, ht1632_shadowram[(d * 96) + addr]);
}


//get_shadowram
//return the value of a pixel from the shadow ram.
byte get_shadowram(byte x, byte y)
{
  byte addr, bitval, d;

  //select display depending on plot values passed in
  if (x >= 0 && x <= 23 ) {
    d = 0;
  }
  if (x >= 24 && x <= 47) {
    d = 1;
    x = x - 24;
  }

  bitval = 8 >> (y & 3); // compute which bit will need set
  addr = (x << 2) + (y >> 2);   // compute which memory word this is in
  return (0 != (ht1632_shadowram[(d * 96) + addr] & bitval));
}


//snapshot_shadowram
//Copy the shadow ram into the snapshot ram (the upper bits)
//This gives us a separate copy so we can plot new data while
//still having a copy of the old data.  snapshotram is NOT
//updated by the plot functions (except "clear")
void snapshot_shadowram()
{
  for (byte i = 0; i < sizeof ht1632_shadowram; i++) {
    ht1632_shadowram[i] = (ht1632_shadowram[i] & 0x0F) | ht1632_shadowram[i] << 4;  // Use the upper bits
  }

}

//get_snapshotram
//get a pixel value from the snapshot ram (instead of
//the actual displayed (shadow) memory
byte get_snapshotram(byte x, byte y)
{

  byte addr, bitval;
  byte d = 0;

  //select display depending on plot values passed in
  if (x >= 24 && x <= 47) {
    d = 1;
    x = x - 24;
  }

  bitval = 128 >> (y & 3); // user upper bits!
  addr = (x << 2) + (y >> 2); // compute which memory word this is in
  if (ht1632_shadowram[(d * 96) + addr] & bitval)
    return 1;
  return 0;
}


//ht1632_clear
//clear the display, and the shadow memory, and the snapshot
//memory.  This uses the "write multiple words" capability of
//the chipset by writing all 96 words of memory without raising
//the chipselect signal.
void ht1632_clear()
{
  char i;
  for (byte d = 0; d < NUM_DISPLAYS; d++)
  {
    ht1632_chipselect(ht1632_cs[d]);  // Select chip
    ht1632_writebits(HT1632_ID_WR, 1 << 2); // send ID: WRITE to RAM
    ht1632_writebits(0, 1 << 6); // Send address
    for (i = 0; i < 96 / 2; i++) // Clear entire display
      ht1632_writebits(0, 1 << 7); // send 8 bits of data
    ht1632_chipfree(ht1632_cs[d]); // done
    for (i = 0; i < 96; i++)
      ht1632_shadowram[96 * d + i] = 0;
  }
}


/*
   fade_down
   fade the display to black
*/
void fade_down() {
  char intensity;
  for (intensity = brightness; intensity >= 0; intensity--) {
    ht1632_sendcmd(0, HT1632_CMD_PWM + intensity); //send intensity commands using CS0 for display 0
    ht1632_sendcmd(1, HT1632_CMD_PWM + intensity); //send intensity commands using CS0 for display 1
    delay(FADEDELAY);
  }
  //clear the display and set it to full brightness again so we're ready to plot new stuff
  cls();
  ht1632_sendcmd(0, HT1632_CMD_PWM + brightness);
  ht1632_sendcmd(1, HT1632_CMD_PWM + brightness);
}


/*
   fade_up
   fade the display up to full brightness
*/
void fade_up() {
  char intensity;
  for ( intensity = 0; intensity < brightness; intensity++) {
    ht1632_sendcmd(0, HT1632_CMD_PWM + intensity); //send intensity commands using CS0 for display 0
    ht1632_sendcmd(1, HT1632_CMD_PWM + intensity); //send intensity commands using CS0 for display 1
    delay(FADEDELAY);
  }
}


/* ht1632_putchar
   Copy a 5x7 character glyph from the myfont data structure to display memory, with its upper left at the given coordinate
   This is unoptimized and simply uses plot() to draw each dot.
*/
void ht1632_putchar(byte x, byte y, char c)
{

  byte dots;
  //  if (c >= 'A' && c <= 'Z' || (c >= 'a' && c <= 'z') ) {
  //    c &= 0x1F;   // A-Z maps to 1-26
  //  }
  if (c >= 'A' && c <= 'Z' ) {
    c &= 0x1F;   // A-Z maps to 1-26
  }
  else if (c >= 'a' && c <= 'z') {
    c = (c - 'a') + 41;   // A-Z maps to 41-67
  }
  else if (c >= '0' && c <= '9') {
    c = (c - '0') + 31;
  }
  else if (c == ' ') {
    c = 0; // space
  }
  else if (c == '.') {
    c = 27; // full stop
  }
  else if (c == '/') {
    c = 28; // single quote mark
  }
  else if (c == ':') {
    c = 29; // clock_mode selector arrow
  }
  else if (c == '>') {
    c = 30; // clock_mode selector arrow
  }
  else if (c == '#') {
    c = 67; // grad celsius
  }
  else if (c == ';') {
    c = 68; // ä
  }
  else if (c == ',') {
    c = 69; // ü
  }


  for (char col = 0; col < 5; col++) {
    dots = pgm_read_byte_near(&myfont[c][col]);
    for (char row = 0; row < 7; row++) {
      //check coords are on screen before trying to plot
      if ((x >= 0) && (x <= X_MAX) && (y >= 0) && (y <= Y_MAX)) {

        if (dots & (64 >> row)) {   // only 7 rows.
          plot(x + col, y + row, 1);
        } else {
          plot(x + col, y + row, 0);
        }
      }
    }
  }
}

//ht1632_putbigchar
//Copy a 6x14 character glyph from the myfont data structure to display memory, with its upper left at the given coordinate
//This is unoptimized and simply uses plot() to draw each dot.
void ht1632_putbigchar(byte x, byte y, char c)
{
  byte dots;
  if (c >= 'A' && c <= 'Z' || (c >= 'a' && c <= 'z') ) {
    return;   //return, as the 10x14 font contains only numeric characters
  }
  if (c >= '0' && c <= '9') {
    c = (c - '0');
    c &= 0x1F;
  }

  for (byte col = 0; col < 6; col++) {
    dots = pgm_read_byte_near(&mybigfont[c][col]);
    for (char row = 0; row < 8; row++) {
      if (dots & (128 >> row))
        plot(x + col, y + row, 1);
      else
        plot(x + col, y + row, 0);
    }

    dots = pgm_read_byte_near(&mybigfont[c][col + 6]);
    for (char row = 0; row < 8; row++) {
      if (dots & (128 >> row))
        plot(x + col, y + row + 8, 1);
      else
        plot(x + col, y + row + 8, 0);
    }
  }
}



// ht1632_puttinychar
// Copy a 3x5 character glyph from the myfont data structure to display memory, with its upper left at the given coordinate
// This is unoptimized and simply uses plot() to draw each dot.
void ht1632_puttinychar(byte x, byte y, char c)
{
  byte dots;
  if (c >= 'A' && c <= 'Z' || (c >= 'a' && c <= 'z') ) {
    c &= 0x1F;   // A-Z maps to 1-26
  }
  else if (c >= '0' && c <= '9') {
    c = (c - '0') + 31;
  }
  else if (c == ' ') {
    c = 0; // space
  }
  else if (c == '.') {
    c = 27; // full stop
  }
  else if (c == '/') {
    c = 28; // single quote mark
  }
  else if (c == '!') {
    c = 29; // single quote mark
  }
  else if (c == '?') {
    c = 30; // single quote mark
  }
  // add symbols
  else if (c == '-') {
    c = 41;    // minus
  }
  else if (c == '>') {
    c = 42;    // arrow
  }

  for (byte col = 0; col < 3; col++) {
    dots = pgm_read_byte_near(&mytinyfont[c][col]);
    for (char row = 0; row < 5; row++) {
      if (dots & (16 >> row))
        plot(x + col, y + row, 1);
      else
        plot(x + col, y + row, 0);
    }
  }
}

//ht1632_puticonchar
//Copy a 11x11 character glyph from the myfont data structure to display memory, with its upper left at the given coordinate
//This is unoptimized and simply uses plot() to draw each dot.
void ht1632_puticonchar(byte x, byte y, char c)
{
  byte dots;
  if (c == ' ') {
    c = 0; // space
  }
  else if (c == '1') {
    c = 1; // Sun
  }
  else if (c == '2') {
    c = 2; // Moon
  }
  else if (c == '3') {
    c = 3; // Ghost
  }
  else if (c == '4') {
    c = 4; // Scull
  }
  else if (c == '5') {
    c = 5; // B-Day Present
  }
  else if (c == '6') {
    c = 6; // XMAS Tree
  }
  else if (c == '7') {
    c = 7; // Dont Panic
  }
  else if (c == '8') {
    c = 8; // Schniblo
  }
  else if (c == '9') {
    c = 9; // Kitty
  }
  else if (c == 'a') {
    c = 10; // Beer
  }

  for (byte col = 0; col < 11; col++) {
    dots = pgm_read_byte_near(&myiconfont[c][col]);
    for (char row = 0; row < 8; row++) {
      if (dots & (128 >> row))
        plot(x + col, y + row, 1);
      else
        plot(x + col, y + row, 0);
    }

    dots = pgm_read_byte_near(&myiconfont[c][col + 11]);
    for (char row = 0; row < 8; row++) {
      if (dots & (128 >> row))
        plot(x + col, y + row + 8, 1);
      else
        plot(x + col, y + row + 8, 0);
    }
  }
}

//ht1632_invaders
//Copy the 8x12 character glyph from the myfont data structure to display memory, with its upper left at the given coordinate
//This is unoptimized and simply uses plot() to draw each dot.
void ht1632_invaders(byte x, byte y, char c)
{

  byte dots;
  if (c == ' ') {
    c = 0; // space
  }
  else if (c >= -9 && c <= -2) {
    c *= -1;
  }


  for (char col = 0; col < 12; col++) {
    dots = pgm_read_byte_near(&myinvaders[c][col]);
    for (char row = 0; row < 8; row++) {
      //check coords are on screen before trying to plot
      if ((x >= 0) && (x <= X_MAX) && (y >= 0) && (y <= Y_MAX)) {

        if (dots & (128 >> row)) {   // only 8 rows.
          plot(x + col, y + row, 1);
        } else {
          plot(x + col, y + row, 0);
        }
      }
    }
  }
}

//ht1632_putbinarychar
//Copy a 3x16 character glyph from the myfont data structure to display memory, with its upper left at the given coordinate
//This is unoptimized and simply uses plot() to draw each dot.
void ht1632_putbinarychar(byte x, byte y, char c)
{
  byte dots;
  if (c >= 'A' && c <= 'Z' || (c >= 'a' && c <= 'z') ) {
    return;   //return, as the 10x14 font contains only numeric characters
  }
  if (c >= '0' && c <= '9') {
    c = (c - '0');
    c &= 0x1F;
  }

  for (byte col = 0; col < 3; col++) {
    dots = pgm_read_byte_near(&mybinaryfont[c][col]);
    for (char row = 0; row < 8; row++) {
      if (dots & (128 >> row))
        plot(x + col, y + row, 1);
      else
        plot(x + col, y + row, 0);
    }

    dots = pgm_read_byte_near(&mybinaryfont[c][col + 3]);
    for (char row = 0; row < 8; row++) {
      if (dots & (128 >> row))
        plot(x + col, y + row + 8, 1);
      else
        plot(x + col, y + row + 8, 0);
    }
  }
}


//************ CLOCK MODES ***************


void invaders()
{

  byte hours = 0;
  byte mins  = 0;
  byte old_mins = 100;   // holds old seconds value - from last time seconds were updated o display - used to check if seconds have changed

  cls();


  // run clock main loop as long as run_mode returns true
  while (run_mode()) {

    // check buttons
    if (buttonA.wasPressed()) {
      switch_mode();
      return; // exit this mode back to loop(), so the new clock mode function is called
    }
    if (buttonB.wasPressed()) {
      display_date();
      fade_down();
      return; // exit this mode back to loop(), then we re-enter the function and the clock mode starts again. This refreshes the display.
    }


    // get the time from the clock chip
    get_time();

    // udate mins and hours with the new time
    mins  = rtc[1];
    hours = rtc[2];

    //print "Score"
    byte i = 0;
    char text[9] = "SCORE 00";
    while (text[i]) {
      ht1632_puttinychar((i * 4), 0, text[i]);
      i++;
    }
    plot(20, 1, 1);
    plot(20, 3, 1);


    // do 12/24 hour conversion if ampm set to 1
    if (hours > 12) {
      hours = hours - ampm * 12;
    }
    if (hours < 1) {
      hours = hours + ampm * 12;
    }

    // if mins changed then update them on the display
    if (mins != old_mins) {

      char buffer[3];


      // convert hours to string
      itoa(hours, buffer, 10);

      // fix - as otherwise if num has leading zero, e.g. "03" hours, itoa coverts this to chars with space "3 "
      if (hours < 10) {
        buffer[1] = buffer[0];
        // if we are in 12 hour mode blank the leading zero
        if (ampm) {
          buffer[0] = '0';
        }
        else {
          buffer[0] = '0';
        }
      }

      // update the display
      ht1632_puttinychar(32, 0, buffer[0]);
      ht1632_puttinychar(36, 0, buffer[1]);

      // convert mins to string
      itoa(mins, buffer, 10);

      // fix - as otherwise if num has leading zero, e.g. "03" mins, itoa converts this to chars with space "3 "
      if (mins < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }

      // update the display
      ht1632_puttinychar(40, 0, buffer[0]);
      ht1632_puttinychar(44, 0, buffer[1]);

      old_mins = mins;
    }
    byte invader_typ = random(1, 4);

    // scroll from left to right
    if (! scroll_invader(7, 0, 36, invader_typ)) {
      return;
    }
    // then, scroll from right to left
    if (! scroll_invader(7, 36, 0, invader_typ)) {
      return;
    }

    delay(50);
  }
  fade_down();
}


// (called by invaders)
boolean scroll_invader (byte ypos, char xstart, char xend, byte invader_type)
{
  boolean wiggle = false;
  char xstep;
  char xreset;

  if (xstart < xend) {
    xstep  = 1;
    xreset = -1;
  }
  else {
    xstep  = -1;
    xreset = 12;
  }

  for (char i = xstart; i != (xend + xstep); i += xstep) {

    // check buttons
    if (buttonA.uniquePress() || buttonB.uniquePress()) {
      return false;
    }

    display_invader(i, ypos, invader_type, wiggle);
    wiggle = (!wiggle);

    // display off the column at the left or the right of the glyph (for moving effect)
    for (byte y = ypos; y < ypos + 8; y++) {
      // check coords are on screen before trying to plot
      if ((i + xreset >= 0) && (i + xreset <= X_MAX) && (y >= 0) && (y <= Y_MAX)) {
        plot(i + xreset, y, 0);
      }
    }

    delay(100);


  }
  return true;
}


// (called by scroll_invader)
void display_invader (byte xpos, byte ypos, byte invader_type, boolean wiggle)
{
  // base invaders are at: -67, -71, -75 wiggle is -2 from each
  char invader = -1 - (invader_type * 2);
  if (wiggle) {
    invader -= 1;
  }

  // update the display
  ht1632_invaders(xpos, ypos, invader);
  //  ht1632_invaders(xpos+5, ypos, invader-1);
}



// digits()
// show the time in 6x14 characters and update it whilst the loop runs
void digits()
{
  cls();

  char buffer[3];   //for int to char conversion to turn rtc values into chars we can print on screen
  byte offset = 0;  //used to offset the x postition of the digits and centre the display when we are in 12 hour mode and the clock shows only 3 digits. e.g. 3:21
  byte x, y;        //used to draw a clear box over the left hand "1" of the display when we roll from 12:59 -> 1:00am in 12 hour mode.

  //do 12/24 hour conversion if ampm set to 1
  byte hours = rtc[2];
  if (hours > 12) {
    hours = hours - ampm * 12;
  }
  if (hours < 1) {
    hours = hours + ampm * 12;
  }

  //set the next minute we show the date at
  set_next_date();

  // initially set mins to value 100 - so it wll never equal rtc[1] on the first loop of the clock, meaning we draw the clock display when we enter the function
  byte secs = rtc[0]; //seconds
  byte old_secs = secs; //holds old seconds value - from last time seconds were updated o display - used to check if seconds have changed
  byte mins = 100;
  int count = 0;

  //run clock main loop as long as run_mode returns true
  while (run_mode()) {

    //get the time from the clock chip
    get_time();

    //check to see if the buttons have been pressed
    if (buttonA.uniquePress()) {
      switch_mode(); // pick the new mode via the menu
      return; //exit this mode back to loop(), so the new clock mode function is called.
    }
    if (buttonB.uniquePress()) {
      display_date();
      fade_down();
      return; //exit the mode back to loop(), Then we reenter the function and the clock mode starts again. This refreshes the display
    }
    if (is_special_event()) {
      fade_down();
      return; // exit this mode back to loop(), then we re-enter the function and the clock mode starts again. This refreshes the display.
    }

    //check whether it's time to automatically display the date
    check_show_date();

    //draw the flashing : as on if the secs have changed.
    if (secs != rtc[0]) {

      //update secs with new value
      secs = rtc[0];

      //draw :
      plot (20 - offset, 13, 1); //bottom point
      plot (20 - offset, 14, 1);
      plot (21 - offset, 13, 1);
      plot (21 - offset, 14, 1);
      count = 400;
    }

    //if count has run out, turn off the :
    if (count == 0) {
      plot (20 - offset, 13, 0); //bottom point
      plot (20 - offset, 14, 0);
      plot (21 - offset, 13, 0);
      plot (21 - offset, 14, 0);
    }
    else {
      count--;
    }


    //re draw the display if mins != rtc[1] i.e. if the time has changed from what we had stored in mins, (also trigggered on first entering function when mins is 100)
    if (mins != rtc[1]) {

      //update secs, mins and hours with the new values
      secs = rtc[0];
      mins = rtc[1];
      hours = rtc[2];

      //adjust hours of ampm set to 12 hour mode
      if (hours > 12) {
        hours = hours - ampm * 12;
      }
      if (hours < 1) {
        hours = hours + ampm * 12;
      }

      itoa(hours, buffer, 10);

      //if hours < 10 the num e.g. "3" hours, itoa coverts this to chars with space "3 " which we dont want
      if (hours < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }

      //print hours
      //if we in 12 hour mode and hours < 10, then don't print the leading zero, and set the offset so we centre the display with 3 digits.
      if (ampm && hours < 10) {
        offset = 5;

        //if the time is 1:00am clear the entire display as the offset changes at this time and we need to blank out the old 12:59
        if ((hours == 1 && mins == 0) ) {
          cls();
        }
      }
      else {
        //else no offset and print hours tens digit
        offset = 0;

        //if the time is 10:00am clear the entire display as the offset changes at this time and we need to blank out the old 9:59
        if (hours == 10 && mins == 0) {
          cls();
        }


        ht1632_putbigchar(4,  1, buffer[0]);
      }
      //print hours ones digit
      ht1632_putbigchar(12 - offset, 1, buffer[1]);


      //print mins
      //add leading zero if mins < 10
      itoa (mins, buffer, 10);
      if (mins < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }
      //print mins tens and ones digits
      ht1632_putbigchar(24 - offset, 1, buffer[0]);
      ht1632_putbigchar(32 - offset, 1, buffer[1]);
    }
    

    //if secs changed then update them on the display
    secs = rtc[0];
    if (secs != old_secs) {

      //secs
      char buffer[3];
      itoa(secs, buffer, 10);

      //fix - as otherwise if num has leading zero, e.g. "03" secs, itoa coverts this to chars with space "3 ".
      if (secs < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }

      ht1632_puttinychar(41 - offset, 2, buffer[0]); //seconds
      ht1632_puttinychar(41 - offset, 9, buffer[1]); //seconds
      old_secs = secs;
    }
  }
  fade_down();
}



// play pong - using the score as the time
void pong() {

  float ballpos_x, ballpos_y;
  byte erase_x = 10;  //holds ball old pos so we can erase it, set to blank area of screen initially.
  byte erase_y = 10;
  float ballvel_x, ballvel_y;
  int bat1_y = 5;  //bat starting y positions
  int bat2_y = 5;
  int bat1_target_y = 5;  //bat targets for bats to move to
  int bat2_target_y = 5;
  byte bat1_update = 1;  //flags - set to update bat position
  byte bat2_update = 1;
  byte bat1miss, bat2miss; //flags set on the minute or hour that trigger the bats to miss the ball, thus upping the score to match the time.
  byte restart = 1;   //game restart flag - set to 1 initially to setup 1st game
  byte bat_start_range; //y range that bat can be in so it hits the ball - used when we know we need to hit the ball, but want some randomness as to which part of the 6 pixel bat is hit
  byte bat_end_range;

  //show "play pong" message and draw pitch line
  pong_setup();

  //run clock main loop as long as run_mode returns true
  while (run_mode()) {

    if (buttonA.uniquePress()) {
      switch_mode();
      return;
    }
    if (buttonB.uniquePress()) {
      display_date();
      fade_down();
      return;
    }
    if (is_special_event()) {
      fade_down();
      return; // exit this mode back to loop(), then we re-enter the function and the clock mode starts again. This refreshes the display.
    }

    //if restart flag is 1, setup a new game
    if (restart) {

      //check whether it's time to automatically display the date
      if (check_show_date()) {
        //if date was displayed, we need to redraw the entire display, so run the setup routine
        pong_setup();
      }

      //erase ball pos
      plot (erase_x, erase_y, 0);

      //update score / time
      byte mins = rtc[1];
      byte hours = rtc[2];
      if (hours > 12) {
        hours = hours - ampm * 12;
      }
      if (hours < 1) {
        hours = hours + ampm * 12;
      }

      char buffer[3];

      itoa(hours, buffer, 10);
      //fix  - as otherwise if num has leading zero, e.g. "03" hours, itoa coverts this to chars with space "3 ".
      if (hours < 10) {
        buffer[1] = buffer[0];
        //uncomment the next 5 lines that are commented out if you want to blank the leading hours zero in pong if in 12 hour mode.
        //if (ampm) {
        //  buffer[0] = ' ';
        //}
        //else{
        buffer[0] = '0';
        //}
      }
      ht1632_puttinychar(14, 0, buffer[0] );
      ht1632_puttinychar(18, 0, buffer[1]);

      itoa(mins, buffer, 10);
      if (mins < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }
      ht1632_puttinychar(28, 0, buffer[0]);
      ht1632_puttinychar(32, 0, buffer[1]);

      //set ball start pos
      ballpos_x = 23;
      ballpos_y = random (4, 12);

      //pick random ball direction
      if (random(0, 2) > 0) {
        ballvel_x = 1;
      }
      else {
        ballvel_x = -1;
      }
      if (random(0, 2) > 0) {
        ballvel_y = 0.5;
      }
      else {
        ballvel_y = -0.5;
      }
      //draw bats in initial positions
      bat1miss = 0;
      bat2miss = 0;
      //reset game restart flag
      restart = 0;

      //short wait before we launch the ball for effect
      delay(400);
    }

    //get the time from the rtc
    get_time();

    //if coming up to the minute: secs = 59 and mins < 59, flag bat 2 (right side) to miss the return so we inc the minutes score
    if (rtc[0] == 59 && rtc[1] < 59) {
      bat1miss = 1;
    }
    // if coming up to the hour: secs = 59  and mins = 59, flag bat 1 (left side) to miss the return, so we inc the hours score.
    if (rtc[0] == 59 && rtc[1] == 59) {
      bat2miss = 1;
    }


    //AI - we run 2 sets of 'AI' for each bat to work out where to go to hit the ball back

    //very basic AI...
    // For each bat, First just tell the bat to move to the height of the ball when we get to a  location.
    //for bat1
    if (ballpos_x == random(30, 41)) { // && ballvel_x < 0) {
      bat1_target_y = ballpos_y;
    }
    //for bat2
    if (ballpos_x == random(8, 19)) { //  && ballvel_x > 0) {
      bat2_target_y = ballpos_y;
    }

    //when the ball is closer to the left bat, run the ball maths to find out where the ball will land
    if (ballpos_x == 23 && ballvel_x < 0) {

      byte end_ball_y = pong_get_ball_endpoint(ballpos_x, ballpos_y, ballvel_x, ballvel_y);

      //if the miss flag is set,  then the bat needs to miss the ball when it gets to end_ball_y
      if (bat1miss == 1) {
        bat1miss = 0;
        if ( end_ball_y > 8) {
          bat1_target_y = random (0, 3);
        }
        else {
          bat1_target_y = 8 + random (0, 3);
        }
      }

      //if the miss flag isn't set,  set bat target to ball end point with some randomness so its not always hitting top of bat
      else {

        if (end_ball_y <= 5) {
          bat_start_range = 0;
        } else {
          bat_start_range = end_ball_y - 5;
        }

        //bat end range. if ball <= 10 end range = ball end. else ball end = 10
        if (end_ball_y <= 10) {
          bat_end_range = end_ball_y;
        } else {
          bat_end_range = 10;
        }

        bat1_target_y = random(bat_start_range, bat_end_range + 1);
      }
    }


    //right bat AI
    //if positive velocity then predict for right bat - first just match ball height

    //when the ball is closer to the right bat, run the ball maths to find out where it will land
    if (ballpos_x == 25 && ballvel_x > 0) {

      byte end_ball_y = pong_get_ball_endpoint(ballpos_x, ballpos_y, ballvel_x, ballvel_y);

      //if flag set to miss, move bat out way of ball
      if (bat2miss == 1) {
        bat2miss = 0;
        //if ball end point above 8 then move bat down, else move it up- so either way it misses
        if (end_ball_y > 8) {
          bat2_target_y = random (0, 3);
        } else {
          bat2_target_y = 8 + random (0, 3);
        }
      } else {

        //set bat target range - i.e. the range of coords bat can be at to where it will still hit the ball.
        // We pick a random value in this range so the ball doesn't always hit the top pixel of the bat
        //bat is 6 pixels long. values below are top pixel of bat and take into account it's 6 pixel length

        // if  ball end 0 bat end pos range = 0 to 0
        // if  ball end 1 bat end pos = 0 to 1
        // if  ball end 2 bat end pos = 0 to 2
        // if  ball end 3 bat end pos = 0 to 3
        // if  ball end 4 bat end pos = 0 to 4
        // if  ball end 5 bat end pos = 0 to 5
        // if  ball end 6 bat end pos = 1 to 6
        // if  ball end 7 bat end pos = 2 up 7
        // if  ball end 8 bat end pos = 3 to 8
        // if  ball end 9 bat end pos = 4 to 9
        // if  ball end 10 bat end pos = 5 to 10
        // if  ball end 11 bat end pos = 6 to 10
        // if  ball end 12 bat end pos = 7 to 10
        // if  ball end 13 bat end pos = 8 to 10
        // if  ball end 14 bat end pos = 9 to 10
        // if  ball end 15 bat end pos = 10 to 10

        //bat start range. If ball <= 5 start range = 0. else range = ball end - 5
        if (end_ball_y <= 5) {
          bat_start_range = 0;
        } else {
          bat_start_range = end_ball_y - 5;
        }

        //bat end range. if ball <= 10 end range = ball end. else ball end = 10
        if (end_ball_y <= 10) {
          bat_end_range = end_ball_y;
        } else {
          bat_end_range = 10;
        }

        bat2_target_y = random(bat_start_range, bat_end_range + 1);
      }
    }


    //move bat 1 towards target
    //if bat y greater than target y move down until hit 0 (dont go any further or bat will move off screen)
    if (bat1_y > bat1_target_y && bat1_y > 0 ) {
      bat1_y--;
      bat1_update = 1;
    }

    //if bat y less than target y move up until hit 10 (as bat is 6)
    if (bat1_y < bat1_target_y && bat1_y < 10) {
      bat1_y++;
      bat1_update = 1;
    }

    //draw bat 1
    if (bat1_update) {
      for (byte i = 0; i < 16; i++) {
        if (i - bat1_y < 6 &&  i - bat1_y > -1) {
          plot(BAT1_X - 1, i , 1);
          plot(BAT1_X - 2, i , 1);
        }
        else {
          plot(BAT1_X - 1, i , 0);
          plot(BAT1_X - 2, i , 0);
        }
      }
    }


    //move bat 2 towards target

    //if bat y greater than target y move down until hit 0
    if (bat2_y > bat2_target_y && bat2_y > 0 ) {
      bat2_y--;
      bat2_update = 1;
    }

    //if bat y less than target y move up until hit max of 10 (as bat is 6)
    if (bat2_y < bat2_target_y && bat2_y < 10) {
      bat2_y++;
      bat2_update = 1;
    }

    //draw bat2
    if (bat2_update) {
      for (byte i = 0; i < 16; i++) {
        if (  i - bat2_y < 6 && i - bat2_y > -1) {
          plot(BAT2_X + 1, i , 1);
          plot(BAT2_X + 2, i , 1);
        }
        else {
          plot(BAT2_X + 1, i , 0);
          plot(BAT2_X + 2, i , 0);
        }
      }
    }

    //update the ball position using the velocity
    ballpos_x =  ballpos_x + ballvel_x;
    ballpos_y =  ballpos_y + ballvel_y;

    //check ball collision with top of screen and reverse the y velocity if hit
    if (ballpos_y <= 0 ) {
      ballvel_y = ballvel_y * -1;
      ballpos_y = 0; //make sure value goes no less that 0
    }

    //check ball collision with bottom of screen and reverse the y velocity if hit
    if (ballpos_y >= 15) {
      ballvel_y = ballvel_y * -1;
      ballpos_y = 15; //make sure value goes no more than 15
    }

    //check for ball collision with bat1. check ballx is same as batx
    //and also check if bally lies within width of bat i.e. baty to baty + 6. We can use the exp if(a < b && b < c)
    if ((int)ballpos_x == BAT1_X && (bat1_y <= (int)ballpos_y && (int)ballpos_y <= bat1_y + 5) ) {

      //random if bat flicks ball to return it - and therefor changes ball velocity
      if (!random(0, 3)) { //not true == no flick - just straight rebound and no change to ball y vel
        ballvel_x = ballvel_x * -1;
      }
      else {
        bat1_update = 1;
        byte flick;  //0 = up, 1 = down.

        if (bat1_y > 1 || bat1_y < 8) {
          flick = random(0, 2);  //pick a random dir to flick - up or down
        }

        //if bat 1 or 2 away from top only flick down
        if (bat1_y <= 1 ) {
          flick = 0;   //move bat down 1 or 2 pixels
        }
        //if bat 1 or 2 away from bottom only flick up
        if (bat1_y >=  8 ) {
          flick = 1;  //move bat up 1 or 2 pixels
        }

        switch (flick) {
          //flick up
          case 0:
            bat1_target_y = bat1_target_y + random(1, 3);
            ballvel_x = ballvel_x * -1;
            if (ballvel_y < 2) {
              ballvel_y = ballvel_y + 0.2;
            }
            break;

          //flick down
          case 1:
            bat1_target_y = bat1_target_y - random(1, 3);
            ballvel_x = ballvel_x * -1;
            if (ballvel_y > 0.2) {
              ballvel_y = ballvel_y - 0.2;
            }
            break;
        }
      }
    }


    //check for ball collision with bat2. check ballx is same as batx
    //and also check if bally lies within width of bat i.e. baty to baty + 6. We can use the exp if(a < b && b < c)
    if ((int)ballpos_x == BAT2_X && (bat2_y <= (int)ballpos_y && (int)ballpos_y <= bat2_y + 5) ) {

      //random if bat flicks ball to return it - and therefor changes ball velocity
      if (!random(0, 3)) {
        ballvel_x = ballvel_x * -1;    //not true = no flick - just straight rebound and no change to ball y vel
      }
      else {
        bat1_update = 1;
        byte flick;  //0 = up, 1 = down.

        if (bat2_y > 1 || bat2_y < 8) {
          flick = random(0, 2);  //pick a random dir to flick - up or down
        }
        //if bat 1 or 2 away from top only flick down
        if (bat2_y <= 1 ) {
          flick = 0;  //move bat up 1 or 2 pixels
        }
        //if bat 1 or 2 away from bottom only flick up
        if (bat2_y >=  8 ) {
          flick = 1;   //move bat down 1 or 2 pixels
        }

        switch (flick) {
          //flick up
          case 0:
            bat2_target_y = bat2_target_y + random(1, 3);
            ballvel_x = ballvel_x * -1;
            if (ballvel_y < 2) {
              ballvel_y = ballvel_y + 0.2;
            }
            break;

          //flick down
          case 1:
            bat2_target_y = bat2_target_y - random(1, 3);
            ballvel_x = ballvel_x * -1;
            if (ballvel_y > 0.2) {
              ballvel_y = ballvel_y - 0.2;
            }
            break;
        }
      }
    }

    //plot the ball on the screen
    byte plot_x = (int)(ballpos_x + 0.5f);
    byte plot_y = (int)(ballpos_y + 0.5f);

    //take a snapshot of all the led states
    snapshot_shadowram();

    //if the led at the ball pos is on already, dont bother printing the ball.
    if (get_shadowram(plot_x, plot_y)) {
      //erase old point, but don't update the erase positions, so next loop the same point will be erased rather than this point which shuldn't be
      plot (erase_x, erase_y, 0);
    }
    else {
      //else plot the ball and erase the old position
      plot (plot_x, plot_y, 1);
      plot (erase_x, erase_y, 0);
      //reset erase to new pos
      erase_x = plot_x;
      erase_y = plot_y;
    }

    //check if a bat missed the ball. if it did, reset the game.
    if ((int)ballpos_x == 0 || (int) ballpos_x == 47) {
      restart = 1;
    }
    delay(20);
  }
  fade_down();
}

//pong ball calculation
byte pong_get_ball_endpoint(float tempballpos_x, float  tempballpos_y, float  tempballvel_x, float tempballvel_y) {

  //run calculations until ball hits bat
  while (tempballpos_x > BAT1_X && tempballpos_x < BAT2_X  ) {
    tempballpos_x = tempballpos_x + tempballvel_x;
    tempballpos_y = tempballpos_y + tempballvel_y;

    //check for collisions with top / bottom
    if (tempballpos_y <= 0 ) {
      tempballvel_y = tempballvel_y * -1;
      tempballpos_y = 0; //make sure value goes no less that 0
    }

    if (tempballpos_y >= 15) {
      tempballvel_y = tempballvel_y * -1;
      tempballpos_y = 15; //make sure value goes no more than 15
    }
  }
  return tempballpos_y;
}


//play pong message and draw centre line
void pong_setup() {
  //setup
  cls();

  //uncomment for clear screen "wipe" effect
  //flashing_cursor (0,0,47,16,0);

  //uncomment for "Ready? Play Pong!" Message
  byte i = 0;
  char intro1[6] = "Ready";
  while (intro1[i]) {
    delay(25);
    ht1632_puttinychar((i * 4) + 12, 4, intro1[i]);
    i++;
  }
  //flashing question mark
  for (byte i = 0; i < 2; i++) {
    ht1632_puttinychar(34, 4, '?');
    delay(200);
    ht1632_puttinychar(34, 4, ' ');
    delay(400);
  }

  cls();
  i = 0;
  char intro2[11] = "Play Pong!";
  while (intro2[i]) {
    ht1632_puttinychar((i * 4) + 6, 4, intro2[i]);
    i++;
  }
  delay(800); //hold message for a moment
  cls();
  // end "Play Pong!" Message code

  get_time();

  //draw pitch centre line
  byte offset = random(0, 2); //offset top centre line randomly, so different led's get used.
  for (byte i = 0; i < 16; i++) {

    //uncomment for dotted centre line
    if ( i % 2 == 0 ) { //plot point if an even number
      plot(24, i + offset, 1);
      delay(30);
    }
    // end dotted centre line code

    // uncomment for solid pitch centre line if peferred
    //plot(24, i, 1);
  }

  delay(120); //delay for effect
}



//like normal but with slide effect
void slide() {

  byte digits_old[6] = {
    99, 99, 99, 99, 99, 99
  }; //old values  we store time in. Set to somthing that will never match the time initially so all digits get drawn wnen the mode starts
  byte digits_new[6]; //new digits time will slide to reveal
  byte digits_x_pos[6] = {
    42, 36, 24, 18, 6, 0
  }; //x pos for which to draw each digit at - last 2 are bottom line

  char old_char[2]; //used when we use itoa to transpose the current digit (type byte) into a char to pass to the animation function
  char new_char[2]; //used when we use itoa to transpose the new digit (type byte) into a char to pass to the animation function

  //old_chars - stores the 5 day and date suffix chars on the display. e.g. "mon" and "st". We feed these into the slide animation as the current char when these chars are updated.
  //We sent them as A initially, which are used when the clocl enters the mode and no last chars are stored.
  char old_chars[6] = "AAAAA";

  //plot the clock colons on the display - these don't change
  cls();
  ht1632_putchar( 12, 5, ':');
  ht1632_putchar( 30, 5, ':');

  byte old_secs = rtc[0]; //store seconds in old_secs. We compare secs and old secs. WHen they are different we redraw the display

  //run clock main loop as long as run_mode returns true
  while (run_mode()) {

    get_time();

    //check buttons
    if (buttonA.uniquePress()) {
      switch_mode();
      return;
    }
    if (buttonB.uniquePress()) {
      display_date();
      fade_down();
      return;
    }
    if (is_special_event()) {
      fade_down();
      return; // exit this mode back to loop(), then we re-enter the function and the clock mode starts again. This refreshes the display.
    }

    //if secs have changed then update the display
    if (rtc[0] != old_secs) {
      old_secs = rtc[0];

      //do 12/24 hour conversion if ampm set to 1
      byte hours = rtc[2];
      if (hours > 12) {
        hours = hours - ampm * 12;
      }
      if (hours < 1) {
        hours = hours + ampm * 12;
      }

      //split all date and time into individual digits - stick in digits_new array

      //rtc[0] = secs                        //array pos and digit stored
      digits_new[0] = (rtc[0] % 10);         //0 - secs ones
      digits_new[1] = ((rtc[0] / 10) % 10);  //1 - secs tens
      //rtc[1] = mins
      digits_new[2] = (rtc[1] % 10);         //2 - mins ones
      digits_new[3] = ((rtc[1] / 10) % 10);  //3 - mins tens
      //rtc[2] = hours
      digits_new[4] = (hours % 10);         //4 - hour ones
      digits_new[5] = ((hours / 10) % 10);  //5 - hour tens
      //rtc[4] = date
      //digits_new[6] = (rtc[4]%10);           //6 - date ones
      //digits_new[7] = ((rtc[4]/10)%10);      //7 - date tens

      //draw initial screen of all chars. After this we just draw the changes.

      //compare top line digits 0 to 5 (secs, mins and hours)
      for (byte i = 0; i <= 5; i++) {
        //see if digit has changed...
        if (digits_old[i] != digits_new[i]) {

          //run sequence for each in turn
          for (byte seq = 0; seq <= 8 ; seq++) {

            //convert digit to string
            itoa(digits_old[i], old_char, 10);
            itoa(digits_new[i], new_char, 10);

            //if set to 12 hour mode and we're on digit 5 (hours tens mode) then check to see if this is a zero. If it is, blank it instead so we get 2.00pm not 02.00pm
            if (ampm && i == 5) {
              if (digits_new[5] == 0) {
                new_char[0] = ' ';
              }
              if (digits_old[5] == 0) {
                old_char[0] = ' ';
              }
            }
            //draw the animation frame for each digit
            slideanim(digits_x_pos[i], 5, seq, old_char[0], new_char[0]);
            delay(SLIDE_DELAY);
          }
        }
      }//end do date line


      //save digita array tol old for comparison next loop
      for (byte i = 0; i <= 7; i++) {
        digits_old[i] =  digits_new[i];
      }
    }//secs/oldsecs
  }//while loop
  fade_down();
}


//called by slide
//this draws the animation of one char sliding on and the other sliding off. There are 8 steps in the animation, we call the function to draw one of the steps from 0-7
//inputs are are char x and y, animation frame sequence (0-7) and the current and new chars being drawn.
void slideanim(byte x, byte y, byte sequence, char current_c, char new_c) {

  //  To slide one char off and another on we need 9 steps or frames in sequence...

  //  seq# 0123456 <-rows of the display
  //   |   |||||||
  //  seq0 0123456  START - all rows of the display 0-6 show the current characters rows 0-6
  //  seq1  012345  current char moves down one row on the display. We only see it's rows 0-5. There are at display positions 1-6 There is a blank row inserted at the top
  //  seq2 6 01234  current char moves down 2 rows. we now only see rows 0-4 at display rows 2-6 on the display. Row 1 of the display is blank. Row 0 shows row 6 of the new char
  //  seq3 56 0123
  //  seq4 456 012  half old / half new char
  //  seq5 3456 01
  //  seq6 23456 0
  //  seq7 123456
  //  seq8 0123456  END - all rows show the new char

  //from above we can see...
  //currentchar runs 0-6 then 0-5 then 0-4 all the way to 0. starting Y position increases by 1 row each time.
  //new char runs 6 then 5-6 then 4-6 then 3-6. starting Y position increases by 1 row each time.

  //if sequence number is below 7, we need to draw the current char
  if (sequence < 7) {
    byte dots;
    // if (current_c >= 'A' &&  || (current_c >= 'a' && current_c <= 'z') ) {
    //   current_c &= 0x1F;   // A-Z maps to 1-26
    // }
    if (current_c >= 'A' && current_c <= 'Z' ) {
      current_c &= 0x1F;   // A-Z maps to 1-26
    }
    else if (current_c >= 'a' && current_c <= 'z') {
      current_c = (current_c - 'a') + 41;   // A-Z maps to 41-67
    }
    else if (current_c >= '0' && current_c <= '9') {
      current_c = (current_c - '0') + 31;
    }
    else if (current_c == ' ') {
      current_c = 0; // space
    }
    else if (current_c == '.') {
      current_c = 27; // full stop
    }
    else if (current_c == '\'') {
      current_c = 28; // single quote mark
    }
    else if (current_c == ':') {
      current_c = 29; //colon
    }
    else if (current_c == '>') {
      current_c = 30; // clock_mode selector arrow
    }

    byte curr_char_row_max = 6 - sequence; //the maximum number of rows to draw is 6 - sequence number
    byte start_y = sequence; //y position to start at - is same as sequence number. We inc this each loop

    //plot each row up to row maximum (calculated from sequence number)
    for (byte curr_char_row = 0; curr_char_row <= curr_char_row_max; curr_char_row++) {
      for (byte col = 0; col < 5; col++) {
        dots = pgm_read_byte_near(&myfont[current_c][col]);
        if (dots & (64 >> curr_char_row))
          plot(x + col, y + start_y, 1); //plot led on
        else
          plot(x + col, y + start_y, 0); //else plot led off
      }
      start_y++;//add one to y so we draw next row one down
    }
  }

  //draw a blank line between the characters if sequence is between 1 and 7. If we don't do this we get the remnants of the current chars last position left on the display
  if (sequence >= 1 && sequence <= 7) {
    for (byte col = 0; col < 5; col++) {
      plot(x + col, y + (sequence - 1), 0); //the y position to draw the line is equivalent to the sequence number - 1
    }
  }

  //if sequence is above 2, we also need to start drawing the new char
  if (sequence >= 2) {

    //work out char
    byte dots;
    //if (new_c >= 'A' && new_c <= 'Z' || (new_c >= 'a' && new_c <= 'z') ) {
    //  new_c &= 0x1F;   // A-Z maps to 1-26
    //}
    if (new_c >= 'A' && new_c <= 'Z' ) {
      new_c &= 0x1F;   // A-Z maps to 1-26
    }
    else if (new_c >= 'a' && new_c <= 'z') {
      new_c = (new_c - 'a') + 41;   // A-Z maps to 41-67
    }
    else if (new_c >= '0' && new_c <= '9') {
      new_c = (new_c - '0') + 31;
    }
    else if (new_c == ' ') {
      new_c = 0; // space
    }
    else if (new_c == '.') {
      new_c = 27; // full stop
    }
    else if (new_c == '\'') {
      new_c = 28; // single quote mark
    }
    else if (new_c == ':') {
      new_c = 29; // clock_mode selector arrow
    }
    else if (new_c == '>') {
      new_c = 30; // clock_mode selector arrow
    }

    byte newcharrowmin = 6 - (sequence - 2); //minimumm row num to draw for new char - this generates an output of 6 to 0 when fed sequence numbers 2-8. This is the minimum row to draw for the new char
    byte start_y = 0; //y position to start at - is same as sequence number. we inc it each row

    //plot each row up from row minimum (calculated by sequence number) up to 6
    for (byte newcharrow = newcharrowmin; newcharrow <= 6; newcharrow++) {
      for (byte col = 0; col < 5; col++) {
        dots = pgm_read_byte_near(&myfont[new_c][col]);
        if (dots & (64 >> newcharrow))
          plot(x + col, y + start_y, 1); //plot led on
        else
          plot(x + col, y + start_y, 0); //else plot led off
      }
      start_y++;//add one to y so we draw next row one down
    }
  }
}


//if the minute matches next_display_date then display the date, and set the next minute the date should be displayed
bool check_show_date() {

  if (rtc[1] == next_display_date) {
    fade_down();
    display_date();
    fade_down();
    set_next_date();
    return 1; //return true - the date was shown so run whatever setup you need (e.g redraw the pong pitch)
  }
  else {
    return 0; //the date wasn't shown.
  }

}



// normal clock
void normal() {

  char textchar[18]; // the 16 characters on the display
  byte mins = 100; //mins
  byte secs = rtc[0]; //seconds
  byte old_secs = secs; //holds old seconds value - from last time seconds were updated o display - used to check if seconds have changed

  cls();

  //run clock main loop as long as run_mode returns true
  while (run_mode()) {

    get_time();

    //check buttons
    if (buttonA.uniquePress()) {
      switch_mode();
      return;
    }
    if (buttonB.uniquePress()) {
      display_date();
      fade_down();
      return;
    }
    if (is_special_event()) {
      fade_down();
      return; // exit this mode back to loop(), then we re-enter the function and the clock mode starts again. This refreshes the display.
    }

    //if secs changed then update them on the display
    secs = rtc[0];
    if (secs != old_secs) {

      //secs
      char buffer[3];
      itoa(secs, buffer, 10);

      //fix - as otherwise if num has leading zero, e.g. "03" secs, itoa coverts this to chars with space "3 ".
      if (secs < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }

      ht1632_putchar( 29, 1, ':'); //seconds colon
      ht1632_putchar( 34, 1, buffer[0]); //seconds
      ht1632_putchar( 40, 1, buffer[1]); //seconds
      old_secs = secs;
    }

    //if minute changes change time
    if (mins != rtc[1]) {

      //reset these for comparison next time
      mins = rtc[1];
      byte hours = rtc[2];
      if (hours > 12) {
        hours = hours - ampm * 12;
      }
      if (hours < 1) {
        hours = hours + ampm * 12;
      }

      byte dow  = rtc[3]; // the DS1307 outputs 0 - 6 where 0 = Sunday0 - 6 where 0 = Sunday.
      byte date = rtc[4];
      byte month = rtc[5];
      int year = rtc[6];


      //set characters
      char buffer[3];
      itoa(hours, buffer, 10);

      //fix - as otherwise if num has leading zero, e.g. "03" hours, itoa coverts this to chars with space "3 ".
      if (hours < 10) {
        buffer[1] = buffer[0];
        //if we are in 12 hour mode blank the leading zero.
        if (ampm) {
          buffer[0] = ' ';
        }
        else {
          buffer[0] = '0';
        }
      }
      //set hours chars
      textchar[0] = buffer[0];
      textchar[1] = buffer[1];
      textchar[2] = ':';

      itoa (mins, buffer, 10);
      if (mins < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }
      //set mins characters
      textchar[3] = buffer[0];
      textchar[4] = buffer[1];

      //do seconds
      textchar[5] = ':';
      buffer[3];
      secs = rtc[0];
      itoa(secs, buffer, 10);

      //fix - as otherwise if num has leading zero, e.g. "03" secs, itoa coverts this to chars with space "3 ".
      if (secs < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }
      //set seconds
      textchar[6] = buffer[0];
      textchar[7] = buffer[1];

      //do date
      itoa (date, buffer, 10);
      if (date < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }

      textchar[8] = buffer[0];
      textchar[9] = buffer[1];
      textchar[10] = '.';

      //do month
      itoa (month, buffer, 10);
      if (month < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }

      textchar[11] = buffer[0];
      textchar[12] = buffer[1];
      textchar[13] = '.';
      textchar[14] = '2';
      textchar[15] = '0';

      //do year
      itoa (year, buffer, 10);

      textchar[16] = buffer[2];
      textchar[17] = buffer[3];

      byte x = 0;
      byte y = 0;

      //print each char

      ht1632_putchar( 2, 1, textchar[0]); //top line
      ht1632_putchar( 8, 1, textchar[1]); //top line
      ht1632_putchar( 13, 1, textchar[2]); //top line
      ht1632_putchar( 18, 1, textchar[3]); //top line
      ht1632_putchar( 24, 1, textchar[4]); //top line
      ht1632_putchar( 29, 1, textchar[5]); //top line
      ht1632_putchar( 34, 1, textchar[6]); //top line
      ht1632_putchar( 40, 1, textchar[7]); //top line

      for (byte x = 0; x < 10 ; x++) {
        ht1632_puttinychar( x * 4 + 4, 10, textchar[x + 8]); //bottom line
      }

    }
    delay(50);
  }
  fade_down();
}



// Binary clock
void binary() {

  char textchar[18]; // the 16 characters on the display
  byte mins = 100; //mins
  byte secs = rtc[0]; //seconds
  byte old_secs = secs; //holds old seconds value - from last time seconds were updated o display - used to check if seconds have changed

  cls();

  //run clock main loop as long as run_mode returns true
  while (run_mode()) {

    get_time();

    //check buttons
    if (buttonA.uniquePress()) {
      switch_mode();
      return;
    }
    if (buttonB.uniquePress()) {
      display_date();
      fade_down();
      return;
    }
    if (is_special_event()) {
      fade_down();
      return; // exit this mode back to loop(), then we re-enter the function and the clock mode starts again. This refreshes the display.
    }

    //if secs changed then update them on the display
    secs = rtc[0];
    if (secs != old_secs) {

      //secs
      char buffer[3];
      itoa(secs, buffer, 10);

      //fix - as otherwise if num has leading zero, e.g. "03" secs, itoa coverts this to chars with space "3 ".
      if (secs < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }

      ht1632_putbinarychar( 36, 0, buffer[0]); //seconds
      ht1632_putbinarychar( 43, 0, buffer[1]); //seconds
      old_secs = secs;
    }

    //if minute changes change time
    if (mins != rtc[1]) {

      //reset these for comparison next time
      mins = rtc[1];
      byte hours = rtc[2];
      if (hours > 12) {
        hours = hours - ampm * 12;
      }
      if (hours < 1) {
        hours = hours + ampm * 12;
      }

      byte dow  = rtc[3]; // the DS1307 outputs 0 - 6 where 0 = Sunday0 - 6 where 0 = Sunday.
      byte date = rtc[4];
      byte month = rtc[5];
      int year = rtc[6];


      //set characters
      char buffer[3];
      itoa(hours, buffer, 10);

      //fix - as otherwise if num has leading zero, e.g. "03" hours, itoa coverts this to chars with space "3 ".
      if (hours < 10) {
        buffer[1] = buffer[0];
        //if we are in 12 hour mode blank the leading zero.
        if (ampm) {
          buffer[0] = ' ';
        }
        else {
          buffer[0] = '0';
        }
      }
      //set hours chars
      textchar[0] = buffer[0];
      textchar[1] = buffer[1];

      itoa (mins, buffer, 10);
      if (mins < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }
      //set mins characters
      textchar[2] = buffer[0];
      textchar[3] = buffer[1];

      //do seconds
      buffer[3];
      secs = rtc[0];
      itoa(secs, buffer, 10);

      //fix - as otherwise if num has leading zero, e.g. "03" secs, itoa coverts this to chars with space "3 ".
      if (secs < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }
      //set seconds
      textchar[4] = buffer[0];
      textchar[5] = buffer[1];

      byte x = 0;
      byte y = 0;

      //print each char

      ht1632_putbinarychar( 2, 0, textchar[0]); //top line
      ht1632_putbinarychar( 9, 0, textchar[1]); //top line
      ht1632_putbinarychar( 19, 0, textchar[2]); //top line
      ht1632_putbinarychar( 26, 0, textchar[3]); //top line
      ht1632_putbinarychar( 36, 0, textchar[4]); //top line
      ht1632_putbinarychar( 43, 0, textchar[5]); //top line

    }
    delay(50);
  }
  fade_down();
}



// normal clock with temperature
void thermo() {

  char textchar[16];      //the 16 characters on the display
  char iconchar[1];       //the 1 character on the display
  byte hours = rtc[2];    //hours
  byte mins = 100;        //mins
  byte secs = rtc[0];     //seconds
  byte old_secs = secs;   //holds old seconds value - from last time seconds were updated o display - used to check if seconds have changed

  cls();


  //run clock main loop as long as run_mode returns true
  while (run_mode()) {


    plot(23, 2, 1);   //Top :
    plot(23, 4, 1);
    plot(35, 2, 1);   //Bottom :
    plot(35, 4, 1);


    temp[0] = map(analogRead(TMP36), 0, 410, -50, 150);
    delay(10);
    temp[1] = map(analogRead(TMP36), 0, 410, -50, 150);
    delay(10);
    temp[2] = map(analogRead(TMP36), 0, 410, -50, 150);
    delay(10);
    temp[3] = map(analogRead(TMP36), 0, 410, -50, 150);
    delay(10);
    temp[4] = map(analogRead(TMP36), 0, 410, -50, 150);
    delay(10);
    temp[5] = map(analogRead(TMP36), 0, 410, -50, 150);
    delay(10);
    temp[6] = map(analogRead(TMP36), 0, 410, -50, 150);
    delay(10);
    temp[7] = map(analogRead(TMP36), 0, 410, -50, 150);
    delay(10);
    temp[8] = map(analogRead(TMP36), 0, 410, -50, 150);
    delay(10);
    temp[9] = map(analogRead(TMP36), 0, 410, -50, 150);

    temperatur = (temp[0] + temp[1] + temp[2] + temp[3] + temp[4] + temp[5] + temp[6] + temp[7] + temp[8] + temp[9]) / 10 + tempcorr;


    get_time();


    //check buttons
    if (buttonA.uniquePress()) {
      switch_mode();
      return;
    }
    if (buttonB.uniquePress()) {
      display_date();
      fade_down();
      return;
    }
    if (is_special_event()) {
      fade_down();
      return; // exit this mode back to loop(), then we re-enter the function and the clock mode starts again. This refreshes the display.
    }

    //if secs changed then update them on the display
    secs = rtc[0];
    if (secs != old_secs) {

      //secs
      char buffer[3];
      itoa(secs, buffer, 10);

      //fix - as otherwise if num has leading zero, e.g. "03" secs, itoa coverts this to chars with space "3 ".
      if (secs < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }

      ht1632_puttinychar( 38, 1, buffer[0]); //seconds
      ht1632_puttinychar( 42, 1, buffer[1]); //seconds
      old_secs = secs;
    }

    //if minute changes change time
    if (mins != rtc[1]) {

      //reset these for comparison next time
      mins = rtc[1];
      byte hours = rtc[2];
      if (hours > 12) {
        hours = hours - ampm * 12;
      }
      if (hours < 1) {
        hours = hours + ampm * 12;
      }

      byte dow  = rtc[3]; // the DS1307 outputs 0 - 6 where 0 = Sunday0 - 6 where 0 = Sunday.
      byte date = rtc[4];
      byte month = rtc[5];
      int year = rtc[6];


      //set characters
      char buffer[3];
      itoa(hours, buffer, 10);

      //fix - as otherwise if num has leading zero, e.g. "03" hours, itoa coverts this to chars with space "3 ".
      if (hours < 10) {
        buffer[1] = buffer[0];
        //if we are in 12 hour mode blank the leading zero.
        if (ampm) {
          buffer[0] = ' ';
        }
        else {
          buffer[0] = '0';
        }
      }
      //set hours chars
      textchar[0] = buffer[0];
      textchar[1] = buffer[1];
      textchar[2] = ' ';

      itoa (mins, buffer, 10);
      if (mins < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }
      //set mins characters
      textchar[3] = buffer[0];
      textchar[4] = buffer[1];

      //do seconds
      textchar[5] = ' ';
      buffer[3];
      secs = rtc[0];
      itoa(secs, buffer, 10);

      //fix - as otherwise if num has leading zero, e.g. "03" secs, itoa coverts this to chars with space "3 ".
      if (secs < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }
      //set seconds
      textchar[6] = buffer[0];
      textchar[7] = buffer[1];



      textchar[8] = ' ';
      textchar[9] = ' ';


      itoa(temperatur, buffer, 10);

      //fix - as otherwise if num has leading zero, e.g. "03" secs, itoa coverts this to chars with space "3 ".
      if (temperatur < 10) {
        buffer[1] = buffer[0];
        buffer[0] = '0';
      }

      textchar[10] = buffer[0];
      textchar[11] = buffer[1];
      textchar[12] = '#';
      textchar[13] = 'C';
      textchar[14] = ' ';
      textchar[15] = ' ';

      byte x = 0;
      byte y = 0;

      //print each char
      for (byte x = 0; x < 8 ; x++) {
        ht1632_puttinychar( x * 4 + 14, 1, textchar[x]); //top line
      }


      //Icons
      if (date == 11 && month == 2) {
        iconchar[1] = '5'; // Draw B-Day Present
      }
      else if (date == 23 && month == 4) {
        iconchar[1] = 'a'; // Draw Beer
      }
      else if (date == 14 && month == 3) {
        iconchar[1] = '8'; // Draw Schniblo
      }
      else if (date == 25 && month == 5) {
        iconchar[1] = '7'; // Draw Dont Panic
      }
      else if (date == 8 && month == 8) {
        iconchar[1] = '9'; // Draw Kitty
      }
      else if (date == 24 && month == 12) {
        iconchar[1] = '6'; // Draw X-Mas Tree
      }
      else if (hours > 20) {
        iconchar[1] = '2'; // Draw Moon
      }
      else if (hours == 0 && mins == 0) {
        iconchar[1] = '3'; // Draw Ghost
      }
      else if (hours == 0 && mins == 13) {
        iconchar[1] = '4'; // Draw Scull for testing
      }
      else if (hours < 6) {
        iconchar[1] = '2'; // Draw Moon
      }
      else {
        iconchar[1] = '1'; // Draw Sun
      }

      //print each char anf icon
      ht1632_putchar( 18, 8, textchar[10]);   //bottom line
      ht1632_putchar( 24, 8, textchar[11]);   //bottom line
      ht1632_putchar( 30, 8, textchar[12]);   //bottom line
      ht1632_putchar( 36, 8, textchar[13]);   //bottom line
      ht1632_puticonchar( 1, 2, iconchar[1]); //left icon

    }
    delay(50);
  }
  fade_down();
}



//set the next minute the date should be automatically displayed
void set_next_date() {

  get_time();

  next_display_date = rtc[1] + random(NEXT_DATE_MIN, NEXT_DATE_MAX);
  //check we've not gone over 59
  if (next_display_date >= 59) {
    next_display_date = random(NEXT_DATE_MIN, NEXT_DATE_MAX);
  }
}



//display_date - print the day of week, date and month with a flashing cursor effect
void display_date()
{

  temp[0] = map(analogRead(TMP36), 0, 410, -50, 150);
  delay(10);
  temp[1] = map(analogRead(TMP36), 0, 410, -50, 150);
  delay(10);
  temp[2] = map(analogRead(TMP36), 0, 410, -50, 150);
  delay(10);
  temp[3] = map(analogRead(TMP36), 0, 410, -50, 150);
  delay(10);
  temp[4] = map(analogRead(TMP36), 0, 410, -50, 150);
  delay(10);
  temp[5] = map(analogRead(TMP36), 0, 410, -50, 150);
  delay(10);
  temp[6] = map(analogRead(TMP36), 0, 410, -50, 150);
  delay(10);
  temp[7] = map(analogRead(TMP36), 0, 410, -50, 150);
  delay(10);
  temp[8] = map(analogRead(TMP36), 0, 410, -50, 150);
  delay(10);
  temp[9] = map(analogRead(TMP36), 0, 410, -50, 150);

  temperatur = (temp[0] + temp[1] + temp[2] + temp[3] + temp[4] + temp[5] + temp[6] + temp[7] + temp[8] + temp[9]) / 10 + tempcorr;


  cls();
  //read the date from the DS1307

  char textchar[16]; // the 16 characters on the display
  byte dow = rtc[3]; // day of week 0 = Sunday
  byte date = rtc[4];
  byte month = rtc[5] - 1;


  //array of month names to print on the display. Some are shortened as we only have 8 characters across to play with
  char monthnames[12][9] = {
    "Januar", "Februar", "M;rz", "April", "Mai", "Juni", "Juli", "August", "Sept", "Oktober", "November", "Dezember"
  };

  //call the flashing cursor effect for one blink at x,y pos 0,0, height 5, width 7, repeats 1
  flashing_cursor(0, 0, 5, 7, 1);

  //print the day name
  int i = 0;
  while (daysfull[dow][i])
  {
    flashing_cursor(i * 6, 0, 5, 7, 0);
    ht1632_putchar(i * 6 , 0, daysfull[dow][i]);
    i++;

    //check for button press and exit if there is one.
    if (buttonA.uniquePress() || buttonB.uniquePress()) {
      return;
    }
  }

  //pause at the end of the line with a flashing cursor if there is space to print it.
  //if there is no space left, dont print the cursor, just wait.
  if (i * 6 < 48) {
    flashing_cursor(i * 6, 0, 5, 7, 1);
  }
  else {
    delay(300);
  }

  //flash the cursor on the next line
  flashing_cursor(0, 8, 5, 7, 0);

  //print the date on the next line: First convert the date number to chars so we can print it with ht1632_putchar
  char buffer[3];
  itoa(date, buffer, 10);

  //then work out date 2 letter suffix - eg st, nd, rd, th etc
  char suffix[4][3] = {".", ".", ".", "."  }; //define at top of code
  byte s = 3;
  if (date == 1 || date == 21 || date == 31) {
    s = 0;
  }
  else if (date == 2 || date == 22) {
    s = 1;
  }
  else if (date == 3 || date == 23) {
    s = 2;
  }

  //print the 1st date number
  ht1632_putchar(0, 8, buffer[0]);

  //if date is under 10 - then we only have 1 digit so set positions of sufix etc one character nearer
  byte suffixposx = 6;

  //if date over 9 then print second number and set xpos of suffix to be 1 char further away
  if (date > 9) {
    suffixposx = 12;
    flashing_cursor(6, 8, 5, 7, 0);
    ht1632_putchar(6, 8, buffer[1]);
  }

  //print the 2 suffix characters
  flashing_cursor(suffixposx, 8, 5, 7, 0);
  ht1632_putchar(suffixposx, 8, suffix[s][0]);
  //delay(70);

  flashing_cursor(suffixposx + 6, 8, 5, 7, 0);
  ht1632_putchar(suffixposx + 6, 8, suffix[s][1]);
  //delay(70);

  //blink cursor after
  flashing_cursor(suffixposx + 6, 8, 5, 7, 1);

  //replace day name with date on top line - effectively scroll the bottom line up by 8 pixels
  cls();

  ht1632_putchar(0, 0, buffer[0]);  //date first digit
  ht1632_putchar(6, 0, buffer[1]);  //date second digit - this may be blank and overwritten if the date is a single number
  ht1632_putchar(suffixposx, 0, suffix[s][0]);   //date suffix
  ht1632_putchar(suffixposx + 6, 0, suffix[s][1]); //date suffix


  //flash the cursor for a second for effect
  flashing_cursor(suffixposx + 12, 0, 5, 7, 0);

  //print the month name on the bottom row
  i = 0;
  while (monthnames[month][i])
  {
    flashing_cursor(i * 6, 8, 5, 7, 0);
    ht1632_putchar(i * 6, 8, monthnames[month][i]);
    i++;

    //check for button press and exit if there is one.
    if (buttonA.uniquePress() || buttonB.uniquePress()) {
      return;
    }
  }

  //blink the cursor at end if enough space after the month name, otherwise juts wait a while
  if (i * 6 < 48) {
    flashing_cursor(i * 6, 8, 5, 7, 1);
  }

  //print the temprature name on the top row
  cls();

  // get temparature
  i = 0;

  itoa(temperatur, buffer, 10);

  if (temperatur < 10) {
    buffer[1] = buffer[0];
    buffer[0] = '0';
  }

  textchar[0] = buffer[0];
  textchar[1] = buffer[1];
  textchar[2] = '#';
  textchar[3] = 'C';
  textchar[4] = ' ';
  textchar[5] = ' ';

  byte x = 0;
  byte y = 0;

  //print each char for temperature
  for (byte x = 0; x < 5 ; x++) {
    flashing_cursor(i * 6, 0, 5, 7, 0);
    ht1632_putchar( i * 6, 0, textchar[x]); //bottom line
    i++;

    //check for button press and exit if there is one.
    if (buttonA.uniquePress() || buttonB.uniquePress()) {
      return;
    }
  }

  //blink the cursor at end if enough space after the temperature, otherwise juts wait a while
  if (i * 6 < 48) {
    flashing_cursor(i * 6, 0, 5, 7, 3);
  }
  else {
    delay(500);
  }
  delay(1500);
}


// display a horizontal bar on the screen at offset xbar by ybar with height and width of xbar, ybar
void levelbar (byte xpos, byte ypos, byte xbar, byte ybar) {
  for (byte x = 0; x <= xbar; x++) {
    for (byte y = 0; y <= ybar; y++) {
      plot(x + xpos, y + ypos, 1);
    }
  }
}

//flashing_cursor
//print a flashing_cursor at xpos, ypos and flash it repeats times
void flashing_cursor(byte xpos, byte ypos, byte cursor_width, byte cursor_height, byte repeats)
{
  for (byte r = 0; r <= repeats; r++) {
    for (byte x = 0; x <= cursor_width; x++) {
      for (byte y = 0; y <= cursor_height; y++) {
        plot(x + xpos, y + ypos, 1);
      }
    }

    if (repeats > 0) {
      delay(400);
    }
    else {
      delay(70);
    }

    for (byte x = 0; x <= cursor_width; x++) {
      for (byte y = 0; y <= cursor_height; y++) {
        plot(x + xpos, y + ypos, 0);
      }
    }
    //if cursor set to repeat, wait a while
    if (repeats > 0) {
      delay(400);
    }
  }
}


// is there a special event to display today?
bool is_special_event()
{
  byte mins, date, month;
  bool tiny = false;
  char message[2][13] = {"", ""};

  // get time and date
  mins  = rtc[1];
  date  = rtc[4];
  month = rtc[5];

  // display event only every 15 min (if not already done)
  //if ((mins % 15 == 0) && (mins != old_mins)) {
  if ((mins % 30 == 0) && (mins != old_mins)) {     // every 30 min
    //if (rtc[0] % 15 == 0) {                         // DEBUG - every 15 sec
    //if (mins != old_mins) {                          // DEBUG - every 1 min

    old_mins = mins;     // holds time when the last special event was displayed (to avoid to re-display it repetitively)

    // check if an event occurs today

    // Note:
    //   NORMAL font is required for messages up to 8 characters. This is the default font.
    //   TINY font is required when there are more than 8 characters per line (and no more than 12).

    if (date == 1 && month == 1) {           // 1. January, Neujahr
      strcpy(message[0], "Frohes");
      strcpy(message[1], "Neues Jahr");
      tiny = true;
    }
    else if (date == 11 && month == 2) {      // 11. February,
      strcpy(message[0], "Happy B-Day");
      strcpy(message[1], "NeoRame");
      tiny = true;
    }
    else if (date == 14 && month == 3) {      // 14. March, Schiblo Day
      strcpy(message[0], "Schnitzel u.");
      strcpy(message[1], "Blowjob Tag");
      tiny = true;
    }
    else if (date == 23 && month == 4) {     // 23 April, Tag des Deutschen Bieres
      strcpy(message[0], "Tag des");
      strcpy(message[1], "Dt. Bieres");
      tiny = true;
    }
    else if (date == 1 && month == 5) {     // 1. Mai, Tag der Arbeit
      strcpy(message[0], "Tag der");
      strcpy(message[1], "Arbeit");
    }
    else if (date == 25 && month == 5) {    // 25 Mai, Towel Day
      strcpy(message[0], "KEINE PANIK");
      strcpy(message[1], "Handtuch Tag");
      tiny = true;
    }
    else if (date == 3 && month == 10) {     // 3 Oktober, Tag der Deutschen Einheit
      strcpy(message[0], "Tag der");
      strcpy(message[1], "Dt. Einheit");
      tiny = true;
    }
    else if (date == 6 && month == 12) {    // 6 Dezember, Nikolaus
      strcpy(message[0], "6. Dez");
      strcpy(message[1], "Nikolaustag");
      tiny = true;
    }
    else if (date == 24 && month == 12) {    // 24 Dezember, Weihnachten
      strcpy(message[0], "Frohes");
      strcpy(message[1], "Fest");
    }

    // yes an event occurs today, display the message
    if (strlen(message[0]) > 0) {

      cls();

      for (byte i = 0; i < 2; i++) {

        // display line
        display_event(i + 1, message[i], tiny);

        // check buttons
        if (buttonA.isPressed()) {
          return true;
        }
      }

      /* DOESN'T WORK PROPERLY --> the 2 screens blinks alternatively
        // set blink ON
        for (byte d = 0; d < NUM_DISPLAYS; d++) {
        ht1632_sendcmd(d, HT1632_CMD_BLON);
        }
        delay(4000);

        // set blink OFF
        for (byte d = 0; d < NUM_DISPLAYS; d++) {
        ht1632_sendcmd(d, HT1632_CMD_BLOFF);
        }
      */
      delay(3000);

      return true;
    }
  }
  return false;
}


// (called by is_special_event)
// display an event message on a line 1 or 2, with NORMAL or TINY font
void display_event (byte line, char mess[], bool tiny)
{
  byte i;
  byte y;  // starting y position on the line
  byte w;  // character width
  byte h;  // character height

  if (tiny) {
    y = 2 + ((line - 1) * 7);   // 1st line = 2, 2nd line = 9
    w = 3;                      // 3x4 pix character (max 12 char per line)
    h = 4;
  } else {
    y = 0 + ((line - 1) * 8);   // 1st line = 0, 2nd line = 8
    w = 5;                      // 5x7 pix character (max 8 char per line)
    h = 7;
  }

  // call the flashing cursor effect for one blink at x, y pos, height h, width w, repeats 1 (1st line only)
  if (line == 1) {
    flashing_cursor(0, y, w, h, 1);
  }

  // display the line
  i = 0;
  while (mess[i]) {

    flashing_cursor(i * (w + 1), y, w, h, 0);

    if (tiny) {
      ht1632_puttinychar(i * (w + 1), y, mess[i]);
    } else {
      ht1632_putchar(i * (w + 1), y, mess[i]);
    }
    i++;

    // check buttons
    if (buttonA.isPressed()) {
      return;
    }
  }

  // pause at the end of the 2nd line with a flashing cursor if there is space to print it
  // if there is no space left, don't print the cursor, just wait
  if (i * (w + 1) < 48) {
    if (line == 2) {
      flashing_cursor(i * (w + 1), y, w, h, 1);
      delay(400);
    }
  }
  else {
    delay(2000);
  }
}


//************ MENU & MISC ROUTINES ******************

// button_delay
// like regular delay but can be quit by a button press

void button_delay(int wait) {
  int i = 0;
  while ( i < wait) {
    //check if a button is pressed, if it is, quit waiting
    if (buttonA.uniquePress() || buttonB.uniquePress()) {
      return;
    }
    //else wait a moment
    delay (1);
    i++;
  }
}


//dislpay menu to change the clock mode
void switch_mode() {

  //remember mode we are in. We use this value if we go into settings mode, so we can change back from settings mode (6) to whatever mode we were in.
  old_mode = clock_mode;

  char* modes[] = {
    "Invader", "Pong", "gr.Zahlen", "Normal", "Temperatur", "Slide", "8421-BCD", "Menue"
  };

  byte next_clock_mode;
  byte firstrun = 1;

  //loop waiting for button (timeout after 35 loops to return to mode X)
  for (int count = 0; count < 35 ; count++) {

    //if user hits button, change the clock_mode
    if (buttonA.uniquePress() || firstrun == 1) {

      count = 0;
      cls();

      if (firstrun == 0) {
        clock_mode++;
      }
      if (clock_mode > NUM_DISPLAY_MODES + 1 ) {
        clock_mode = 0;
      }

      //print arrown and current clock_mode name on line one and print next clock_mode name on line two
      char str_top[12];
      char str_bot[12];

      strcpy (str_top, ">");
      strcat (str_top, modes[clock_mode]);

      next_clock_mode = clock_mode + 1;
      if (next_clock_mode >  NUM_DISPLAY_MODES + 1 ) {
        next_clock_mode = 0;
      }

      strcpy (str_bot, " ");
      strcat (str_bot, modes[next_clock_mode]);

      byte i = 0;
      while (str_top[i]) {
        ht1632_puttinychar(i * 4, 2, str_top[i]);
        i++;
      }

      i = 0;
      while (str_bot[i]) {
        ht1632_puttinychar(i * 4, 10, str_bot[i]);
        i++;
      }
      firstrun = 0;
    }
    delay(50);
  }
}



//dislpay menu to change the clock settings
void setup_menu() {

  char* set_modes[] = {
    "Zurueck", "Zufall aus", "Uhrzeit", "Helligkeit", "Sommerzeit"
  };
  if (random_mode == 0) {
    set_modes[1] = ("Zufall an");
  }

  byte setting_mode = 0;
  byte next_setting_mode;
  byte firstrun = 1;

  //loop waiting for button (timeout after 35 loops to return to mode X)
  for (int count = 0; count < 35 ; count++) {

    //if user hits button, change the clock_mode
    if (buttonA.uniquePress() || firstrun == 1) {

      count = 0;
      cls();

      if (firstrun == 0) {
        setting_mode++;
      }
      if (setting_mode > NUM_SETTINGS_MODES) {
        setting_mode = 0;
      }

      //print arrown and current clock_mode name on line one and print next clock_mode name on line two
      char str_top[12];
      char str_bot[12];

      strcpy (str_top, ">");
      strcat (str_top, set_modes[setting_mode]);

      next_setting_mode = setting_mode + 1;
      if (next_setting_mode > NUM_SETTINGS_MODES) {
        next_setting_mode = 0;
      }

      strcpy (str_bot, " ");
      strcat (str_bot,  set_modes[next_setting_mode]);

      byte i = 0;
      while (str_top[i]) {
        ht1632_puttinychar(i * 4, 2, str_top[i]);
        i++;
      }

      i = 0;
      while (str_bot[i]) {
        ht1632_puttinychar(i * 4, 10, str_bot[i]);
        i++;
      }
      firstrun = 0;
    }
    delay(50);
  }

  //pick the mode
  switch (setting_mode) {
    case 0:
      //exit
      break;
    case 1:
      set_random();
      break;
    case 2:
      set_time();
      break;
    case 3:
      set_brightness();
      break;
    case 4:
      set_dst();
      break;
  }

  //change the clock from mode 6 (settings) back to the one it was in before
  clock_mode = old_mode;
}


//add or remove an hour for daylight savings
void set_dst() {
  cls();

  char text_a[11] = "Sommerzeit";
  char text_b[11] = " Modus An ";
  char text_c[11] = " Modus Aus";

  byte i = 0;

  get_time();
  byte hr = rtc[2];

  //if daylight mode if on, turn it off
  if (daylight_mode) {

    //turn random mode off
    daylight_mode = 0;

    //take one off the hour
    if (hr > 0) {
      hr--;
    } else {
      hr = 23;
    }

    //print a message on the display
    while (text_a[i]) {
      ht1632_puttinychar((i * 4) + 4,  2, text_a[i]);
      ht1632_puttinychar((i * 4) + 4, 10, text_c[i]);
      i++;
    }
  } else {
    //turn daylight mode on.
    daylight_mode = 1;

    //add one to the hour
    if (hr < 23) {
      hr++;
    } else {
      hr = 0;
    }

    //print a message on the display
    while (text_a[i]) {
      ht1632_puttinychar((i * 4) + 4,  2, text_a[i]);
      ht1632_puttinychar((i * 4) + 4, 10, text_b[i]);
      i++;
    }
  }

  //write the change to the clock chip
  DateTime now = ds1307.now();
  ds1307.adjust(DateTime(now.year(), now.month(), now.day(), hr, now.minute(), now.second()));

  delay(1500); //leave the message up for a second or so
}

//run clock main loop as long as run_mode returns true
byte run_mode() {

  //if random mode is on... check the hour when we change mode.
  if (random_mode) {
    //if hour value in change mode time = hours. then reurn false = i.e. exit mode.
    if (change_mode_time == rtc[2]) {
      //set the next random clock mode and time to change it
      set_next_random();
      //exit the current mode.
      return 0;
    }
  }

  //else return 1 - keep running in this mode
  return 1;

}


//toggle random mode - pick a different clock mode every few hours
void set_random() {
  cls();

  char text_a[13] = "Zufallsmodus";
  char text_b[13] = " Modus An   ";
  char text_c[13] = " Modus Aus  ";
  byte i = 0;

  //if random mode is on, turn it off
  if (random_mode) {

    //turn random mode off
    random_mode = 0;

    //print a message on the display
    while (text_a[i]) {
      ht1632_puttinychar((i * 4),  2, text_a[i]);
      ht1632_puttinychar((i * 4), 10, text_c[i]);
      i++;
    }
  } else {
    //turn randome mode on.
    random_mode = 1;

    //set hour mode will change
    set_next_random();

    //print a message on the display
    while (text_a[i]) {
      ht1632_puttinychar((i * 4),  2, text_a[i]);
      ht1632_puttinychar((i * 4), 10, text_b[i]);
      i++;
    }
  }
  delay(1500); //leave the message up for a second or so
}


//set the next hour the clock will change mode when random mode is on
void set_next_random() {

  //set the next hour the clock mode will change - current time plus 1 hours
  get_time();
  change_mode_time = rtc[2] + 1 ;

  //if change_mode_time now happens to be over 23, then set it to 0
  if (change_mode_time == 24) {
    change_mode_time = 0;
  }

  //set the new clock mode
  clock_mode = random(0, NUM_DISPLAY_MODES + 1);  //pick new random clock mode
}


//change screen brightness
void set_brightness() {

  cls();

  //print "brightness"
  byte i = 0;
  char text[11] = "Helligkeit";
  while (text[i]) {
    ht1632_puttinychar((i * 4) + 4, 2, text[i]);
    i++;
  }

  //wait for button input
  while (!buttonA.uniquePress()) {

    levelbar (1, 8, brightness * 3, 6); //display the brightness level as a bar
    while (buttonB.isPressed()) {

      if (brightness == 15) {
        brightness = 0;
        cls ();
      }
      else {
        brightness++;
      }
      //print the new value
      i = 0;
      while (text[i]) {
        ht1632_puttinychar((i * 4) + 4, 2, text[i]);
        i++;
      }
      levelbar (1, 8, brightness * 3, 6); //display the brightness level as a bar
      ht1632_sendcmd(0, HT1632_CMD_PWM + brightness);    //immediately change the
      ht1632_sendcmd(1, HT1632_CMD_PWM + brightness);    //brightness on both panels
      delay(150);
    }
  }
}


//power up led test & display software version number
void printver() {

  byte i = 0;
  char ver_top[16]   = "Multi Clock";
  char ver_lower[16] = "by NeoRame";

  //test all leds.
  for (byte x = 0; x < 48; x++) {
    for (byte y = 0; y < 16; y++) {
      plot(x, y, 1);
    }
  }
  delay(1000);
  fade_down();

  //draw box
  for (byte x = 0; x < 46; x++) {
    plot(x + 1, 0, 1);
    plot(x + 1, 15, 1);
  }
  for (byte y = 1; y < 15; y++) {
    plot(0, y, 1);
    plot(47, y, 1);
  }

  //top line
  while (ver_top[i]) {
    ht1632_puttinychar((i * 4) + 2, 2, ver_top[i]);
    delay(35);
    i++;
  }

  i = 0;
  //bottom line
  while (ver_lower[i]) {
    ht1632_puttinychar((i * 4) + 4, 9, ver_lower[i]);
    delay(35);
    i++;
  }

  delay(4000);
  fade_down();
  // end version message
}




//************* DS1307 ROUTINES ****************


//set time and date routine
void set_time() {

  cls();

  //fill settings with current clock values read from clock
  get_time();
  byte set_min   = rtc[1];
  byte set_hr    = rtc[2];
  byte set_date  = rtc[4];
  byte set_mnth  = rtc[5];
  int  set_yr    = rtc[6];

  //Set function - we pass in: which 'set' message to show at top, current value, reset value, and rollover limit.
  set_date = set_value(2, set_date, 1, 31);
  set_mnth = set_value(3, set_mnth, 1, 12);
  set_yr   = set_value(4, set_yr, 2013, 2099);
  set_hr   = set_value(1, set_hr, 0, 23);
  set_min  = set_value(0, set_min, 0, 59);

  ds1307.adjust(DateTime(set_yr, set_mnth, set_date, set_hr, set_min));

  cls();
}


//used to set min, hr, date, month, year values. pass
//message = which 'set' message to print,
//current value = current value of property we are setting
//reset_value = what to reset value to if to rolls over. E.g. mins roll from 60 to 0, months from 12 to 1
//rollover limit = when value rolls over
int set_value(byte message, int current_value, int reset_value, int rollover_limit) {

  cls();
  char messages[6][17]   = {
    "Minuten", "Stunden", "Tag", "Monat", "Jahr"
  };

  //Print "set xyz" top line
  byte i = 0;
  while (messages[message][i])
  {
    ht1632_puttinychar((i * 4) + 1 , 2, messages[message][i]);
    i++;
  }

  //print digits bottom line
  char buffer[5] = "    ";
  itoa(current_value, buffer, 10);
  ht1632_putchar(0 , 8, buffer[0]);
  ht1632_putchar(6 , 8, buffer[1]);
  ht1632_putchar(12, 8, buffer[2]);
  ht1632_putchar(18, 8, buffer[3]);

  delay(300);
  //wait for button input
  while (!buttonA.uniquePress()) {

    while (buttonB.isPressed()) {

      if (current_value < rollover_limit) {
        current_value++;
      }
      else {
        current_value = reset_value;
      }
      //print the new value
      itoa(current_value, buffer , 10);
      ht1632_putchar(0 , 8, buffer[0]);
      ht1632_putchar(6 , 8, buffer[1]);
      ht1632_putchar(12, 8, buffer[2]);
      ht1632_putchar(18, 8, buffer[3]);

      delay(150);
    }
  }
  return current_value;
}


//Get the time from the DS1307 chip.
void get_time()
{
  //get time
  DateTime now = ds1307.now();
  //save time to array
  rtc[6] = now.year();
  rtc[5] = now.month();
  rtc[4] = now.day();
  rtc[3] = now.dayOfWeek(); //returns 0-6 where 0 = Sunday
  rtc[2] = now.hour();
  rtc[1] = now.minute();
  rtc[0] = now.second();

  //flash arduino led on pin 13 every second
  if ( (rtc[0] % 2) == 0) {
    digitalWrite(13, HIGH);
  }
  else {
    digitalWrite(13, LOW);
  }

  //print the time to the serial port.
  Serial.print(rtc[2]);
  Serial.print(":");
  Serial.print(rtc[1]);
  Serial.print(":");
  Serial.println(rtc[0]);
}
