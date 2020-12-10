/*=========================================================================*
 *                                                                         *
 * Software:  volca-boy-plan-b                                                                        *
 * Version:   0.1                                                          *
 * Date:      10 January 2020                                              *
 * Author:    Silvio Gregorini                                             *
 *                                                                         *
 *-------------------------------------------------------------------------*                                                                         *
    Copyright 2020 Silvio Gregorini
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.                         *
                                                                          
 *=========================================================================*/


#include "Adafruit_NeoTrellis.h"  //      Library to communicate with the Adafruit NeoTrellis keypads through the I2C serial protocol.   Documentation: https://adafruit.github.io/Adafruit_Seesaw/html/index.html
#include <U8x8lib.h>              //      Graphics library for embedded devices that supports monochrome OLEDs and LCD. Compatible with SSD1309 and I2C serial protocol.  Documentation: https://github.com/olikraus/u8g2/wiki/u8x8reference#setinversefont
#include "MIDI.h"                 /*      Library to enable MIDI I/O communication on the Arduino serial port (RX and TX pins).   Documentation: http://fortyseveneffects.github.io/arduino_midi_library/index.html
                                          This could be easily done by manually sending the bytes that compose a MIDI message, for example:

LIBRARY=                  MIDI.sendNoteOn(42, 127, 1); ---> send a note on command with note value 42 and note velocity 127 to channel 1

WITHOUT LIBRARY=          Serial.write(0x90); ------------> send a note on command to channel 1
Serial.write(0x2A); ------------> send the note value equal to 42
Serial.write(0x7F); ------------> send the velocity value equal to 127
*/
#include "config.h"               //      Header file with the pin setup for the Arduino and the configuration settings of the sequencer 
#include "graphics.h"             //      Header file containig all the graphics created for the UI, including the boot screen animation


#define Y_DIM 4   //    Number of rows of NeoTrellis keys
#define X_DIM 8   //    Number of columns of NeoTrellis keys

// Create a matrix of trellis panels
Adafruit_NeoTrellis t_array[Y_DIM / 4][X_DIM / 4] = {           //     In order to be tiled together, the 4x4 NeoTrellis keypads need different I2C addresses.
  { Adafruit_NeoTrellis(0x2E), Adafruit_NeoTrellis(0x2F) }      //     This can be achieved by short circuiting two pads on the back of the keypad's PCB. More on that
};                                                              //     can be found at: https://learn.adafruit.com/adafruit-neotrellis/tiling


bool keyState[32];    //    Boolean array to store the state of each key -----> true = Key is lit up; false = Key is turned off;
bool keyArmed[32];    //    Boolean array to store if a key has a note in memory or not -----> true = Note selected; false = Note missing;
int keyNote[32];      //    Array to store the value of each selected note at its key position

static uint8_t selectedPos = 0;

Adafruit_MultiTrellis trellis((Adafruit_NeoTrellis *)t_array, Y_DIM / 4, X_DIM / 4); //    Pass this matrix to the multitrellis object to initialise it

U8X8_SSD1309_128X64_NONAME2_HW_I2C u8x8(OLED_RST_PIN);                              //    Initialise the OLED display

MIDI_CREATE_DEFAULT_INSTANCE();                                                     //    Initialise the MIDI instance


//====================================================================//
//====================   ROTARY ENCODER METHODS   ====================//
//====================================================================//

/*  ------------------------------------------------------------------
      Methods and variables to initialise and update the rotary encoder (upper knob on the right side of the device). The read_rotary() method is nothing more than a set of
      bitwise operations to make the device understand whether the encoder is increasing or decreasing.

      Useful resources:
       - Bitwise operators - https://playground.arduino.cc/Code/BitMath/
       - How rotary encoder works - https://howtomechatronics.com/tutorials/arduino/rotary-encoder-works-use-arduino/
       - Digital debounce filter - https://www.best-microcontroller-projects.com/rotary-encoder.html#Digital_Debounce_Filter
*/

static uint8_t prevNextCode = 0;
static uint16_t store = 0;

