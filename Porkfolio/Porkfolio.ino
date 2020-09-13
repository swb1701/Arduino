//Porkfolio Implementation -- Scott Bennett, 9/20
//Running on Wemos D1 Mini (ESP8265)

#include <SoftwareSerial.h> //for talking to the coin reader
#include <Wire.h> //for i2c comms to accelerometer
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

#define PSDA 4 //SDA
#define PSCL 5 //SDL
#define PSER 13 //Coin Reader Serial
#define PRED 16 //Red LED (LOW to Light)
#define PGREEN 14 //Green LED (LOW to Light)
#define PBLUE 12 //Blue LED (LOW to Light)
#define PGREEN2 0 //Green Bi-Color (LOW to Light)
#define PRED2 2 //Red BiColor (LOW to Light)
#define PPHOTO_POWER 15 //Power to Phototransistor
#define PPHOTO_EMIT ADC0 //Read PhotoTransistor (from original blinkup)

#define BAUD_RATE 600 //baud rate for coin reader

SoftwareSerial swSer;
Adafruit_LIS3DH lis = Adafruit_LIS3DH();
// Adjust this number for the sensitivity of the 'click' force
// this strongly depend on the range! for 16G, try 5-10
// for 8G, try 10-20. for 4G try 20-40. for 2G try 40-80
#define CLICKTHRESHHOLD 80

const char* ssid="<yourssid>";
const char* pass="<yourpwd>";
const char* mqttServer="<yourmqttip>";
const int mqttPort=1883;

int amountAddress=0;
float amount=0.0;

int lastLight=-1;
int tot=0;
int cnt=0;
int lightThresh=512;

WiFiClient espClient;
PubSubClient client(espClient);

StaticJsonDocument<200> doc;

