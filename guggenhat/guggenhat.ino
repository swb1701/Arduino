/*--------------------------------------------------------------------------
  GUGGENHAT: a Bluefruit LE-enabled wearable NeoPixel marquee.

  Adapted for the Adafruit Feather 32U4 LE by FTC Team 8535
  We Also Use a 5 Meter 60/Pixel Per Meter Strip
  
  Requires:
 
  - BLE_UART, NeoPixel, NeoMatrix and GFX libraries for Arduino.

  Written by Phil Burgess / Paint Your Dragon for Adafruit Industries.
  MIT license.  All text above must be included in any redistribution.
  --------------------------------------------------------------------------*/

#include <Adafruit_BLE_UART.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"
#include "BluefruitConfig.h"
#include <Adafruit_NeoPixel.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_GFX.h>

//#if SOFTWARE_SERIAL_AVAILABLE
//  #include <SoftwareSerial.h>
//#endif

#define MINIMUM_FIRMWARE_VERSION    "0.6.6"
#define MODE_LED_BEHAVIOUR          "MODE"

#define VBATPIN A9

// NEOPIXEL STUFF ----------------------------------------------------------

// 5 meters of NeoPixel strip is coiled around a top hat; the result is
// not a perfect grid.  My large-ish 61cm circumference hat accommodates
// 38 pixels around...a 300 pixel reel makes 8 rows
#define NEO_PIN     6 // Arduino pin to NeoPixel data input
#define NEO_WIDTH  38 // Hat circumference in pixels
#define NEO_HEIGHT  8 // Number of pixel rows (round up if not equal)
#define NEO_OFFSET  (((NEO_WIDTH * NEO_HEIGHT) - 300) / 2)

// Pixel strip must be coiled counterclockwise, top to bottom, due to
// custom remap function (not a regular grid).
Adafruit_NeoMatrix matrix(NEO_WIDTH, NEO_HEIGHT, NEO_PIN,
  NEO_MATRIX_TOP  + NEO_MATRIX_LEFT +
  NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE,
  NEO_GRB         + NEO_KHZ800);

String name="Emma Hat"; //name that will be given to your device

char          lockstr[4] = {'p','w','d',0};
int           lock=0;
int           bufLen;
char          buf[60];
char          msg[21]       = {'T','e','a','m',' ','8','5','3','5',0};            // BLE 20 char limit + NUL
uint8_t       msgLen        = 9;              // Empty message
int           msgX          = matrix.width(); // Start off right edge
unsigned long prevFrameTime = 0L;             // For animation timing
#define FPS 20                                // Scrolling speed

// BLUEFRUIT LE STUFF-------------------------------------------------------

Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

// UTILITY FUNCTIONS -------------------------------------------------------

// Because the NeoPixel strip is coiled and not a uniform grid, a special
// remapping function is used for the NeoMatrix library.  Given an X and Y
// grid position, this returns the corresponding strip pixel number.
// Any off-strip pixels are automatically clipped by the NeoPixel library.
uint16_t remapXY(uint16_t x, uint16_t y) {
  return y * NEO_WIDTH + x - NEO_OFFSET;
}

// Given hexadecimal character [0-9,a-f], return decimal value (0 if invalid)
uint8_t unhex(char c) {
  return ((c >= '0') && (c <= '9')) ?      c - '0' :
         ((c >= 'a') && (c <= 'f')) ? 10 + c - 'a' :
         ((c >= 'A') && (c <= 'F')) ? 10 + c - 'A' : 0;
}

// Read from BTLE into buffer, up to maxlen chars (remainder discarded).
// Does NOT append trailing NUL.  Returns number of bytes stored.
uint8_t readStr(char dest[], uint8_t maxlen) {
  int     c;
  uint8_t len = 0;
  while((c = ble.read()) >= 0) {
    if(len < maxlen) dest[len++] = c;
  }
  if (dest[len-1]==10) len--; //trim LF on end
  return len;
}

// MEAT, POTATOES ----------------------------------------------------------

void setup() {

  //while(!Serial);
  //delay(500);
    
  matrix.begin();
  matrix.setRemapFunction(remapXY);
  matrix.setTextWrap(false);   // Allow scrolling off left
  matrix.setTextColor(matrix.Color(0,0xFF,0)); //Green by default
  //matrix.setTextColor(0xF800); // Red by default
  matrix.setBrightness(31);    // Batteries have limited sauce

  ble.begin();
  String cmd=String("AT+GAPDEVNAME="+name);
  cmd.toCharArray(buf,60);
  ble.sendCommandCheckOK(buf);
  //pinMode(LED, OUTPUT);
  //digitalWrite(LED, LOW);

  //Serial.begin(115200);
  //Serial.println("Bluefruit info:");
  //ble.info();
  //Serial.println("Waiting For Connect");
  //while (!ble.isConnected()) {
  //  delay(500);
  //}
  if ( ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) )
  {
    // Change Mode LED Activity
    //Serial.println(F("******************************"));
    //Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR));
    ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
    //Serial.println(F("******************************"));
  }  
  //Serial.println( F("Switching to DATA mode!") );
  ble.setMode(BLUEFRUIT_MODE_DATA);

  //Serial.println(F("******************************"));  
  //Serial.println("Waiting for Commands");
}

