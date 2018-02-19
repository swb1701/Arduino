/*
  This is a library written for the AS726X Spectral Sensor (Visible or IR) with I2C firmware
  specially loaded. SparkFun sells these at its website: www.sparkfun.com

  Written by Nathan Seidle & Andrew England @ SparkFun Electronics, July 12th, 2017

  https://github.com/sparkfun/Qwiic_Spectral_Sensor_AS726X

  Do you like this library? Help support SparkFun. Buy a board!

  Development environment specifics:
  Arduino IDE 1.8.1

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

//Modified by Scott Bennett to multiplex among the visible and NIR sensors 11/18/17
//Using TCA Multiplexer from Adafruit: https://learn.adafruit.com/adafruit-tca9548a-1-to-8-i2c-multiplexer-breakout/wiring-and-test

#include "AS726X.h"
AS726X sensor1;//Creates the sensor object
AS726X sensor2;

#define TCAADDR 0x70

byte GAIN = 3;
byte MEASUREMENT_MODE = 3;

void tcaselect(uint8_t i) {
  if (i > 7) return;
 
  Wire.beginTransmission(TCAADDR);
  Wire.write(1 << i);
  Wire.endTransmission();  
}

void setup() {
  Wire.begin();
  tcaselect(0);
  delay(500);
  sensor1.begin(Wire, GAIN, MEASUREMENT_MODE);
  tcaselect(1);
  delay(500);
  sensor2.begin(Wire, GAIN, MEASUREMENT_MODE);
}

void loop() {
  tcaselect(0);
  delay(100);
  sensor1.takeMeasurementsWithBulb();
  sensor1.printMeasurements();
  tcaselect(1);
  delay(100);
  sensor2.takeMeasurementsWithBulb();
  sensor2.printMeasurements();  
}