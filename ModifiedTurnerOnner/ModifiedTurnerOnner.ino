/*
 * Copyright (c) 2015, Majenko Technologies
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * * Neither the name of Majenko Technologies nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

 /* This code has been customized to add WeMo emulation and to tune the
  *  servo center and on and off offsets -- Scott Bennett, 9/17 */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <Servo.h>
#include "fauxmoESP.h"

const char *ssid = "enter-your-ssid";
const char *password = "enter-your-password";

ESP8266WebServer server ( 80 );
HTTPClient http;
Servo myservo;
fauxmoESP fauxmo;
int act = 0;

int pos = 90;
int offset = 0;//14;
int on_offset=0;//14;
int off_offset=0;//20;

const int led = 13;

char temp[3000];

void handleRoot() {
  digitalWrite ( led, 1 );  
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;

  snprintf ( temp, 3000,
"<!DOCTYPE html>\
<html lang=\"en\">\
  <head>\
    <meta charset=\"utf-8\">\
    <meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\">\
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
    <!-- The above 3 meta tags *must* come first in the head; any other head content must come *after* these tags -->\
    <title>Turner Onner</title>\    
    <link href=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.6/css/bootstrap.min.css\" rel=\"stylesheet\">\
    <!-- HTML5 shim and Respond.js for IE8 support of HTML5 elements and media queries -->\
    <!-- WARNING: Respond.js doesn't work if you view the page via file:// -->\
    <!--[if lt IE 9]>\
      <script src=\"https://oss.maxcdn.com/html5shiv/3.7.2/html5shiv.min.js\"></script>\
      <script src=\"https://oss.maxcdn.com/respond/1.4.2/respond.min.js\"></script>\
    <![endif]-->\
  </head>\
  <body>\
    <div class=\"container\">\
      <div class=\"row\">\
        <div class=\"col-md-12\">\
          <h1>Welcome to the Amazing Turner Onner!</h1>\
        </div>\
      </div>\
      <div class=\"row\">\
        <div class=\"col-md-12\">\
          <button id=\"on_button\" class=\"btn btn-success\">On</button>\
          <button id=\"off_button\" class=\"btn btn-danger\">Off</button>\
          <button id=\"status_button\" class=\"btn btn-info\">Status</button>\
        </div>\
      </div>\
    </div>\
    <script src=\"https://ajax.googleapis.com/ajax/libs/jquery/1.11.3/jquery.min.js\"></script>\    
    <script src=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.6/js/bootstrap.min.js\"></script>\
    <script>\
      $('#on_button').click(function(){\
        $('#status_button').removeClass('btn-info').addClass('btn-warning');\
        $.get('/on', function(data){\
          $('#status_button').removeClass('btn-warning').addClass('btn-info');\
        });\
      });\
      $('#off_button').click(function(){\
        $('#status_button').removeClass('btn-info').addClass('btn-warning');\
        $.get('/off', function(data){\
          $('#status_button').removeClass('btn-warning').addClass('btn-info');\
        });\
      });\      
    </script>\
  </body>\
</html>",

// printf format strings
    hr, 
    min % 60, 
    sec % 60
  );
  server.send ( 200, "text/html", temp ); 
  // MIME-type = text/html
  // Status Code = 200 "OK"
  // Body is temp
  
  digitalWrite ( led, 0 );
}

void handleNotFound() {
  digitalWrite ( led, 1 );
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }

  server.send ( 404, "text/plain", message );
  digitalWrite ( led, 0 );
}

