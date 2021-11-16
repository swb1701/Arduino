/*
 * This is a modification of the advanced web server examples for ESP8266 
 * (hence below copyright notice) to serve UVC sensor results. -- SWB 10/18/20
 *
 * This is just a quick hack of the example to plot the raw UVC sensor voltage value and show the last value.
 *
 * This should probably be modified to the plot dynamically with javascript as well as having a download
 * SVG option (or download Excel).  The values should be converted to mw/cm^2 (not sure I trust the
 * datasheet since they seem to have changed it by 2x for the device).  I suppose it would be nice to
 * have a meter to calibrate/measure the sensor to calibrate/measure the bulb ;) .
 *
 */

/*
   Copyright (c) 2015, Majenko Technologies
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:

 * * Redistributions of source code must retain the above copyright notice, this
     list of conditions and the following disclaimer.

 * * Redistributions in binary form must reproduce the above copyright notice, this
     list of conditions and the following disclaimer in the documentation and/or
     other materials provided with the distribution.

 * * Neither the name of Majenko Technologies nor the names of its
     contributors may be used to endorse or promote products derived from
     this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
   ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
   ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#ifndef STASSID
#define STASSID "<YourSSID>"
#define STAPSK  "<YourPWD>"
#endif

//Approach below from the post:
//https://www.reddit.com/r/AskElectronics/comments/gr1lzm/i_made_a_uvc_meter_with_an_adruino_board_and_20/

#define REF_VOLTAGE             3.3 //5.0
//Pulled from tech sheet for GUVC-T21GH
//http://www.geni-uv.com/download/products/GUVC-T21GH.pdf
//Vout(V) = 0.355 x UV-C Power (mW/cm^2) or mW/cm^2 = Vout/0.355 or mJ/cm^2 = (Vout/0.355)x time in seconds
#define SUPPLY_CURRENT          50      //microAmp
#define RESPONSTIVITY           0.6     //mV/nM
#define OUTPUT_VOLTAGE          0.355   //V at 1 mW/cm^2
#define OFFSET_VOLTAGE          0.01    //V
#define TARGET_DOSE             60      //mJ/cm^2 taken from this guidance https://www.nebraskamed.com/sites/default/files/documents/covid-19/n-95-decon-process.pdf
//sensor reading smoothing defaults for example AVG_TIME = 1000, AVG_READ_DELAY = 10) this will take 100 readings over 1 second
#define AVG_TIME                1000    //miliseconds  time that reads are averaged for reading smoothing
#define AVG_READ_DELAY          10      //miliseconds  time between reads during averaging cycle
float totalDose=0;                      //running total of uvc mj/cm^2
float CYCLE_TIME_SECS=AVG_TIME/1000;
int   totalTime=0;
float vals[50];
int cur=0;

const char *ssid = STASSID;
const char *password = STAPSK;

ESP8266WebServer server(80);

const int led = 13;

void handleRoot() {
  digitalWrite(led, 1);
  char temp[400];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;

  snprintf(temp, 400,

           "<html>\
  <head>\
    <meta http-equiv='refresh' content='5'/>\
    <title>ESP8266 Demo</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Welcome to the UVC Meter</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\
    <p>Last Reading: %04d/1024</p>\
    <img src=\"/test.svg\" />\
  </body>\
</html>",

           hr, min % 60, sec % 60, (int)vals[(cur+49)%50]
          );
  server.send(200, "text/html", temp);
  digitalWrite(led, 0);
}

void handleNotFound() {
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

void drawGraph() {
  String out;
  out.reserve(2600);
  char temp[70];
  out += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"500\" height=\"256\">\n";
  out += "<rect width=\"500\" height=\"256\" fill=\"rgb(250, 230, 210)\" stroke-width=\"1\" stroke=\"rgb(0, 0, 0)\" />\n";
  out += "<g stroke=\"black\">\n";
  int y=((int)vals[(cur+49)%50]);
  for(int x=0;x<499;x=x+10) {
  //int y = rand() % 130;
  //for (int x = 10; x < 390; x += 10) {
    int y2=((int)vals[(cur+(x/10)+50)%50]);
    Serial.print(x);
    Serial.print(',');
    Serial.print(y);
    Serial.print(',');
    Serial.print(y2);
    //int y2 = rand() % 130;
    sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke-width=\"1\" />\n", x, (int)(256-y/4), x + 10, (int)(256-y2/4));
    out += temp;
    y = y2;
  }
  out += "</g>\n</svg>\n";

  server.send(200, "image/svg+xml", out);
}

void setup(void) {
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  
  Serial.begin(115200);
  Serial.println("Starting...");
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
  delay(2000);
  Serial.println("Waiting for WIFI...");
  WiFi.mode(WIFI_STA);
  //WiFi.persistent(false);
  //WiFi.disconnect(true);
  Serial.println(WiFi.begin(ssid, password));
  Serial.println("");

  // Wait for connection
/*
  while(true) {
    int result=WiFi.status();
    if (result==WL_CONNECTED) break;
    Serial.print(result);
    Serial.println(WiFi.localIP());
    delay(500);
  }
  */
  
  while (WiFi.status() != WL_CONNECTED) {
    //Serial.print(WiFi.status());
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("uvc-meter")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/test.svg", drawGraph);
  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
  for(int i=0;i<50;i++) vals[i]=0.0;
}

void loop(void) {
  
 uint16_t sensorVal = 0;
 uint32_t sensorSum = 0;
 float sensorAvg, sensorVoltage, intensity;
 //average the sensor values for 1 second
 for (int i=0; i<(AVG_TIME/AVG_READ_DELAY); i++)
    {
      sensorVal = analogRead(A0);
      sensorSum += sensorVal;
      delay(AVG_READ_DELAY);
    }
 sensorAvg = (float)sensorSum/(float)(AVG_TIME/AVG_READ_DELAY);
 sensorVoltage = sensorAvg * (REF_VOLTAGE / 1024);
 if (cur>49) cur=0;
 vals[cur]=sensorAvg;
 cur++;
 intensity = sensorVoltage/OUTPUT_VOLTAGE; // taken from the spec sheet for Vout = 0.335 x UV-C Power (mW/cm^2)
 totalDose += intensity*CYCLE_TIME_SECS;
 totalTime =+CYCLE_TIME_SECS;
 Serial.print ("sensorAvg: ");
 Serial.println(sensorAvg);
 Serial.print("totalDose: ");
 Serial.println(totalDose);
 Serial.print ("Voltage");
 Serial.println (sensorVoltage);
 Serial.print ("totaltime");
 Serial.println (totalTime);
  server.handleClient();
  MDNS.update();
}
