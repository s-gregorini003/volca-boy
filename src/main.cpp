#include "Adafruit_NeoTrellis.h"  //  Library to communicate with the Adafruit NeoTrellis keypads through the I2C serial protocol
#include <U8x8lib.h>              //  Graphics library for embedded devices that supports monochrome OLEDs and LCD. Compatible with SSD1309 and I2C serial protocol
#include "MIDI.h"                 //  Library to enable MIDI I/O communication on the Arduino serial port

//#define NEOTRELLIS_KEYPAD
Adafruit_NeoTrellis trellis;

#define INT_PIN 10

U8X8_SSD1309_128X64_NONAME2_HW_I2C u8x8(8);

uint8_t skull[16]={0x00,0x00,0x00,0x00,0x78,0xCC,0xCF,0xFF,0xFF,0xCF,0xCC,0x78,0x00,0x00,0x00,0x00};
uint8_t slot[8]={0xE7,0x81,0x81,0x00,0x00,0x81,0x81,0xE7,};
//====================================================================

/*
Methods and variables to initialise and update the rotary encoder (lower knob on the right side of the device). The updateEncoder() method is nothing more than a set of
bitwise operations to make the device understand whether the encoder is increasing or decreasing.

Useful resources:
- Bitwise operators - https://playground.arduino.cc/Code/BitMath/
- How rotary encoder works - https://howtomechatronics.com/tutorials/arduino/rotary-encoder-works-use-arduino/
*/

int encoderPin1 = 2;
int encoderPin2 = 3;
 
volatile int lastEncoded = 0;
volatile long encoderValue = 0;
 
long lastencoderValue = 0;
 
int lastMSB = 0;
int lastLSB = 0;

void updateEncoder(){
  int MSB = digitalRead(encoderPin1);   //  Most Significant Bit
  int LSB = digitalRead(encoderPin2);   //  Least Significant Bit

  int encoded = (MSB << 1) | LSB; //  Converting the 2 pin value to single number
  int sum  = (lastEncoded << 2) | encoded;  //  Adding it to the previous encoded value
 
  if(sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) encoderValue ++;
  if(sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) encoderValue --;
 
  lastEncoded = encoded; //store this value for next time
}

char buf[4];


//====================================================================

// Input a value 0 to 255 to get a color value.
// The colors are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return trellis.pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return trellis.pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return trellis.pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

//define a callback for key presses
TrellisCallback blink(keyEvent evt){
  
  if(evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING)
    trellis.pixels.setPixelColor(evt.bit.NUM, Wheel(map(evt.bit.NUM, 0, trellis.pixels.numPixels(), 0, 255))); //on rising
  else if(evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING)
    trellis.pixels.setPixelColor(evt.bit.NUM, 0); //off falling
    
  trellis.pixels.show();
  return 0;
}

void setup() {
  Serial.begin(31250); //The standard midi sample rate is 31250 kbps
  //while(!Serial);

//====================================================================
//DISPLAY SETUP

  u8x8.setI2CAddress(0x78);
  
  pinMode(INT_PIN, INPUT);
  
  u8x8.begin();
  u8x8.setPowerSave(0);


//====================================================================
//ROTARY ENCODER SETUP
  pinMode(encoderPin1, INPUT_PULLUP);
  pinMode(encoderPin2, INPUT_PULLUP);
 
  attachInterrupt(2, updateEncoder, CHANGE);
  attachInterrupt(3, updateEncoder, CHANGE);


//====================================================================
//NEOTRELLIS SETUP
  #ifdef NEOTRELLIS_KEYPAD
    if(!trellis.begin(0x2F)){
      Serial.println("could not start trellis");
      while(1);
    }
    else{
      Serial.println("trellis started");
    }
    
    //activate all keys and set callbacks
    for(int i=0; i<NEO_TRELLIS_NUM_KEYS; i++){
      trellis.activateKey(i, SEESAW_KEYPAD_EDGE_RISING);
      trellis.activateKey(i, SEESAW_KEYPAD_EDGE_FALLING);
      trellis.registerCallback(i, blink);
    }

    //do a little animation to show we're on
    for(uint16_t i=0; i<trellis.pixels.numPixels(); i++) {
      trellis.pixels.setPixelColor(i, Wheel(map(i, 0, trellis.pixels.numPixels(), 0, 255)));
      trellis.pixels.show();
      delay(50);
    }
    for(uint16_t i=0; i<trellis.pixels.numPixels(); i++) {
      trellis.pixels.setPixelColor(i, 0x000000);
      trellis.pixels.show();
      delay(50);
    }
  #endif

}

void loop() {

  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.drawTile(0, 0, 4, slot);
  u8x8.setInverseFont(1);
  sprintf(buf, "%d", int(encoderValue));
  const char* p=buf;
  u8x8.drawString(0,0,p);
  #ifdef NEOTRELLIS_KEYPAD
    if(!digitalRead(INT_PIN)){
      trellis.read(false);
    }
  #endif
}