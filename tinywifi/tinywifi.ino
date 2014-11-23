#include <SoftwareSerial.h>
#include <Adafruit_NeoPixel.h>

Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, 4, NEO_GRB + NEO_KHZ800);

#define SSID "SSID" // insert your SSID
#define PASS "PWD" // insert your password
#define DST_IP "54.235.246.209" //temporacloud.com
SoftwareSerial sSerial(5,6); // RX, TX for talking to wifi module

#if defined(__AVR_ATtiny85__)
  #define DEBUG_CONNECT(x)
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_FLUSH()
#else
  #define DEBUG_CONNECT(x)  Serial.begin(x)
  #define DEBUG_PRINT(x)    Serial.print(x)
  #define DEBUG_PRINTLN(x)    Serial.println(x)
  #define DEBUG_FLUSH()     Serial.flush()
#endif

void setup() {
	strip.begin(); //initialize neopixel
	strip.show();
	DEBUG_CONNECT(9600); //set up console for debugging prints
	sSerial.begin(9600); //set up software serial for talking to wifi module
	DEBUG_PRINTLN("AT+RST <--reset and test if module is ready"); //reset and test if module is ready
	sSerial.println("AT+RST");
	strip.setPixelColor(0, strip.Color(0, 0, 255));
	strip.show();
	delay(1000);
	if (waitForString("ready", 5, 5000)) {
		DEBUG_PRINTLN("<--Wifi Module is ready");
		strip.setPixelColor(0, strip.Color(0, 255, 0));
		strip.show();
	} else {
		DEBUG_PRINTLN("<--Wifi Module not responding -- try a reset");
		strip.setPixelColor(0, strip.Color(255, 0, 0));
		strip.show();
		while (1)
			;
	}
	delay(1000);
	// try to connect to wifi
	boolean connected = false;
	for (int i = 0; i < 5; i++) {
		if (connectWiFi()) {
			connected = true;
			break;
		}
	}
	if (!connected) {
		strip.setPixelColor(0, strip.Color(255, 0, 0));
		strip.show();
		while (1)
			;
	}
	strip.setPixelColor(0, strip.Color(255, 255, 0));
	strip.show();
	delay(5000);
	DEBUG_PRINTLN("AT+CIPMUX=0 <--set to single connection mode"); // set to single connection mode
	sSerial.println("AT+CIPMUX=0");
	strip.setPixelColor(0, strip.Color(0, 0, 0));
	strip.show();
}

void loop() {
	String cmd = "AT+CIPSTART=\"TCP\",\"";
	cmd += DST_IP;
	cmd += "\",80";
	DEBUG_PRINT(cmd);
	DEBUG_PRINTLN(" <--open a port to the web server");
	sSerial.println(cmd);
	if (waitForString("Error", 5, 5000))
		return;
	cmd = "GET /connection/listen?streams=Scott%27s%20LED%20Notifier:i&tokens=<tokencode> HTTP/1.0\r\nHost: temporacloud.com\r\n\r\n";
	DEBUG_PRINT("AT+CIPSEND=");
	sSerial.print("AT+CIPSEND=");
	DEBUG_PRINT(cmd.length());
	DEBUG_PRINTLN(" <--send the number of bytes we are going to transmit");
	sSerial.println(cmd.length());
	if (waitForString(">", 1, 5000)) {
		DEBUG_PRINT(">");
	} else {
		DEBUG_PRINTLN("AT+CIPCLOSE");
		sSerial.println("AT+CIPCLOSE");
		DEBUG_PRINTLN("connection timeout");
		delay(1000);
		return;
	}
	DEBUG_PRINT(cmd);
	DEBUG_PRINTLN(" <--send the get request");
	sSerial.print(cmd);
	int n = 0; // char counter
	char json[100] = "";
	while (1) {
		n = 0;
		if (waitForString("rgbColor", 8, 30000)) {
			int cnt = 5; //skip 5 after rgbColor
			while (1) {
				if (sSerial.available()) {
					cnt--;
					if (cnt < 0) {
						char c = sSerial.read();
						json[n] = c;
						n++;
					} else {
						char c = sSerial.read();
					}
					if (cnt == -6)
						break;
				}
			}
			int r;
			int g;
			int b;
			sscanf(json, "%2x%2x%2x", &r, &g, &b);
			DEBUG_PRINT("Received RGB Color: ");
			DEBUG_PRINT("(");
			DEBUG_PRINT(r);
			DEBUG_PRINT(",");
			DEBUG_PRINT(g);
			DEBUG_PRINT(",");
			DEBUG_PRINT(b);
			DEBUG_PRINTLN(")");
			strip.setPixelColor(0, strip.Color(r, g, b));
			strip.show();
		}
		delay(1000);
	}
}

boolean connectWiFi() {
	DEBUG_PRINTLN("AT+CWMODE=1 <--Set Wifi station mode");
	sSerial.println("AT+CWMODE=1");
	String cmd = "AT+CWJAP=\"";
	cmd += SSID;
	cmd += "\",\"";
	cmd += PASS;
	cmd += "\"";
	DEBUG_PRINT(cmd);
	DEBUG_PRINTLN(" <--join the AP");
	sSerial.println(cmd);
	delay(2000);
	if (waitForString("OK", 2, 5000)) {
		DEBUG_PRINTLN("<--OK, Connected to WiFi.");
		return true;
	} else {
		DEBUG_PRINTLN("<--Can not connect to the WiFi.");
		return false;
	}
}

//Wait for specific input string until timeout runs out
bool waitForString(char* input, uint8_t length, unsigned int timeout) {

	unsigned long end_time = millis() + timeout;
	int current_byte = 0;
	uint8_t index = 0;

	while (end_time >= millis()) {

		if (sSerial.available()) {

			//Read one byte from serial port
			current_byte = sSerial.read();
			DEBUG_PRINT((char )current_byte);

			if (current_byte != -1) {
				//Search one character at a time
				if (current_byte == input[index]) {
					index++;

					//Found the string
					if (index == length) {
						DEBUG_PRINTLN("<--found");
						return true;
					}
					//Restart position of character to look for
				} else {
					index = 0;
				}
			}
		}
	}
	//Timed out
	DEBUG_PRINTLN("<--timeout");
	return false;
}
