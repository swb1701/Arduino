void setup() {
  Serial.begin(9600); //Baud rate: 9600
}
void loop() {
  int sensorValue = analogRead(A0);// read the input on analog pin 0:
  float v = sensorValue * (5.0 / 1024.0); // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
  Serial.println(v);
  /*
  Serial.print("V=");
  Serial.println(v); // print out the value you read:
  float t=-1120.4*v*v+5742.3*v-4352.9;
  Serial.print("T=");
  Serial.println(t);
  */
  delay(500);
}


