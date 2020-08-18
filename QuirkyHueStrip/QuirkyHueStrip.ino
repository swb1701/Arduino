/*
	This is a quick mashup of two implementations:
	
	https://github.com/probonopd/ESP8266HueEmulator (provides a Hue Bridge Emulator for the 8266)
    https://github.com/w1ll1am23/pivot_power_genius_mqtt (provides an MQTT implementation for the power strip)

    Follow the dependency instructions for the Hue Bridge Emulator.  It works well enough for the Echo to control things
    either through the app or through voice.  The Hue app seems to have issues when the SPIFFs write is triggered to save
    configurations.
    
    Currently you have to name your devices in the LightService.h file.  Logically a next step (including cleaning up these
    two pasted-together apps would be allowing web-based configuration).

	I'm running this on the Wemos D1 Mini.

        It uses a single neopixel as an indicator light (replacing where the Imp did blink-up).

	--Scott Bennett, 2020-08-16
*/


/**
 * Emulate Philips Hue Bridge ; so far the Hue app finds the emulated Bridge and gets its config
 * and switch NeoPixels with it
 **/

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <TimeLib.h>
#include <NtpClientLib.h>
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h> // instead of NeoPixelAnimator branch
#include "LightService.h"

// these are only used in LightHandler.cpp, but it seems that the IDE only scans the .ino and real libraries for dependencies
#include "SSDP.h"
#include <aJSON.h> // Replace avm/pgmspace.h with pgmspace.h there and set #define PRINT_BUFFER_LEN 4096 ################# IMPORTANT

//#include "/secrets.h" // Delete this line and populate the following
const char* ssid = "yourssid";
const char* password = "yourpwd";

RgbColor red = RgbColor(COLOR_SATURATION, 0, 0);
RgbColor green = RgbColor(0, COLOR_SATURATION, 0);
RgbColor white = RgbColor(COLOR_SATURATION);
RgbColor black = RgbColor(0);

bool outlet_1_power = false;
bool outlet_2_power = false;
bool should_toggle_1 = false;
bool should_toggle_2 = false;
int outlet_1_button_state;
int outlet_2_button_state;
int last_outlet_1_button_state = LOW;
int last_outlet_2_button_state = LOW;
unsigned long last_outlet_1_button_debounce_time = 0;
unsigned long last_outlet_2_button_debounce_time = 0;
unsigned long debounceDelay = 100;

// Settings for the NeoPixels
#define NUM_PIXELS_PER_LIGHT 1 // How many physical LEDs per emulated bulb

// PIN setup
#define OUTLET_1_BUTTON_PIN 13
#define OUTLET_2_BUTTON_PIN 12
#define OUTLET_1_PIN  4
#define OUTLET_2_PIN 5

#define pixelCount 1
#define pixelPin 2 // Strip is attached to GPIO2 on ESP-01
NeoPixelBus<NeoGrbFeature, NeoEsp8266Uart1Ws2812xMethod> strip(MAX_LIGHT_HANDLERS * NUM_PIXELS_PER_LIGHT, pixelPin);
NeoPixelAnimator animator(MAX_LIGHT_HANDLERS * NUM_PIXELS_PER_LIGHT, NEO_MILLISECONDS); // NeoPixel animation management object
LightServiceClass LightService;

HsbColor getHsb(int hue, int sat, int bri) {
  float H, S, B;
  H = ((float)hue) / 182.04 / 360.0;
  S = ((float)sat) / COLOR_SATURATION;
  B = ((float)bri) / COLOR_SATURATION;
  return HsbColor(H, S, B);
}

