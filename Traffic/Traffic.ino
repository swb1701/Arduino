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

/* Modification of AdvancedWebServer Example for Cloud Traffic Light -- Scott Bennett, May 2022 */

#include "FS.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "config.h"

const char *ssid = STASSID;
const char *password = STAPSK;

const char* host = BUILD_HOST;
const char* fingerprint = BUILD_FINGERPRINT;

ESP8266WebServer server(80);
byte mac[6];
char mac_Id[18];
const char* AWS_endpoint = "<your aws iot endpoint>.amazonaws.com"; //MQTT broker ip
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");
WiFiClientSecure espClient;

int lightOn=0; //state variable for whether lamp is on
char statusMessage[1024];

//turn on the light and echo status
void lightGreen() {
  light(1,0,0);
  server.send(200,"text/plain","green on");
  statusMessage[0]=0;
  strcat(statusMessage,"Green");
}
void lightYellow() {
  light(0,1,0);
  server.send(200,"text/plain","yellow on");
  statusMessage[0]=0;
  strcat(statusMessage,"Yellow");
}
void lightRed() {
  light(0,0,1);
  server.send(200,"text/plain","red on");
  statusMessage[0]=0;
  strcat(statusMessage,"Red");
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
   Serial.println();
    //String msg=String((byte *)payload);
    //Serial.print(msg);
    //light(0,1,0);
    payload[length]='\0';
    String str((char*) payload);
    if (str=="traffic:red") {
      light(0,0,1);
    } else if (str=="traffic:green") {
      light(1,0,0);
    } else if (str=="traffic:yellow") {
      light(0,1,0);
    }
    /*
    if (strcmp((char *)payload[i],"traffic:red")==0) {
      light(0,0,1);
    } else if (strcmp((char *)payload[i],"traffic:green")==0) {
      light(1,0,0);
    } else if (strcmp((char *)payload[i],"traffic:yellow")==0) {
      light(0,1,0);
    }
    */
  
 
}

PubSubClient client(AWS_endpoint, 8883, callback, espClient);

void light(int g,int y,int r) {
  digitalWrite(D6,1-r);
  digitalWrite(D7,1-y);
  digitalWrite(D8,1-g);
}

void handleRoot() {
  char temp[400];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;

  snprintf(temp, 400,

           "<html>\
  <head>\
    <meta http-equiv='refresh' content='5'/>\
    <title>Traffic</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Traffic Light</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\
    <p>Status: %s</p>\
  </body>\
</html>",
           hr, min % 60, sec % 60, statusMessage
          );
  server.send(200, "text/html", temp);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("traffic1")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      char buf[256];
      espClient.getLastSSLError(buf, 256);
      Serial.print("WiFiClientSecure SSL error: ");
      Serial.println(buf);
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup(void) {
  pinMode(D6,OUTPUT);
  pinMode(D7,OUTPUT);
  pinMode(D8,OUTPUT);
  light(0,0,1);
  Serial.begin(115200);
  Serial.println("Started");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  light(0,1,0);
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  timeClient.begin();
  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }
  espClient.setBufferSizes(512,512);
  espClient.setX509Time(timeClient.getEpochTime());
  if (MDNS.begin("traffic")) {
    Serial.println("MDNS responder started");
  }
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }
  Serial.print("Heap:");
  Serial.println(ESP.getFreeHeap());
  // Load certificate file
  File cert = SPIFFS.open("/cert.der", "r"); //replace cert.crt eith your uploaded file name
  if (!cert) {
    Serial.println("Failed to open cert file");
  }
  delay(1000);

  if (espClient.loadCertificate(cert))
    Serial.println("CERT Loaded");
  else
    Serial.println("CERT NOT Loaded");

  // Load private key file
  File private_key = SPIFFS.open("/private.der", "r"); //replace private eith your uploaded file name
  if (!private_key) {
    Serial.println("Failed to open private cert file");
  }

  delay(1000);

  if (espClient.loadPrivateKey(private_key))
    Serial.println("Private Key Loaded");
  else
    Serial.println("Private Key NOT Loaded");

  // Load CA file
  File ca = SPIFFS.open("/ca.der", "r"); //replace ca eith your uploaded file name
  if (!ca) {
    Serial.println("Failed to open ca ");
  }

  delay(1000);

  if (espClient.loadCACert(ca))
    Serial.println("CA Loaded");
  else
    Serial.println("CA NOT Loaded");

  Serial.print("Heap:");
  Serial.println(ESP.getFreeHeap());

  WiFi.macAddress(mac);
  snprintf(mac_Id, sizeof(mac_Id), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.print("MAC:");
  Serial.println(mac_Id);
  
  server.on("/", handleRoot);
  server.on("/red",lightRed);
  server.on("/yellow",lightYellow);
  server.on("/green",lightGreen);
  server.begin();
  Serial.println("HTTP server started");
  light(1,0,0);
}

void loop(void) {
   if (!client.connected()) {
    reconnect();
    light(1,0,0);
  }
  client.loop();
  server.handleClient();
  MDNS.update();
}