void setup ( void ) {
  pinMode ( led, OUTPUT );
  digitalWrite ( led, 0 );
  Serial.begin ( 115200 );
  WiFi.begin ( ssid, password );
  Serial.println ( "" );

  // Wait for connection
  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }

  Serial.println ( "" );
  Serial.print ( "Connected to " );
  Serial.println ( ssid );
  Serial.print ( "IP address: " );
  Serial.println ( WiFi.localIP() );

  //what follows is an example of posting the IP of the device to a slack channel (provide your own hook)
  /*
  http.begin("https://hooks.slack.com/services/your-slack-hook","AC:95:5A:58:B8:4E:0B:CD:B3:97:D2:88:68:F5:CA:C1:0A:81:E3:6E");
  http.addHeader("Content-Type","application/x-www-form-urlencoded");
  String msg="payload=%7B%22channel%22%3A%22#ips%22,%20%22username%22%3A%22Bedroom%20Light%22,%20%22text%22%3A%22Bedroom%20Light%20has%20an%20IP%20of%20"+WiFi.localIP().toString()+"%22%7D";
  Serial.println(msg);
  int httpCode=http.POST(msg);
  http.writeToStream(&Serial);
  http.end();
  */

  if ( MDNS.begin ( "bedroomlight" ) ) {
    Serial.println ( "\nMDNS responder started" );
  }

  server.on ( "/", handleRoot );
  server.on("/on", turnSwitchOnCb);
  server.on("/off", turnSwitchOffCb);
  server.on ( "/test.svg", drawGraph );
  server.on ( "/inline", []() {
    server.send ( 200, "text/plain", "this works as well" );
  } );
  server.onNotFound ( handleNotFound );
  server.begin();
  Serial.println ( "HTTP server started" );

  fauxmo.addDevice("bedroom light");
  fauxmo.onMessage([](unsigned char device_id, const char * device_name, bool state) {
        Serial.printf("[MAIN] Device #%d (%s) state: %s\n", device_id, device_name, state ? "ON" : "OFF");
        if (strcmp(device_name,"bedroom light")==0) {
          if (state) {
            act=2;
            //Serial.printf("turning on servo\n");
            //moveServoToOnPosition();
          } else {
            act=1;
            //Serial.printf("turning off servo\n");
            //moveServoToOffPosition();
          }
        }
  }); 
/*
  myservo.attach(2);
  Serial.println("servo attached");
  myservo.write(pos+offset);
  moveServoToOffPosition();
*/  
}

void loop ( void ) {
  fauxmo.handle();
  if (act>0) {
    if (act==1) {
      Serial.printf("turning off servo\n");
      moveServoToOffPosition();
    } else if (act==2) {
      Serial.printf("turning on servo\n");
      moveServoToOnPosition();
    }
    act=0;
  }
  server.handleClient();
}

/* To stop servo stuttering -- was changed to detach from servo in between light flips */

void moveServoToOnPosition(void){
  myservo.attach(2);
  for(pos = 90+offset; pos<=170+offset-on_offset; pos+=1)     // goes from 90 degrees to 170 degrees
  {                                  // in steps of 1 degree 
    myservo.write(pos);              // tell servo to go to position in variable 'pos' 
    delay(15);                       // waits 15ms for the servo to reach the position 
  } 
  for(pos = 170+offset-on_offset; pos>=90+offset; pos-=1)     // goes from 170 degrees to 90 degrees
  {                                  // in steps of 1 degree 
    myservo.write(pos);              // tell servo to go to position in variable 'pos' 
    delay(15);                       // waits 15ms for the servo to reach the position 
    Serial.println(pos);
  } 
  myservo.detach();
}

void moveServoToOffPosition(void){
  myservo.attach(2);
  for(pos = 90+offset; pos>=0+offset+off_offset; pos-=1)     // goes from 90 degrees to 0 degrees
  {                                  // in steps of 1 degree 
    myservo.write(pos);              // tell servo to go to position in variable 'pos' 
    delay(15);                       // waits 15ms for the servo to reach the position 
    Serial.println(pos);
  } 
 for(pos = 0+offset+off_offset; pos<=90+offset; pos+=1)     // goes from 180 degrees to 0 degrees
  {                                  // in steps of 1 degree 
    myservo.write(pos);              // tell servo to go to position in variable 'pos' 
    delay(15);                       // waits 15ms for the servo to reach the position 
    Serial.println(pos);
  } 
  myservo.detach();
}

void turnSwitchOnCb(){
  moveServoToOnPosition();
  server.send(200, "text/html", "ON!");
}

void turnSwitchOffCb(){
  moveServoToOffPosition();
  server.send(200, "text/html", "OFF!");
}

void drawGraph() {
  String out = "";
  char temp[100];
  out += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"400\" height=\"150\">\n";
  out += "<rect width=\"400\" height=\"150\" fill=\"rgb(250, 230, 210)\" stroke-width=\"1\" stroke=\"rgb(0, 0, 0)\" />\n";
  out += "<g stroke=\"black\">\n";
  int y = rand() % 130;
  for (int x = 10; x < 390; x+= 10) {
    int y2 = rand() % 130;
    sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke-width=\"1\" />\n", x, 140 - y, x + 10, 140 - y2);
    out += temp;
    y = y2;
  }
  out += "</g>\n</svg>\n";

  server.send ( 200, "image/svg+xml", out);
}