// A vald CW or  CCW move returns 1, invalid returns 0.
int8_t read_rotary() {
  static int8_t rot_enc_table[] = {0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0};

  prevNextCode <<= 2;
  if (digitalRead(POT_DT_PIN)) prevNextCode |= 0x02;
  if (digitalRead(POT_CLK_PIN)) prevNextCode |= 0x01;
  prevNextCode &= 0x0f;

  // If valid then store as 16 bit data.
  if  (rot_enc_table[prevNextCode] ) {
    store <<= 4;
    store |= prevNextCode;
    //if (store==0xd42b) return 1;
    //if (store==0xe817) return -1;
    if ((store & 0xff) == 0x2b) return -1;
    if ((store & 0xff) == 0x17) return 1;
  }
  return 0;
}

/*  ------------------------------------------------------------------
       The updateEncoder() method in one of the most important functions of this software. It is called in the loop() and through it
       the following actions are performed:

       - update of the counter value (which is used to select the note on each channel and step)
       - display of the note value on the OLED
       - selection and confirmation of the note
       - switch of the selected NeoTrellis key from ENABLED to ARMED

*/

void updateEncoder() {
  static int8_t counter, val;

  if ( val = read_rotary() ) {
    counter += val;
    //Serial.print(counter);Serial.print(" ");

    if (counter >= 127) {
      counter = 0;
    }
    if (counter <= 0) {
      counter = 127;
    }

    numberOnScreen(counter, selectedPos / 8 + 1);

  }

  if (!digitalRead(POT_SW_PIN)) {
    keyNote[selectedPos] = counter;
    keyArmed[selectedPos] = true;

    trellis.setPixelColor(selectedPos, ARMED_COLOR);
    trellis.show();

    long lastMillis = millis();
    while (lastMillis + 400 >= millis()) {
      u8x8.drawTile(14, selectedPos / 8 + 1, 1, confirmTile);
    }
    u8x8.drawTile(14, selectedPos / 8 + 1, 1, emptyTile);
  }
}


//====================================================================//
//=====================   OLED DISPLAY METHODS   =====================//
//====================================================================//

/*  ------------------------------------------------------------------
       These two functions - numberOnScreen() and stepOnScreen() - draw a basic UI on the display:

       Ch1 |  STEP    NOTE
       Ch2 |  STEP    NOTE
       Ch3 |  STEP    NOTE
       Ch4 |  STEP    NOTE

*/

void numberOnScreen(int number, int row) {

  u8x8.drawString(0, 1, "Ch1|");
  u8x8.drawTile(9, 1, 1, noteTile);
  u8x8.drawTile(5, 1, 1, stepTile);

  u8x8.drawString(0, 2, "Ch2|");
  u8x8.drawTile(9, 2, 1, noteTile);
  u8x8.drawTile(5, 2, 1, stepTile);

  u8x8.drawString(0, 3, "Ch3|");
  u8x8.drawTile(9, 3, 1, noteTile);
  u8x8.drawTile(5, 3, 1, stepTile);

  u8x8.drawString(0, 4, "Ch4|");
  u8x8.drawTile(9, 4, 1, noteTile);
  u8x8.drawTile(5, 4, 1, stepTile);

  char noteBuf[3];
  sprintf(noteBuf, "%d  ", number);

  byte i, y;

  // u8x8 does not wrap lines.
  y = row;
  for (i = 0; i < strlen(noteBuf); i++) {
    u8x8.setCursor(i % 16 + 10, y);
    u8x8.print(noteBuf[i]);
  }
}

void stepOnScreen(int keyNum) {

  int stp = keyNum % 8 + 1;
  u8x8.setCursor(6, keyNum / 8 + 1);
  u8x8.print(stp, DEC);
}


//====================================================================//
//======================   NEOTRELLIS METHODS   ======================//
//====================================================================//

/*  ------------------------------------------------------------------
      This function is taken from an example in the NeoTrellis library, it sets the color of each key according to a gradient.
      Basically useless, unless for the boot animation.

      Input a value 0 to 255 to get a color value.
      The colors are a transition r - g - b - back to r.
*/

uint32_t Wheel(byte WheelPos) {
  if (WheelPos < 85) {
    return seesaw_NeoPixel::Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    return seesaw_NeoPixel::Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
    WheelPos -= 170;
    return seesaw_NeoPixel::Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  return 0;
}

/*  ------------------------------------------------------------------
      This defineS the callback of a key press, i.e. what the software is meant to do when a key on the grid is pressed.
*/

TrellisCallback selectNote(keyEvent evt) {

  if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING && !keyState[evt.bit.NUM]) {  //    If the key is DISABLED when pressed, enable it
    trellis.setPixelColor(evt.bit.NUM, SELECTED_COLOR[evt.bit.NUM / 8]);
    keyState[evt.bit.NUM] = 1;

    selectedPos = evt.bit.NUM;

    stepOnScreen(selectedPos);

  }
  else if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING && keyState[evt.bit.NUM]) {  //    If the key is ENABLED when pressed, disable and/or disarm it
    trellis.setPixelColor(evt.bit.NUM, 0);
    keyState[evt.bit.NUM] = false;
    keyArmed[evt.bit.NUM] = false;
  }
  trellis.show();
  return 0;
}