class PixelHandler : public LightHandler {
  private:
    HueLightInfo _info;
    int16_t colorloopIndex = -1;
  public:
    void handleQuery(int lightNumber, HueLightInfo newInfo, aJsonObject* raw) {
      Serial.print("LN=");
      Serial.println(lightNumber);
      // define the effect to apply, in this case linear blend
      HslColor newColor = HslColor(getHsb(newInfo.hue, newInfo.saturation, newInfo.brightness));
      HslColor originalColor = strip.GetPixelColor(lightNumber);
      _info = newInfo;

    
      if (newInfo.on) {

        if (lightNumber==0) {
        
        AnimUpdateCallback animUpdate = [ = ](const AnimationParam & param)
        {
          // progress will start at 0.0 and end at 1.0
          HslColor updatedColor = HslColor::LinearBlend<NeoHueBlendShortestDistance>(originalColor, newColor, param.progress);

          for(int i=lightNumber * NUM_PIXELS_PER_LIGHT; i < (lightNumber * NUM_PIXELS_PER_LIGHT) + NUM_PIXELS_PER_LIGHT; i++) {
            strip.SetPixelColor(i, updatedColor);
          }
        };
        
        Serial.print("Animating on...");
        Serial.println(lightNumber);
        animator.StartAnimation(lightNumber, _info.transitionTime, animUpdate);
        /*for(int i=lightNumber * NUM_PIXELS_PER_LIGHT; i < (lightNumber * NUM_PIXELS_PER_LIGHT) + NUM_PIXELS_PER_LIGHT; i++) {
            strip.SetPixelColor(i, red);
        }    
        strip.Show();    */
        } else {
          Serial.println("In else clause");
          if (lightNumber==1) {
            Serial.println("Turning Outlet 1 On");
            digitalWrite(OUTLET_1_PIN,HIGH);
          } else if (lightNumber==2) {
            Serial.println("Turning Outlet 2 On");
            digitalWrite(OUTLET_2_PIN,HIGH);
          }
        }
      }
      else {

        if (lightNumber==0) {
        AnimUpdateCallback animUpdate = [ = ](const AnimationParam & param)
        {
          // progress will start at 0.0 and end at 1.0
          HslColor updatedColor = HslColor::LinearBlend<NeoHueBlendShortestDistance>(originalColor, black, param.progress);
          
          for(int i=lightNumber * NUM_PIXELS_PER_LIGHT; i < (lightNumber * NUM_PIXELS_PER_LIGHT) + NUM_PIXELS_PER_LIGHT; i++) {
            strip.SetPixelColor(i, updatedColor);
          }
        };
        
        Serial.print("Animating off...");
        Serial.println(lightNumber);
        animator.StartAnimation(lightNumber, _info.transitionTime, animUpdate);
        /*for(int i=lightNumber * NUM_PIXELS_PER_LIGHT; i < (lightNumber * NUM_PIXELS_PER_LIGHT) + NUM_PIXELS_PER_LIGHT; i++) {
            strip.SetPixelColor(i, black);
        }    
        strip.Show();           */
        } else {
          if (lightNumber==1) {
            Serial.println("Turning Outlet 1 Off");
            digitalWrite(OUTLET_1_PIN,LOW);
          } else if (lightNumber==2) {
            Serial.println("Turning Outlet 2 Off");
            digitalWrite(OUTLET_2_PIN,LOW);
          }
        }
      }
    }

    HueLightInfo getInfo(int lightNumber) { return _info; }
};

void check_outlet_1_button() {
  int reading = digitalRead(OUTLET_1_BUTTON_PIN);

  if (reading != last_outlet_1_button_state) {
    last_outlet_1_button_debounce_time = millis();
  }

  if ((millis() - last_outlet_1_button_debounce_time) > debounceDelay) {
    if (reading != outlet_1_button_state) {
      outlet_1_button_state = reading;

      if (outlet_1_button_state == LOW) {
        Serial.println("Outlet 1 button pressed");
        should_toggle_1 = true;
      }
    }
  }

  last_outlet_1_button_state = reading;
}

void check_outlet_2_button() {
  int reading = digitalRead(OUTLET_2_BUTTON_PIN);

  if (reading != last_outlet_2_button_state) {
    last_outlet_2_button_debounce_time = millis();
  }

  if ((millis() - last_outlet_2_button_debounce_time) > debounceDelay) {
    if (reading != outlet_2_button_state) {
      outlet_2_button_state = reading;

      // only toggle the LED if the new button state is HIGH
      if (outlet_2_button_state == LOW) {
        Serial.println("Outlet 2 button pressed");
        should_toggle_2 = true;
      }
    }
  }

  last_outlet_2_button_state = reading;
}

