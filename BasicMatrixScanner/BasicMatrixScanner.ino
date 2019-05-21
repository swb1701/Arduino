/* 
 * BasicMatrixScanner
 * 
 * (c) 2019 Marcio Teixeira
 * 
 * The code in this page is free software: you can
 * redistribute it and/or modify it under the terms of the GNU
 * General Public License (GNU GPL) as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version.  The code is distributed WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU GPL for more details.
 * 
 */

#include <RGBmatrixPanel.h>

// Most of the signal pins are configurable, but the CLK pin has some
// special constraints.  On 8-bit AVR boards it must be on PORTB...
// Pin 8 works on the Arduino Uno & compatibles (e.g. Adafruit Metro),
// Pin 11 works on the Arduino Mega.  On 32-bit SAMD boards it must be
// on the same PORT as the RGB data pins (D2-D7)...
// Pin 8 works on the Adafruit Metro M0 or Arduino Zero,
// Pin A4 works on the Adafruit Metro M4 (if using the Adafruit RGB
// Matrix Shield, cut trace between CLK pads and run a wire to A4).

#define CLK  8   // USE THIS ON ARDUINO UNO, ADAFRUIT METRO M0, etc.
//#define CLK A4 // USE THIS ON METRO M4 (not M0)
//#define CLK 11 // USE THIS ON ARDUINO MEGA
#define OE   9
#define LAT 10
#define A   A0
#define B   A1
#define C   A2
#define D   A3

#define SENSOR A5  // choose the input pin for the phototransistor

// The following class handles scanning the matrix and reading
// illumination data from the phototransistor.

class RGBmatrixScanner {
  private:
    static constexpr uint8_t  READINGS_PER_PIXELS = 2;
    static constexpr uint8_t  THRESHOLD           = 15;
    static constexpr bool     INVERT              = false;
    static constexpr bool     CLEAR               = true;

    static constexpr uint8_t R1_PIN = 2;
    static constexpr uint8_t G1_PIN = 3;
    static constexpr uint8_t B1_PIN = 4;
    static constexpr uint8_t R2_PIN = 5;
    static constexpr uint8_t G2_PIN = 6;
    static constexpr uint8_t B2_PIN = 7;

    RGBmatrixPanel &matrix;
    uint8_t row, col;
    uint16_t color;

    void setup_pins() {
      // Configure the sensor pin as an input
      // with a pullup resistor to provide
      // power to the phototransistor
      pinMode(SENSOR, INPUT_PULLUP);

      // Configure all the matrix control lines as outputs
      pinMode(CLK,    OUTPUT);
      pinMode(LAT,    OUTPUT);
      pinMode(OE,     OUTPUT);
      pinMode(A,      OUTPUT);
      pinMode(B,      OUTPUT);
      pinMode(C,      OUTPUT);
      pinMode(D,      OUTPUT);
      pinMode(R1_PIN, OUTPUT);
      pinMode(G1_PIN, OUTPUT);
      pinMode(B1_PIN, OUTPUT);
      pinMode(R2_PIN, OUTPUT);
      pinMode(G2_PIN, OUTPUT);
      pinMode(B2_PIN, OUTPUT);
    }

    void set_rgb(bool r, bool g, bool b) {
      digitalWrite(R1_PIN, row <= 15 & r);
      digitalWrite(G1_PIN, row <= 15 & g);
      digitalWrite(B1_PIN, row <= 15 & b);
      digitalWrite(R2_PIN, row >  15 & r);
      digitalWrite(G2_PIN, row >  15 & g);
      digitalWrite(B2_PIN, row >  15 & b);
    }

    void set_row() {
      digitalWrite(A, row & 1);
      digitalWrite(B, row & 2);
      digitalWrite(C, row & 4);
      digitalWrite(D, row & 8);
    }

    void output_enable() {
      digitalWrite(OE, LOW);
    }

    void output_disable() {
      digitalWrite(OE, HIGH);
    }

    void latch_row() {
      digitalWrite(LAT, HIGH);
      digitalWrite(LAT, LOW);
    }

    void shift_row() {
      digitalWrite(CLK, HIGH);
      digitalWrite(CLK, LOW);
    }

    void clear_row() {
      for(uint8_t col = 0; col < 32; col++) {
        set_rgb(0,0,0);
        shift_row();
      }
      latch_row();
    }

    int read_photodetector() {
      return analogRead(SENSOR);
    }

    void read_pixel() {
      // Take multiple readings of the pixel, with the
      // LED on and the LED off.
      int32_t high_sum = 0;
      int32_t low_sum  = 0;
      for(uint8_t i = 0; i < READINGS_PER_PIXELS; i++) {
        output_enable();
        high_sum += read_photodetector();
        output_disable();
        low_sum += read_photodetector();
      }
      // If the accumulator exceeds the threshold, paint
      // the pixel
      const uint32_t diff = abs(high_sum - low_sum);
      const bool isVisible = (diff > THRESHOLD) ^ INVERT;
      if(isVisible)
        matrix.drawPixel(31-col, row, color);
    }

    // Scans an entire row of pixels
    void scan_row() {
      set_rgb(1, 1, 1);
      for(col = 0; col < 32; col++) {
        shift_row();
        latch_row();
        read_pixel();
        if(col == 0)
          set_rgb(0,0,0);
      }
    }
    
  public:
    RGBmatrixScanner(RGBmatrixPanel &m) : matrix(m) {
      setup_pins();
    }
    
    void scan(uint16_t _color) {
      if(CLEAR)
        matrix.fillScreen(0);

      color = _color;
      // When scanning, turn off interrupts to stop the
      // AdaFruit library from repainting the matrix.
      noInterrupts();
      row = 0;
      clear_row();
      output_enable();
      for(row = 0; row < 32; row++) {
        set_row();
        scan_row();
      }
      output_disable();
      interrupts();
    }
};

RGBmatrixPanel   matrix(A, B, C, D, CLK, LAT, OE, false);
RGBmatrixScanner scanner(matrix);

void setup() {
  // Initialize the AdaFruit RGB matrix library. The library
  // is used for showing the captured image, but not for scanning.
  matrix.begin();
}

void loop() {
  // To keep things interesting, change the color on each scan.
  static long hue;
  scanner.scan(matrix.ColorHSV(hue += 100, 255, 128, true));

  // Wait a bit until initiating the next scan
  delay(1750);
}
