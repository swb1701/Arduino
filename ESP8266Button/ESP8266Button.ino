/*
  ESP8266 Button
  Scott Bennett

 */

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>

// multicast DNS responder
MDNSResponder mdns;
ESP8266WiFiMulti wifiMulti;
WiFiClient relay;
int rgbGreen=12;
int rgbBlue=13;
int rgbRed=15;
const char* host = "relay.swblabs.com";
const int httpPort = 80;
String cmd="";

// TCP server at port 80 will respond to HTTP requests
WiFiServer server(80);

void setup(void)
{  
  pinMode(rgbRed,OUTPUT); //set status led to RED
  pinMode(rgbGreen,OUTPUT);
  pinMode(rgbBlue,OUTPUT);
  digitalWrite(rgbRed,1);
  digitalWrite(rgbGreen,0);
  digitalWrite(rgbBlue,0);
  Serial.begin(115200);
  delay(10);
  wifiMulti.addAP("YourSSID", "YourPWD");
  //insert several addAP's here to cascade through them
    Serial.println("Connecting Wifi...");
  if(wifiMulti.run() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    digitalWrite(rgbRed,0); //show blue to indicate connected
    digitalWrite(rgbGreen,0);
    digitalWrite(rgbBlue,1);
  }

  //do a test post/get
  if (!relay.connect(host,httpPort)) {
     digitalWrite(rgbRed,1);
     digitalWrite(rgbGreen,0);
     digitalWrite(rgbBlue,0);
  } else {
    //this does a slack channel post on http
    String url="/api/slackPost?token=secrettoken&username=ESP8266&channel=testing&text=Awaiting%20your%20command...";
    relay.print(String("GET ")+url+" HTTP/1.1\r\n"+
    	        "Host: relay.swblabs.com\r\n"+
		"Connection: close\r\n\r\n");
     digitalWrite(rgbRed,0);
     digitalWrite(rgbGreen,1);
     digitalWrite(rgbBlue,0);
  }
  
  // Set up mDNS responder:
  // - first argument is the domain name, in this example
  //   the fully-qualified domain name is "esp8266.local"
  // - second argument is the IP address to advertise
  //   we send our IP address on the WiFi network
  if (!mdns.begin("esp8266", WiFi.localIP())) {
    Serial.println("Error setting up MDNS responder!");
    while(1) { 
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  
  // Start TCP (HTTP) server
  server.begin();
  Serial.println("TCP server started");
  //12=green (rgb)
  //13=blue (rgb)
  //15=red (rgb)
}

void setRGB8(int rgb) {
  digitalWrite(rgbRed,(4&rgb)>>2);
  digitalWrite(rgbGreen,(2&rgb)>>1);
  digitalWrite(rgbBlue,(1&rgb));
}

void sendSlack(String message) {
  if (!relay.connect(host,httpPort)) {
  } else {
  	//a slack post on http
    String url="/api/slackPost?token=secrettoken&username=ESP8266&channel=testing&message="+message;
    relay.print(String("GET ")+url+" HTTP/1.1\r\n"+
    	        "Host: relay.swblabs.com\r\n"+
		"Connection: close\r\n\r\n");
  }
}

void loop(void)
{
  if(wifiMulti.run() != WL_CONNECTED) {
    Serial.println("WiFi not connected!");
    delay(1000);
  }
  if (!relay.connect(host,httpPort)) {
  } else {
  	//this retrieves a slack posting from the relay server
    String poll="/api/popField?token=slacktoken&field=text";
    relay.print(String("GET ")+poll+" HTTP/1.1\r\n"+
    	        "Host: relay.swblabs.com\r\n"+
		"Connection: close\r\n\r\n");
    delay(10);
    while(relay.available()) {
      String line=relay.readStringUntil('\r');
      if ((line.length()>8) && line.startsWith("message",1)) {
        //Serial.println("*** Message ***");
	cmd=line.substring(9);
	Serial.print("Cmd=<");
	Serial.print(cmd);
	Serial.println(">");
	int changed=1;
	if (cmd=="blue") {
	  setRGB8(1);
        } else if (cmd=="red") {
	  setRGB8(4);
        } else if (cmd=="green") {
	  setRGB8(2);
        } else if (cmd=="black") {
	  setRGB8(0);
	} else if (cmd=="white") {
	  setRGB8(7);
	} else if (cmd=="cyan") {
	  setRGB8(3);
	} else if (cmd=="magenta") {
	  setRGB8(5);
        } else if (cmd=="yellow") {
	  setRGB8(6);
	} else {
	  changed=0;
        }
	if (changed==1) {
	  sendSlack("Color%20changed%20to%20"+cmd);
        }
      }
    }
  }
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  Serial.println("");
  Serial.println("New client");

  // Wait for data from client to become available
  while(client.connected() && !client.available()){
    delay(1);
  }
  
  // Read the first line of HTTP request
  String req = client.readStringUntil('\r');
  
  // First line of HTTP request looks like "GET /path HTTP/1.1"
  // Retrieve the "/path" part by finding the spaces
  int addr_start = req.indexOf(' ');
  int addr_end = req.indexOf(' ', addr_start + 1);
  if (addr_start == -1 || addr_end == -1) {
    Serial.print("Invalid request: ");
    Serial.println(req);
    return;
  }
  req = req.substring(addr_start + 1, addr_end);
  Serial.print("Request: ");
  Serial.println(req);
  client.flush();
  
  String s;
  if (req == "/")
  {
    IPAddress ip = WiFi.localIP();
    String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
    s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 at ";
    s += ipStr;
    s += "<br/>Color="+cmd+"</html>\r\n\r\n";
    Serial.println("Sending 200");
  }
  else
  {
    s = "HTTP/1.1 404 Not Found\r\n\r\n";
    Serial.println("Sending 404");
  }
  client.print(s);
  
  Serial.println("Done with client");
}