void setup() {

  // this resets all the neopixels to an off state
  strip.Begin();
  strip.Show();

  // Show that the NeoPixels are alive
  delay(120); // Apparently needed to make the first few pixels animate correctly
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  infoLight(white);

  while (WiFi.status() != WL_CONNECTED) {
    infoLight(red);
    delay(500);
    Serial.print("+");
  }

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  // Sync our clock
  NTP.begin("pool.ntp.org", 0, true);

  // Show that we are connected
  
  infoLight(green);

  pinMode(OUTLET_1_BUTTON_PIN, INPUT);
  pinMode(OUTLET_2_BUTTON_PIN, INPUT);

  //for testing do pullups
  //digitalWrite(OUTLET_1_BUTTON_PIN,HIGH);
  //digitalWrite(OUTLET_2_BUTTON_PIN,HIGH);
  
  pinMode(OUTLET_1_PIN, OUTPUT);
  pinMode(OUTLET_2_PIN, OUTPUT);

  // Turn them on when it starts up?
  digitalWrite(OUTLET_1_PIN, LOW);
  digitalWrite(OUTLET_2_PIN, LOW);
  /*
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  digitalWrite(LED_BUILTIN, LOW);  // Turn the LED off by making the voltage HIGH (D1 Mini is LOW for on)
  */
  LightService.begin();

  // setup pixels as lights
  for (int i = 0; i < MAX_LIGHT_HANDLERS /*&& i < pixelCount*/; i++) {
    LightService.setLightHandler(i, new PixelHandler());
  }

  // We'll get the time eventually ...
  if (timeStatus() == timeSet) {
    Serial.println(NTP.getTimeDateString(now()));
  }
}

void loop() {

  
 check_outlet_1_button();
  if (!outlet_1_power && should_toggle_1) {
    Serial.println("Turning on");
    digitalWrite(OUTLET_1_PIN, HIGH);
    should_toggle_1 = false;
  }
  if (outlet_1_power && should_toggle_1) {
    Serial.println("Turning off");
    digitalWrite(OUTLET_1_PIN, LOW);
    should_toggle_1 = false;
  }

  check_outlet_2_button();
  if (!outlet_2_power && should_toggle_2) {
    Serial.println("Turning on");
    digitalWrite(OUTLET_2_PIN, HIGH);
    should_toggle_2 = false;
  }
  if (outlet_2_power && should_toggle_2) {
    Serial.println("Turning off");
    digitalWrite(OUTLET_2_PIN, LOW);
    should_toggle_2 = false;
  }

 if (digitalRead(OUTLET_1_PIN) == HIGH && !outlet_1_power) {
    
    outlet_1_power = true;
  }
  if (digitalRead(OUTLET_1_PIN) == LOW && outlet_1_power) {
  
    outlet_1_power = false;
  }
  if (digitalRead(OUTLET_2_PIN) == HIGH && !outlet_2_power) {
  
    outlet_2_power = true;
  }
  if (digitalRead(OUTLET_2_PIN) == LOW && outlet_2_power) {
  
    outlet_2_power = false;
  }  
  
  ArduinoOTA.handle();
  
  LightService.update();

  
  static unsigned long update_strip_time = 0;  //  keeps track of pixel refresh rate... limits updates to 33 Hz
  if (millis() - update_strip_time > 30)
  {
    if ( animator.IsAnimating() ) animator.UpdateAnimations();
    strip.Show();
    update_strip_time = millis();
  }
}

void infoLight(RgbColor color) {
  // Flash the strip in the selected color. White = booted, green = WLAN connected, red = WLAN could not connect
  for (int i = 0; i < pixelCount; i++)
  {
    strip.SetPixelColor(i, color);
    strip.Show();
    delay(10);
    strip.SetPixelColor(i, black);
    strip.Show();
  }
}
