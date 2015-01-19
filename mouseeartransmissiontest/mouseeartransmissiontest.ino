#define IRledPin 3

// Mouse Ear Transmission Test Tool for Arduino
// (C) 2012-2013 Jonathan Fether
//Modifications by Scott Bennett, 2015

// This product is not endorsed, authorized, or prepared by the manufacturer of the ears.

/*

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 The license document is available at: http://www.gnu.org/licenses/gpl-2.0.html
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 
 */
 
// This procedure sends a 38KHz pulse to the IRledPin 
// for a certain # of microseconds. We'll use this whenever we need to send code. 
// It's almost exactly LadyAda's modulation code but for some reason the timings were off on my board.

int i=0;
char bytebuf[2];

void pulseIR(long microsecs, int hilo) {
  // we'll count down from the number of microseconds we are told to wait

  while (microsecs > 0) {
    // 38 kHz is about 13 microseconds high and 13 microseconds low
    digitalWrite(IRledPin, hilo);  // this takes about 5 microseconds to happen
    delayMicroseconds(8);         // hang out for 8 microseconds
    digitalWrite(IRledPin, LOW);   // this also takes about 5 microseconds
    delayMicroseconds(8);         // hang out for 8 microseconds
    // so 26 microseconds altogether
    microsecs -= 26;
  }

}

void sendbyte(byte b)
{
  // Send 8-N-1 data as the mouse ears communicate in.
  // Data consists of 1 Start Bit, 8 Data Bits, 1 Stop Bit, and NO parity bit
  // 1 bit is 417 microseconds @ 2400 baud
  pulseIR(400, HIGH); // Start bit
  byte i=0;
  while(i<8)
  {
    pulseIR(400, (b>>(i++)&1)?LOW:HIGH); // Data Bits
  }
  pulseIR(400, LOW); // Stop bit
}

void powerOnReset()
{
  // Power-On Reset emulation. I don't think this is a real power on reset but it emulates one for most purposes. 
  // (Like getting the ears into random pattern mode.)
  sendbyte(0x92);
  sendbyte(0x20);
  sendbyte(0x48);
  sendbyte(0x80);
  sendbyte(0x13);
}

unsigned char calc_crc(unsigned char *data, unsigned char length)
{
  // Courtesy of rjchu and Timon - A godsend
  unsigned char crc = 0;
  while(length--) {
    crc ^= *data++;
    unsigned n = 8; 
    do crc = (crc & 1) ? (crc >> 1) ^ 0x8C : crc >> 1; 
    while(--n);
  }
  return crc;
}

void setup()  
{
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }

  Serial.println("Mouse Ear Transmission Test Tool (C) 2012-2013 Jonathan Fether.");
  Serial.println("Special thanks to those at http://www.doityourselfchristmas.com (rjchu, Timon) who found the checksum algorithm.");
  Serial.println("Modifications by Scott Bennett, 2015");
  Serial.println();
  Serial.println("Usage:");
  Serial.println("Type in a mouse ear transmission code. The 9X (One-Wire) checksum will be added automatically but it is not harmful to include a redundant one.");
  Serial.println("Typing 'P' anywhere in your string will emulate power-on reset the mouse ears (92 20 48 80 13) and delay 3 seconds prior to any further action.");
  Serial.println();
  Serial.println("Ready.");

  pinMode(IRledPin, OUTPUT); 
  pinMode(13, OUTPUT); // Clock Timing Monitor Pin
}

byte bytefromhex(char hexed[2])
{
  // This can be massaged if desired to allow better work. I'm just going to use a cheap ASCII offset.
  // This isn't "safe" or "pretty" programming but works for this purpose.
  if(hexed[1]>'9') hexed[1] -= 7; // (7 is the offset from 'A' to ':' which follows '9')
  if(hexed[0]>'9') hexed[0] -= 7;
  return (byte)(((hexed[0] - '0')<<4) + hexed[1] - '0');
}



void loop() // run over and over
{
  byte cmdbuf[32];
  int cmdcount = 0;
  boolean done=false;
  boolean reset=false;

  bytebuf[0]=0;
  bytebuf[1]=0;

  while(!done)
  {
    bytebuf[i]=Serial.read();
    if(bytebuf[i]>='a' && bytebuf[i]<='z') bytebuf[i]=bytebuf[i]-'a'+'A'; //accept lowercase letters -- SWB
    if(bytebuf[i]=='P') reset=true;
    if(bytebuf[i]<'0') i=0; // Not Hex
    if(bytebuf[i]>'Z') i=0; // Not Hex
    if(bytebuf[i]==10) done=true;  // LF
    if(bytebuf[i]==13) done=true;  // CR
    if(cmdcount>31) done=true;
    if(((bytebuf[i]>='0')&&(bytebuf[i]<='9'))||((bytebuf[i]>='A')&&(bytebuf[i]<='F'))) i++; 
    else i=0;
    if(i==2)
    {
      cmdbuf[cmdcount]=bytefromhex(bytebuf);
      if(cmdbuf[cmdcount]<0x10) Serial.print("0");
      Serial.print(cmdbuf[cmdcount++], HEX);
      Serial.print(" ");
      i=0;
    }
  }
  
  Serial.println();

  if ((cmdbuf[0]&0xF0)!=0x90) { //automatically generate packet start and length code -- SWB
    for(int j=cmdcount;j>=0;j--) {
      cmdbuf[j+1]=cmdbuf[j]; //shift everything up one
    }  
    cmdbuf[0]=0x90+(cmdcount-1);
    cmdcount++; //increase command count
  }
  unsigned char checksum=calc_crc(cmdbuf, cmdcount);
  Serial.println("Sending:"); //print full code string to be sent including header and checksum -- SWB
  for(int j=0;j<cmdcount;j++) {
    if (cmdbuf[j]<0x10) Serial.print("0");
    Serial.print(cmdbuf[j],HEX);
    Serial.print(" ");
  }
  if (cmdcount>0) Serial.print(checksum,HEX);
  
  if(reset)
  {
    Serial.print("Power On Reset and 3 second delay... ");
    powerOnReset();
    delay(3000);
    Serial.println("Done!");
  }

  i=0;
  while(i<cmdcount) 
  {
    sendbyte(cmdbuf[i++]);
  }
  sendbyte(checksum);
  Serial.println();
  while(Serial.available()) Serial.read(); // Garbage Removal. Not perfect, garbage sometimes accumulates.
  Serial.println("Ready again");
}


