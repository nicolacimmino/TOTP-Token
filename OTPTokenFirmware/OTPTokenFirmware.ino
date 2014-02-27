// OTPToken implements HOTP (HMAC-based One Time Password) described in RFC4226,
//  to generate a time based authentication token that can be used, for instance,
//  in a two factors authentication system.
// 
// This code is suitable for a prototype I built out of salvaged parts. If you
//  want to use this code you might consider using an SPI display in place of
//  the multiplexed 7 segments I used.
//
//  Copyright (C) 2014 Nicola Cimmino
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//   This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see http://www.gnu.org/licenses/.
//
// Connections:
//
// D2  -> Display Segment a anode
// D3  -> Display Segment b anode
// D4  -> Display Segment c anode
// D5  -> Display Segment d anode
// D6  -> Display Segment e anode
// D7  -> Display Segment f anode
// D8  -> Display Segment g anode
// D9  -> DS3234 CS
// D10 -> Display Digit 1 cathode + DS3234 SS
// D11 -> Display Digit 2 cathode + DS3234 MOSI
// D12 -> Display Digit 3 cathode + DS3234 MISO
// D13 -> Display Digit 4 cathode + DS3234 SCLK
// A0  -> Display Digit 5 cathode Addressed in code as digital pin 14
// A1  -> Display Digit 6 cathode Addressed in code as digital pin 15
// A2  -> Display Digit 7 cathode Addressed in code as digital pin 16
// A3  -> Display Digit 8 cathode Addressed in code as digital pin 17
//
// Note that the SPI lines are shared with some of the display catodes. When accessing SPI devices
//  all display anodes must be set LOW and when multiplexing digits the SPI.end must have been
//  called before to release the SPI lines, as long as DS3234 CS is held low its pins will all be
//  high impedance. A shift register for the cathode lines would have reduced the used pins count
//  but as stated this was hacked together with what I had available.
//
#define DS3234_CS_PIN 9

#include <Time.h>
#include <avr/sleep.h>
#include <SPI.h>
#include "sha1.h"

byte digitsSegments[10];

uint8_t secretKey[]={ 0x32, 0x83, 0x56, 0x4f, 0x33, 0x9a, 0xac, 0x21, 0x94, 0x82, 0xcc, 0x3f, 0x48, 0x67, 0x12, 0x3c };

void setup()
{
  Serial.begin(115200);
  
  // This represents the status of the seven segments
  //  display for each digit 0-9. Each bit represents
  //  one segment starting from bit6 being segment a
  //  to bit0 with segment g.
  digitsSegments[0]= 0b1111110;
  digitsSegments[1]= 0b0110000;
  digitsSegments[2]= 0b1101101;
  digitsSegments[3]= 0b1111001;
  digitsSegments[4]= 0b0110011;
  digitsSegments[5]= 0b1011011;
  digitsSegments[6]= 0b1011111;
  digitsSegments[7]= 0b1110000;
  digitsSegments[8]= 0b1111111;
  digitsSegments[9]= 0b1111011;
}

////////////////////////////////////////////////////////////////////////////////
// Application etry point after setup() has executed.
//
void loop()
{
  
  initialize_DS3234();
  
  // If your RTC is new or has lost the correct time uncomment the line below,
  //  set the date and time to a UTC time couple of minutes ahead of the current time
  //  upload and then keep reset pressed until few seconds before the time you set.
  // You have now written current time to the RTC. Remember to COMMENT OUT this line
  //  and upload again immediately else you will continue to set back your clock 
  //  at every reset! 
  //SetTimeDate(26,2,14,18,34,00);
  
  // We use the current UTC time expressed as unix epoch (time) divided by 30. So, 
  //  in a sense this is not exactly a one time password, the generated value will
  //  stay valid for 30s. 
  long time = GetUnixTime() / 30;
  
  // We don't need the RTC anymore, free the lines for the display.
  Disable_DS3234();
  setPinsForDisplayOperation();

  // According to RFC4226, Page 6 we start with an HMAC-1 seeded wtih the secret key (K).
  // We use 16 bytes key in this example.
  Sha1.initHmac(secretKey,16);
  
  // We then push the counter (C) which in our case is the unix time. We push 8 bytes
  //  but the counter is shorted, so there will be zeros at the beginning.
  for(int ix=0; ix<8; ix++)
  {
   Sha1.write(((time >> ((7-ix)*8)) & 0xFF));
  }
  
  // This is now the full hash, it needs to be reuncated to get the OTP.
  uint8_t* hash = Sha1.resultHmac();
  
   // According to RFC4226, Page 7. The truncation function should be:
   //  DT(String) // String = String[0]...String[19]
   //   Let OffsetBits be the low-order 4 bits of String[19]
   //   Offset = StToNum(OffsetBits) // 0 <= OffSet <= 15
   //   Let P = String[OffSet]...String[OffSet+3]
   //   Return the Last 31 bits of P
   int  offset = hash[19] & 0xF; 
   long otp = 0;
   for (int ix = 0; ix < 4; ++ix) 
   {
    otp = otp << 8;
    otp = otp | hash[offset + ix];
   }
   
   // We then mask the higer bit to prevent issues with signed numbers.
   // According to RFC4226, Page 7:
   //  The reason for masking the most significant bit of P is to avoid
   //  confusion about signed vs. unsigned modulo computations.  Different
   //  processors perform these operations differently, and masking out the
   //  signed bit removes all ambiguity.
   otp = otp & 0x7FFFFFFF;
   
   // Finally we get a 6 digits number by getting the modulo 1E6 of the result.
   // As from example on RFC4226, Page 8.
   otp = otp % 1000000;
   
   // Show the number for 4 seconds.
   // Note that since we multiplex display digits inside displayNumber
   //   this needs to be called repeatedly until we want the number to
   //   be seen. Another way to do this is by interrupts but in this 
   //   simple example we have nothing else to do in this loop. 
   long startTime = millis();
   while(millis() - startTime < 4000)
   {
     displayNumber(otp);
   }
  
   // Power down. To show a new token user will reset the MCU by pressing
   //  the onboard reset button.
   set_sleep_mode(SLEEP_MODE_PWR_DOWN);
   sleep_enable();
   sleep_cpu();
   
}