void setup() {
  EEPROM.begin(512);
  //define our outputs/input
  pinMode(PRED,OUTPUT);
  pinMode(PGREEN,OUTPUT);
  pinMode(PBLUE,OUTPUT);  
  pinMode(PGREEN2,OUTPUT);
  pinMode(PRED2,OUTPUT);
  pinMode(PPHOTO_POWER,OUTPUT);
  pinMode(PSER,INPUT);

  //turn off our LEDs
  digitalWrite(PRED,HIGH);
  digitalWrite(PGREEN,HIGH);
  digitalWrite(PBLUE,HIGH);
  digitalWrite(PGREEN2,HIGH);
  digitalWrite(PRED2,HIGH);

  //power the photo transistor
  digitalWrite(PPHOTO_POWER,HIGH);

  Serial.begin(115200);

  digitalWrite(PRED,LOW);

  WiFi.begin(ssid,pass);
  while(WiFi.status()!=WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  digitalWrite(PRED,HIGH);
  digitalWrite(PBLUE,LOW);
  
  Serial.println("Looking for accelerometer LIS3DH...");
  while(! lis.begin(0x18)) {   // change this to 0x19 for alternative i2c address
    Serial.print("+");
    delay(500);
  }
  Serial.println("LIS3DH found!");

  client.setServer(mqttServer,mqttPort);
  client.setCallback(callback);

  while(!client.connected()) {
    Serial.println("Connecting to MQTT...");
    if (client.connect("PorkfolioClient")) {
      Serial.println("Connected");
    } else {
      Serial.print("Failed with state ");
      Serial.println(client.state());
      delay(1000);
    }
  }
  
  digitalWrite(PBLUE,HIGH);
  digitalWrite(PGREEN,LOW);

  doc["status"]="started";
  char json[200];
  serializeJson(doc,json);
  client.publish("porkfolio/status",json);
  client.subscribe("porkfolio/commands");
 
  swSer.begin(BAUD_RATE,SWSERIAL_8N1,PSER,PPHOTO_POWER,false,95,11);

  Serial.println("\nPorkfolio Starting\n");

  Serial.println("Reading amount from EEPROM...");
  EEPROM.get(amountAddress,amount);
  Serial.print("Amount=");
  Serial.println(amount);
  
  lis.setRange(LIS3DH_RANGE_2_G);   // 2, 4, 8 or 16 G!
  
  Serial.print("Range = "); Serial.print(2 << lis.getRange());  
  Serial.println("G");

  // 0 = turn off click detection & interrupt
  // 1 = single click only interrupt output
  // 2 = double click only interrupt output, detect single click
  // Adjust threshhold, higher numbers are less sensitive
  lis.setClick(2, CLICKTHRESHHOLD);
  delay(100);
/*
	for (char ch = ' '; ch <= 'z'; ch++) {
		swSer.write(ch);
	}
	swSer.println("");
  */
  //scanPorts();
  //LIS3DH uses 18
  digitalWrite(PGREEN,HIGH);
  
}

/*
uint8_t portArray[] = {16, 5, 4, 0, 2, 14, 12, 13};
String portMap[] = {"D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7"}; //for Wemos

void scanPorts() { 
  for (uint8_t i = 0; i < sizeof(portArray); i++) {
    for (uint8_t j = 0; j < sizeof(portArray); j++) {
      if (i != j){
        Serial.print("Scanning (SDA : SCL) - " + portMap[i] + " : " + portMap[j] + " - ");
        Wire.begin(portArray[i], portArray[j]);
        check_if_exist_I2C();
      }
    }
  }
}

void check_if_exist_I2C() {
  byte error, address;
  int nDevices;
  nDevices = 0;
  for (address = 1; address < 127; address++ )  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0){
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");

      nDevices++;
    } else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  } //for loop
  if (nDevices == 0)
    Serial.println("No I2C devices found");
  else
    Serial.println("**********************************\n");
  //delay(1000);           // wait 1 seconds for next scan, did not find it necessary
}
*/

void callback(char* topic, byte* payload, unsigned int length) {
 
  //Serial.print(topic);
  //Serial.print(":");
  payload[length]=0;
  String paystr((char *) payload);
  /*
  for (int i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  */
  Serial.println(paystr);
  DeserializationError error=deserializeJson(doc,(char*)payload);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
  } else {
    if (doc["command"]=="setColor") {
      digitalWrite(PRED,HIGH);
      digitalWrite(PGREEN,HIGH);
      digitalWrite(PBLUE,HIGH);
      digitalWrite(PGREEN2,HIGH);
      digitalWrite(PRED2,HIGH);
      if (doc["color"]=="red") {
       digitalWrite(PRED,LOW);
       digitalWrite(PRED2,LOW);
      } else if (doc["color"]=="blue") {
       digitalWrite(PBLUE,LOW);
      } else if (doc["color"]=="green") {
       digitalWrite(PGREEN,LOW);
       digitalWrite(PGREEN2,LOW);
      } else if (doc["color"]=="yellow") {
       digitalWrite(PGREEN,LOW);
       digitalWrite(PRED,LOW);
       digitalWrite(PGREEN2,LOW);
       digitalWrite(PRED2,LOW);
      } else if (doc["color"]=="magenta") {
       digitalWrite(PRED,LOW);
       digitalWrite(PBLUE,LOW);
      } else if (doc["color"]=="cyan") {
       digitalWrite(PGREEN,LOW);
       digitalWrite(PBLUE,LOW);
      } else if (doc["color"]=="white") {
       digitalWrite(PGREEN,LOW);
       digitalWrite(PBLUE,LOW);
       digitalWrite(PRED,LOW);
      }
    } else if (doc["command"]=="clearAmount") {
      amount=0.0;
      EEPROM.put(amountAddress,amount);
      EEPROM.commit();
    } else if (doc["command"]=="setLightThreshold") {
      lightThresh=doc["value"];
    } else if (doc["command"]=="getStatus") {
      doc.clear();
      doc["status"]="running";
      doc["amount"]=amount;
      digitalWrite(PPHOTO_POWER,HIGH);
      doc["light"]=analogRead(A0);
      
      char json[200];
      serializeJson(doc,json);
      client.publish("porkfolio/status",json);
    }
  }
}