//====================================================================//
//======================  MISC HARDWARE METHODS  =====================//
//====================================================================//

/*  ------------------------------------------------------------------
 *    Set the pin mode for the controls (apart from th e NeoTrellis, which have been already configured)
 */
 
void configHardwareControls() {

  //  pinMode(BANK_A_PIN,INPUT);                                      //    Due to the problems during the assembly process, this switch has been omitted. In the initial design, it was meant to be used to browse between  
  //  pinMode(BANK_B_PIN,INPUT);                                      //    bank A (to set the sequencer steps from 1 to 8), bank B (steps 9 to 16) and options (where you could set the MIDI CC commands)
  //  pinMode(OPTIONS_PIN,INPUT);

  pinMode(POT_CLK_PIN, INPUT);
  pinMode(POT_DT_PIN, INPUT);
  pinMode(POT_SW_PIN, INPUT);

  pinMode(PUSH_BUTTON_PIN, INPUT_PULLUP);

}


/*  ------------------------------------------------------------------
 *    After the declaration of some global variables to manage the timing of the sequencer, the updateButton() method is what
 *    lets the user play and stop the pattern, by switching the bool variable 'playState'. Like the updateEncoder() method, 
 *    it is one of the few functions called in loop().
 */
 
long lastButtonPressMillis = 0;
long lastNoteOn = 0, lastNoteOff = 0;
static bool playState = false;
int s;
long stepMicrosInterval = 60 * 1000000 / BPM;

void updateButton() {

  if (!digitalRead(PUSH_BUTTON_PIN)) {
    if (millis() - lastButtonPressMillis >= 300) {
      if (!playState) {
        playState = true;
        s = 0;
        lastButtonPressMillis = millis();
        //      Serial.println("press");
      }
      else {
        playState = false;
        stopPattern();
        lastButtonPressMillis = millis();
      }
    }
  }

}


/*  ------------------------------------------------------------------
 *   When 'playState' is TRUE, sends a note on command for each key which is ENABLED (keyState=true) and ARMED (keyArmed=true and note saved in keyNote).
 *   Then, waits the amount of microseconds (*) that fills the gap between a step and the next one, and sends a midi note off command for the previous step
 *   followed by the note on command of the current step.
 *   
 *   
 *   (*) Usually, in Arduino programming, it is used the millis() function, which appreciates milliseconds, rather than the micros(), 
 *   which has a lot higher resolution. That's because micros() is computationally more expensive. Though, in this program, it's necessary to
 *   be able to count microseconds, otherwise the sequencers wouldn't be that reliable.
 */
 
void playPattern() {

  if (micros() - lastNoteOn >= stepMicrosInterval) {
    for (int c = 0; c < CHANNEL_MAX_SIZE; c++) {

      if (s != 0 && !keyArmed[((s - 1) % 8) + (8 * c)]) trellis.setPixelColor(((s - 1) % 8) + (8 * c), 0x000000);

      if (s != 0 && keyArmed[((s - 1) % 8) + (8 * c)]) {
        MIDI.sendNoteOff(keyNote[((s - 1) % 8) + (8 * c)], 127, c + 1);
      }

      if (!keyArmed[s % 8 + 8 * c]) trellis.setPixelColor(s % 8 + 8 * c, CURSOR_COLOR[(s % 8) / 2]) ;
      if (keyArmed[s % 8 + 8 * c]) {
        MIDI.sendNoteOn(keyNote[s % 8 + 8 * c], 127, c + 1);
      }
    }

    u8x8.drawTile(13, 7, 1, playTile);
    u8x8.setCursor(14, 7);
    u8x8.print(s % 8 + 1);
    lastNoteOn = micros();
    s++;

    trellis.show();
  }

}


/*  ------------------------------------------------------------------
 *    When 'playState' is FALSE, sends a midi note off command to all the notes that were playing at that moment
 */
 
