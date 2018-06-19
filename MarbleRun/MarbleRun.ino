/*
 * MarbleRun server using Espalexa.  (See https://github.com/Aircoookie/Espalexa)
 */ 
#include <Espalexa.h>
 #ifdef ARDUINO_ARCH_ESP32
#include <WiFi.h>
#include <dependencies/webserver/WebServer.h> //https://github.com/bbx10/WebServer_tng
#else
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#endif

// prototypes
boolean connectWifi();

//callback functions
void marbleRunSetting(uint8_t brightness);

// Change this!!
const char* ssid = "YOURSSID";
const char* password = "YOURPASSWORD";

boolean wifiConnected = false;

Espalexa espalexa;
#ifdef ARDUINO_ARCH_ESP32
WebServer server(80);
#else
ESP8266WebServer server(80);
#endif
HTTPClient http;
char temp[3000];

void handleRoot() {
  snprintf ( temp, 3000,
"<!DOCTYPE html>\
<html lang=\"en\">\
  <head>\
    <meta charset=\"utf-8\">\
    <meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\">\
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
    <!-- The above 3 meta tags *must* come first in the head; any other head content must come *after* these tags -->\
    <title>Marble Run</title>\    
    <link href=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.6/css/bootstrap.min.css\" rel=\"stylesheet\">\
    <link href=\"https://cdnjs.cloudflare.com/ajax/libs/bootstrap-slider/10.0.2/css/bootstrap-slider.min.css\" rel=\"stylesheet\">\
    <!-- HTML5 shim and Respond.js for IE8 support of HTML5 elements and media queries -->\
    <!-- WARNING: Respond.js doesn't work if you view the page via file:// -->\
    <!--[if lt IE 9]>\
      <script src=\"https://oss.maxcdn.com/html5shiv/3.7.2/html5shiv.min.js\"></script>\
      <script src=\"https://oss.maxcdn.com/respond/1.4.2/respond.min.js\"></script>\
    <![endif]-->\
    <style>\
    #speedSel .slider-selection {  background: #FF8282; }\
    #speedSel .slider-handle {  background: red; }\
    #speed { width: 300px; }\
    </style>\
  </head>\
  <body>\
    <div class=\"container\">\
      <div class=\"row\">\
        <div class=\"col-md-12\">\
          <h1>Welcome to the Motorized Marble Run!</h1>\
        </div>\
      </div>\
      <div class=\"row\">\
        <div class=\"col-md-12\">\
          <button id=\"on_button\" class=\"btn btn-success\">On</button>\
          <button id=\"off_button\" class=\"btn btn-danger\">Off</button><br/>\
          <p><b>Speed</b><input type=\"text\" class=\"span2\" value=\"\" data-slider-min=\"100\" data-slider-max=\"255\" data-slider-step=\"1\" data-slider-value=\"150\" data-slider-id=\"speedSel\" id=\"speed\" data-slider-tooltip=\"hide\" data-slider-handle=\"triangle\" /></p>\
        </div>\
      </div>\
    </div>\
    <script src=\"https://ajax.googleapis.com/ajax/libs/jquery/1.11.3/jquery.min.js\"></script>\    
    <script src=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.6/js/bootstrap.min.js\"></script>\
    <script src=\"https://cdnjs.cloudflare.com/ajax/libs/bootstrap-slider/10.0.2/bootstrap-slider.min.js\"></script>\        
    <script>\
      $('#on_button').click(function(){\
        $('#status_button').removeClass('btn-info').addClass('btn-warning');\
        $.get('/on?speed=150', function(data){\
        });\
      });\
      $('#off_button').click(function(){\
        $('#status_button').removeClass('btn-info').addClass('btn-warning');\
        $.get('/on?speed=0', function(data){\
        });\
      });\    
      var setSpeed = function() { $.get('/on?speed='+r.getValue(), function(data){ })};\
      var r = $('#speed').slider().on('slide',setSpeed).data('slider');\      
    </script>\
  </body>\
</html>"
  );
  server.send ( 200, "text/html", temp ); 
  // MIME-type = text/html
  // Status Code = 200 "OK"
  // Body is temp
}

void setup()
{
  pinMode(2,OUTPUT);
  analogWrite(2,0);
  Serial.begin(115200);
  // Initialise wifi connection
  wifiConnected = connectWifi();
  
  if(wifiConnected){
  /* If you want to get a slack notification of the IP address of your server, add your token below and adjust #ips to whatever channel you want to send to and uncomment this
  http.begin("https://hooks.slack.com/services/YOURTOKEN","C1:0D:53:49:D2:3E:E5:2B:A2:61:D5:9E:6F:99:0D:3D:FD:8B:B2:B3");
  http.addHeader("Content-Type","application/x-www-form-urlencoded");
  String msg="payload=%7B%22channel%22%3A%22#ips%22,%20%22username%22%3A%22MarbleRun%22,%20%22text%22%3A%22MarbleRun%20has%20an%20IP%20of%20"+WiFi.localIP().toString()+"%22%7D";
  Serial.println(msg);
  int httpCode=http.POST(msg);
  http.writeToStream(&Serial);
  http.end();    
  */
  if ( MDNS.begin ( "marblerun" ) ) {
    Serial.println ( "\nMDNS responder started" );
  }   
    server.on ( "/", handleRoot); 
 
    server.on("/on", HTTP_GET, [](){
      String speed=server.arg("speed");
      analogWrite(2,4*speed.toInt());
      server.send(200, "text/plain", "OK");
    });
    server.onNotFound([](){
      if (!espalexa.handleAlexaApiCall(server.uri(),server.arg(0))) //if you don't know the URI, ask espalexa whether it is an Alexa control request
      {
        //whatever you want to do with 404s
        server.send(404, "text/plain", "Not found");
      }
    });

    // Define your devices here.
    espalexa.addDevice("marble run", marbleRunSetting); //simplest definition, default state off

    espalexa.begin(&server); //give espalexa a pointer to your server object so it can use your server instead of creating its own
    //server.begin(); //omit this since it will be done by espalexa.begin(&server)
  } else
  {
    while (1)
    {
      Serial.println("Cannot connect to WiFi. Please check data and reset the ESP.");
      delay(2500);
    }
  }
}
 
void loop()
{
   //server.handleClient() //you can omit this line from your code since it will be called in espalexa.loop()
   espalexa.loop();
   delay(1);
}

//our callback functions
void marbleRunSetting(uint8_t brightness) {
    Serial.print("Marble Run changed to ");
    
    analogWrite(2,4*brightness); //set PWM based on value

    if (brightness == 255) {
      Serial.println("ON");
    }
    else if (brightness == 0) {
      Serial.println("OFF");
    }
    else {
      Serial.print("DIM "); Serial.println(brightness);
    }
}

// connect to wifi – returns true if successful or false if not
boolean connectWifi(){
  boolean state = true;
  int i = 0;
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.println("Connecting to WiFi");

  // Wait for connection
  Serial.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (i > 20){
      state = false; break;
    }
    i++;
  }
  Serial.println("");
  if (state){
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else {
    Serial.println("Connection failed.");
  }
  delay(100);
  return state;
}