void loop() {

  client.loop();
  /*  
  int16_t adc;
  uint16_t volt;  
  //unsigned long duration;

 // duration=pulseIn(PSER,LOW);
  //Serial.print("Pulse:");
  //Serial.println(duration);

  Serial.print("LightValue:");
  Serial.println(analogRead(A0));
  */

  /*
  if (lastLight==-1) {
      digitalWrite(PPHOTO_POWER,HIGH);
      lastLight=analogRead(A0);
      Serial.println("got last light");
  } else {
    if (lightThresh>0) {
      digitalWrite(PPHOTO_POWER,HIGH);
      int lightVal0=analogRead(A0);
      int lightVal=0;
      if (cnt<100) {
	cnt++;
	tot+=lightVal0;
      } else {
	lightVal=(int)(1.0*tot/cnt); //average readings
        tot=0;
	cnt=0;
      }
      if (cnt==0) {
	//Serial.print("lightVal=");
	//Serial.println(lightVal);
	if ((lastLight>lightThresh) && (lightVal<lightThresh)) {
	  Serial.println("darker");
	  doc.clear();
	  doc["change"]="darker";
	  //doc["light"]=lightVal;
	  char json[200];
	  serializeJson(doc,json);
	  client.publish("porkfolio/lightEvent",json);
	} else if ((lastLight<lightThresh) && (lightVal>lightThresh)) {
	  Serial.println("lighter");
	  doc.clear();
	  doc["change"]="lighter";
	  //doc["light"]=lightVal;
	  char json[200];
	  serializeJson(doc,json);
	  client.publish("porkfolio/lightEvent",json);
	}
	lastLight=lightVal;
      }
    }
  }
  */

  uint8_t click = lis.getClick();
  if (click & 0x30) {
  if (click & 0x10) {
    doc.clear();
    doc["type"]="singleClick";
    char json[200];
    serializeJson(doc,json);
    client.publish("porkfolio/motionEvent",json);
  }
  if (click & 0x20) {
    doc.clear();
    doc["type"]="doubleClick";
    char json[200];
    serializeJson(doc,json);
    client.publish("porkfolio/motionEvent",json);
  }
  }
  /*
  if (click == 0) return;
  if (! (click & 0x30)) return;
  Serial.print("Click detected (0x"); Serial.print(click, HEX); Serial.print("): ");
  if (click & 0x10) Serial.print(" single click");
  if (click & 0x20) Serial.print(" double click");
  Serial.println();

  delay(100);
    // read the ADCs
  adc = lis.readADC(1);
  volt = map(adc, -32512, 32512, 1800, 900);
  Serial.print("ADC1:\t"); Serial.print(adc); 
  Serial.print(" ("); Serial.print(volt); Serial.print(" mV)  ");
  
  adc = lis.readADC(2);
  volt = map(adc, -32512, 32512, 1800, 900);
  Serial.print("ADC2:\t"); Serial.print(adc); 
  Serial.print(" ("); Serial.print(volt); Serial.print(" mV)  ");

  adc = lis.readADC(3);
  volt = map(adc, -32512, 32512, 1800, 900);
  Serial.print("ADC3:\t"); Serial.print(adc); 
  Serial.print(" ("); Serial.print(volt); Serial.print(" mV)");

  Serial.println();
  delay(200);
  */

  if (swSer.available()>0) {
    int buf[8];
    for(int i=0;i<8;i++) {
      while (swSer.available()<1) {}
      buf[i]=0xFF&swSer.read();
    }
    int num=0;
    if (buf[0]==255) num|=1;
    if (buf[1]==255) num|=2;
    if (buf[2]==255) num|=4;
    char *msg="unknown,0";
    doc.clear();
    switch(num) {
      case 2: doc["coin"]="dime";doc["value"]=0.1; break;
      case 3: doc["coin"]="penny";doc["value"]=0.01; break;
      case 4: doc["coin"]="nickel";doc["value"]=0.05; break;
      case 5: doc["coin"]="quarter";doc["value"]=0.25; break;
      case 6: doc["coin"]="half dollar";doc["value"]=0.50; break;
      default: doc["coin"]="unknown";doc["value"]=0; break;
    }
    amount+=(float)doc["value"];
    doc["total"]=amount;
    EEPROM.put(amountAddress,amount);
    EEPROM.commit();
    char json[200];
    serializeJson(doc,json);
    client.publish("porkfolio/coinEvent",json);
  }

  /*
  
 digitalWrite(PBLUE,HIGH);
 digitalWrite(PRED,LOW);
 delay(500);
 digitalWrite(PRED,HIGH);
 digitalWrite(PGREEN,LOW);
 delay(500);
 digitalWrite(PGREEN,HIGH);
 digitalWrite(PBLUE,LOW);
 delay(500);
 digitalWrite(PBLUE,HIGH);
 digitalWrite(PGREEN2,LOW);
 delay(500);
 digitalWrite(PRED2,LOW);
 delay(500);
 digitalWrite(PGREEN2,HIGH);
 delay(500);
 digitalWrite(PRED2,HIGH);
 delay(500);
  */ 
 /*
	while (Serial.available() > 0) {
		swSer.write(Serial.read());
		yield();
	}*/
 

}