void stopPattern() {
  for (int i = 0; i < CHANNEL_MAX_SIZE; i++) {
    MIDI.sendNoteOff(keyNote[((s - 1) % 8) + (8 * i)], 127, i + 1);
    if (s != 0 && !keyArmed[((s - 1) % 8) + (8 * i)]) trellis.setPixelColor(((s - 1) % 8) + (8 * i), 0x000000);
  }
  u8x8.drawTile(13, 7, 1, emptyTile);
  u8x8.drawTile(14, 7, 1, emptyTile);
}


//====================================================================//
//=========================  BOOT ANIMATION  =========================//
//====================================================================//

void bootScreen() {
  for (int i = 0; i < 16; i++) {
    u8x8.drawTile(0, 0, i + 1, bootTitle[0]);
    u8x8.drawTile(0, 1, i + 1, bootTitle[1]);
    u8x8.drawTile(0, 2, i + 1, bootTitle[2]);
    delay(100);
  }

  delay(1000);

  u8x8.setFont(u8x8_font_5x7_r);
  u8x8.setInverseFont(1);
  u8x8.drawString(8, 3, " plan B");

  delay(1000);
  u8x8.setInverseFont(0);
  u8x8.drawString(0, 6, "Silvio");
  u8x8.drawString(0, 7, "Gregorini");

  delay(3000);

  u8x8.clearDisplay();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.drawString(0, 1, "Ch1|");
  u8x8.drawTile(9, 1, 1, noteTile);
  u8x8.drawTile(5, 1, 1, stepTile);

  u8x8.drawString(0, 2, "Ch2|");
  u8x8.drawTile(9, 2, 1, noteTile);
  u8x8.drawTile(5, 2, 1, stepTile);

  u8x8.drawString(0, 3, "Ch3|");
  u8x8.drawTile(9, 3, 1, noteTile);
  u8x8.drawTile(5, 3, 1, stepTile);

  u8x8.drawString(0, 4, "Ch4|");
  u8x8.drawTile(9, 4, 1, noteTile);
  u8x8.drawTile(5, 4, 1, stepTile);

  u8x8.setFont(u8x8_font_5x7_r);
  u8x8.drawString(0, 7, "Ver 0.1");

}


//====================================================================//
//=========================   MAIN METHODS   =========================//
//====================================================================//


void setup() {
  
  Serial.begin(31250);                                                //    Begin the serial communication. The standard midi sample rate is 31250 kbps
  
    
//====================================================================
//    DISPLAY SETUP
  configHardwareControls();
  
//====================================================================
//    DISPLAY SETUP

  u8x8.setI2CAddress(0x78);                                           //    Exactly like the 4x4 NeoTrellis keypads, to work with I2C the display needs an address too.
  u8x8.begin();                                                       //    I2C ADDRESSES IN USE:   NeoTrellis_1 -> 0x2E  |   NeoTrellis_2 -> 0x2F  |   OLED_Display -> 0x78
  u8x8.setPowerSave(0);

//====================================================================
//    NEOTRELLIS SETUP

  if (!trellis.begin()) {
    Serial.println("failed to begin trellis");
    while (1);
  }

  /* the array can be addressed as x,y or with the key number */
  for (int i = 0; i < Y_DIM * X_DIM; i++) {                            //   First part of the NeoTrellis boot animation, lights up consequently all the keys 
    trellis.setPixelColor(i, Wheel(map(i, 0, X_DIM * Y_DIM, 0, 255)));
    trellis.show();
    delay(50);
  }

  for (int y = 0; y < Y_DIM; y++) {                                   //    Second part of the NeoTrellis boot animation, which also activates all the keys
    for (int x = 0; x < X_DIM; x++) {
      //activate rising and falling edges on all keys
      trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_RISING, true);
      trellis.activateKey(x, y, SEESAW_KEYPAD_EDGE_FALLING, true);
      trellis.registerCallback(x, y, selectNote);
      trellis.setPixelColor(x, y, 0x000000); //addressed with x,y
      trellis.show(); //show all LEDs
      delay(50);
    }
  }

  bootScreen();                                                         //    Screen boot animation
}


void loop() {

  u8x8.setFont(u8x8_font_chroma48medium8_r);                            //    Set the text font for the display

  trellis.read();                                                       //    Wait for a key to be pressed
  updateEncoder();                                                      //    Wait for the encoder to be turned
  updateButton();                                                       //    Wait for the play button to be pressed

  if (playState) playPattern();
}
