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

/* Modification of AdvancedWebServer Example for Build Break Light -- Scott Bennett, Nov 2021 */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>
#include "config.h"

const char *ssid = STASSID;
const char *password = STAPSK;

const char* host = BUILD_HOST;
const char* fingerprint = BUILD_FINGERPRINT;

ESP8266WebServer server(80);

const int led = 13;
int lightOn=0; //state variable for whether lamp is on
unsigned long lastBuildCheck=millis(); //last time we checked the build(s)
int numBuilds=2;
char* builds[2] = {"qbot-reports-test","qbot-reports-staging"};
char *failedBuild;
char statusMessage[1024];

//turn on the light and echo status
void turnOn() {
  digitalWrite(D4,1);
  server.send(200,"text/plain","on");
  lightOn=1;
}

//turn off the light and echo status
void turnOff() {
  digitalWrite(D4,0);
  server.send(200,"text/plain","off");
  lightOn=0;
}

//turn on the light and don't echo status
void autoOn() {
  digitalWrite(D4,1);
  lightOn=1;
}

//turn off the light and don't echo status
void autoOff() {
  digitalWrite(D4,0);
  lightOn=0;
}

//toggle light state and echo status
void toggle() {
  if (lightOn==1) {
    turnOff();
  } else {
    turnOn();
  }
}

//check builds
void checkBuilds() {
  boolean fail=false;
  for(int i=0;i<numBuilds;i++) {
    if (!checkBuild(builds[i])) {
       fail=true;
       failedBuild=builds[i];
    }
  }
  if (fail) {
      autoOn();
      Serial.print(failedBuild);
      Serial.print(" ");
      Serial.println("FAILURE");
      statusMessage[0]=0;
      strcat(statusMessage,"Build ");
      strcat(statusMessage,failedBuild);
      strcat(statusMessage," Failed!");
  } else {
      autoOff();
      Serial.println("SUCCESS");
      statusMessage[0]=0;
      strcat(statusMessage,"All Builds OK");
  }
}

//check a build
boolean checkBuild(char *buildName) {
  WiFiClientSecure client;
  HTTPClient http;
  client.setFingerprint(fingerprint);
  if (!client.connect(host,443)) {
    Serial.println("connection failed");
    return(false);
  }
  char str[1024]="\0";
  strcat(str,"https://");
  strcat(str,BUILD_HOST);
  strcat(str,"/job/");
  strcat(str,buildName);
  strcat(str,"/lastBuild/api/json");
  http.begin(client,str);
  http.addHeader("Authorization",BUILD_AUTH);
  int httpCode = http.GET();
  boolean result=true;
  if (httpCode>0) {
    if (http.getString().indexOf("FAILURE")>-1) {
      result=false;
    }
  } else {
    Serial.println(httpCode);
  }
  http.end();
  return(result);
}

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
    <h1>Build Break Detector</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\
    <p>Status: %s</p>\
  </body>\
</html>",
           hr, min % 60, sec % 60, statusMessage
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

void setup(void) {
  pinMode(led, OUTPUT);
  pinMode(D4,OUTPUT);
  digitalWrite(led, 0);
  digitalWrite(D4,0);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("redlight")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/on",turnOn);
  server.on("/off",turnOff);
  server.on("/toggle",toggle);
  server.on("/checkBuilds",checkBuilds);
  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient();
  MDNS.update();
  if ((millis()-lastBuildCheck)>30000) {
      checkBuilds();
      lastBuildCheck=millis();
  }
}