void ackBack(String msg) {
      String resp=String(name+": '"+msg+"'");
      resp.toCharArray(buf,60);
      ble.println(buf);
}

void sendBack(String msg) {
      String resp=String(name+": "+msg);
      resp.toCharArray(buf,60);
      ble.println(buf);
}

void loop() {
  unsigned long t = millis(); // Current elapsed time, milliseconds.
  // millis() comparisons are used rather than delay() so that animation
  // speed is consistent regardless of message length & other factors.

  // If connected, check for input from BTLE...
  if(ble.available()) {
    if (lock==1) {
      if (ble.read()!=lockstr[0] || ble.read()!=lockstr[1] || ble.read()!=lockstr[2]) {
         char dummy[20];
         readStr(dummy,sizeof(dummy));
         sendBack("Not Authorized");
         return;
      }
    }
    if(ble.peek() == '?') {
      char dummy[20];
      readStr(dummy,sizeof(dummy));
      float measuredvbat = analogRead(VBATPIN);
      measuredvbat *= 2;    // we divided by 2, so multiply back
      measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
      measuredvbat /= 1024; // convert to voltage
      ble.print("VBat: " ); ble.println(measuredvbat);      
    } else if (ble.peek() == '#') { // Color commands start with '#'
      //Serial.print("Got Color Command:");
      char color[7];
      switch(readStr(color, sizeof(color))) {
       case 4:                  // #RGB    4/4/4 RGB
        matrix.setTextColor(matrix.Color(
          unhex(color[1]) * 17, // Expand to 8/8/8
          unhex(color[2]) * 17,
          unhex(color[3]) * 17));
        break;
       case 5:                  // #XXXX   5/6/5 RGB
        matrix.setTextColor(
          (unhex(color[1]) << 12) +
          (unhex(color[2]) <<  8) +
          (unhex(color[3]) <<  4) +
           unhex(color[4]));
        break;
       case 7:                  // #RRGGBB 8/8/8 RGB
        matrix.setTextColor(matrix.Color(
          (unhex(color[1]) << 4) + unhex(color[2]),
          (unhex(color[3]) << 4) + unhex(color[4]),
          (unhex(color[5]) << 4) + unhex(color[6])));
        break;
      }
      ackBack(color);
    } else { // Not color, must be message string
      bufLen=readStr(buf,sizeof(buf)-1);
      buf[bufLen]=0;
      if (strcmp(buf,"r")==0) {
        matrix.setTextColor(matrix.Color(0xFF,0,0));
        sendBack("red");
      } else if (strcmp(buf,"g")==0) {
        matrix.setTextColor(matrix.Color(0,0xFF,0));
        sendBack("green");
      } else if (strcmp(buf,"b")==0) {
        matrix.setTextColor(matrix.Color(0,0,0xFF));
        sendBack("blue");
      } else if (strcmp(buf,"w")==0) {
        matrix.setTextColor(matrix.Color(0xFF,0xFF,0xFF));
        sendBack("white");
      } else if (strcmp(buf,"lock")==0) {
        lock=1;
        sendBack("locked");
      } else if (strcmp(buf,"unlock")==0) {
        lock=0;
        sendBack("unlocked");
      } else if (strcmp(buf,"help")==0) {
        sendBack("Commands:");
        sendBack("r=red,g=green,b=blue,w=white");
        sendBack("?=battery voltage");
        sendBack("lock,unlock");
        sendBack("#FF0000=any hex color");
        sendBack("Any other text is a message");
      } else {
      strcpy(msg,buf);
      msgLen=bufLen;
      //msgLen      = readStr(msg, sizeof(msg)-1);
      msg[msgLen] = 0;
      msgX        = matrix.width(); // Reset scrolling
      ackBack(msg);
      }
    }
  }

  if((t - prevFrameTime) >= (1000L / FPS)) { // Handle scrolling
    matrix.fillScreen(0);
    matrix.setCursor(msgX, 0);
    matrix.print(msg);
    if(--msgX < (msgLen * -6)) msgX = matrix.width(); // We must repeat!
    matrix.show();
    prevFrameTime = t;
  }
}