////////////////////////////////////////////////////////////////////////////////
// Given a number it will display it with 6 digits including leading zeros.
//  (this is wanted as this is a password not just a number).
//
void displayNumber(long number)
{
   for(int p=0; p<6; p++)
   {
     displayDigit(number%10, p);
     number=number/10;
   } 
}

////////////////////////////////////////////////////////////////////////////////
// Given a single digit number it will display it in the specified position.
//
void displayDigit(byte digit, int pos)
{
  // First we set the anodes according to the contents of the 
  //  lookup table digitsSegments. The first anode correponds
  //  to bit 6 in the lookup table (segment a).
  // Anodes start at pin 2.
  for(int i=0; i<7; i++)
  {
    digitalWrite(2+i,(digitsSegments[digit]>>(6-i)) & 1);
  }
  
  // We flip low the cathode for a short period during which
  //  the segments will lit. We have 1mS per digit, times 8 digits
  //  the multiplex interval is 8mS, which means 125Hz, so it looks
  //  stable.
  digitalWrite(11 + pos, LOW); 
  delay(1);
  digitalWrite(11 + pos, HIGH); 
}



////////////////////////////////////////////////////////////////////////////////
// Prepare DS3234 for operation.
// 
int initialize_DS3234()
{
  // Preapare SPI bus. According to DS3234 datasheed we need MSB first and Mode1 
  pinMode(DS3234_CS_PIN,OUTPUT);
  SPI.begin();
  SPI.setBitOrder(MSBFIRST); 
  SPI.setDataMode(SPI_MODE1);
  
  // Control register. See DS3234 datasheet.
  // We basically disable all extra features (square wave output etc) but
  //  keep the oscillator running on backup battery (bit7 to zero).
  writeDS3234Register(0x0E, 0x00);
}

int setDateAndTime(int day, int month, int year, int hour, int minute, int second)
{
  writeDS3234BCDRegister(0x00, second);
  writeDS3234BCDRegister(0x01, minute);
  writeDS3234BCDRegister(0x02, hour);
  writeDS3234BCDRegister(0x04, day);
  writeDS3234BCDRegister(0x05, month);
  writeDS3234BCDRegister(0x06, year);
}

////////////////////////////////////////////////////////////////////////////////
// Gets the current time from the RTC and converts it to a unix timestamp.
// Year 2048 Compliant! Do not store unix time in signed ints.
//
uint32_t GetUnixTime()
{
    tmElements_t t;
    t.Year = 2000+readDS3234BCDRegister(6)-1970;
    t.Month = readDS3234BCDRegister(5) & 0x1F; // Bit7 is he century, we remove it.           
    t.Day = readDS3234BCDRegister(4);          
    t.Hour = readDS3234BCDRegister(2);     
    t.Minute = readDS3234BCDRegister(1);
    t.Second = readDS3234BCDRegister(0);
    return makeTime(t);
}

////////////////////////////////////////////////////////////////////////////////
// Reads a register containing a BCD number and converts it to a decimaal.
//
byte readDS3234BCDRegister(byte address)
{
  digitalWrite(DS3234_CS_PIN, LOW);
  SPI.transfer(address);
  byte bcdValue = SPI.transfer(0x00);
  digitalWrite(DS3234_CS_PIN, HIGH);
  return (bcdValue & 0xF) + ((bcdValue >> 4) * 10);
}

////////////////////////////////////////////////////////////////////////////////
// Writes to a register that contains BCD values the given decimal number.
//
void writeDS3234BCDRegister(byte address, byte value)
{
  // Convert the value to a two digits BCD.
  byte bcdValue = (value/10) << 4 + (value % 10);
  
  digitalWrite(DS3234_CS_PIN, LOW);
  SPI.transfer(address + 0x80);
  SPI.transfer(bcdValue);
  digitalWrite(DS3234_CS_PIN, HIGH);
}

////////////////////////////////////////////////////////////////////////////////
// Writes the given decimal number to the given register.
//
void writeDS3234Register(byte address, byte value)
{
  digitalWrite(DS3234_CS_PIN, LOW);
  SPI.transfer(address + 0x80);
  SPI.transfer(value);
  digitalWrite(DS3234_CS_PIN, HIGH);
}

////////////////////////////////////////////////////////////////////////////////
// Disables the DS3234 so that lines used by the SPI are free.
// Must be always followed by a call to setPinsForDisplayOperation before the 
//  display is used.
//
void Disable_DS3234()
{
  SPI.end();  
}
  
////////////////////////////////////////////////////////////////////////////////
// Cofigures the pins for display usage and ensures all digits are off.
//
void setPinsForDisplayOperation()
{
 for(int i=2; i<=17; i++)
  {
    pinMode(i, OUTPUT);
    
    // Low D2-D8 (anodes) and high D9-D17 (DS34 CS and cathodes),
    digitalWrite(i,(i<9)?LOW:HIGH);
  } 
}

