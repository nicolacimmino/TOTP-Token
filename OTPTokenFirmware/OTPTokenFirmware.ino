// OTPToken implements HOTP (HMAC-based One Time Password) described in RFC4226,
//  to generate a time based authentication token that can be used, for instance,
//  in a two factors authentication system.
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
// D2  -> Display VCC
// D3  -> Display SCL 
// D4  -> Display SDA
// D5  -> Display RST
// D6  -> Display D/C
// D9  -> DS3234 CS
// D10 -> DS3234 SS
// D11 -> DS3234 MOSI
// D12 -> DS3234 MISO
// D13 -> DS3234 SCL
//

#include <Time.h>        // Arduino Time library (http://playground.arduino.cc/Code/Time)
#include "sha1.h"        // Arduino Cryptosuite  (https://github.com/Cathedrow/Cryptosuite) 
#include <avr/sleep.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>       // Adafruit GFX Lib (https://github.com/adafruit/Adafruit-GFX-Library)
#include <Adafruit_SSD1306.h>   // Adafruit SSD1306 Lib (https://github.com/adafruit/Adafruit_SSD1306)

// Display and RTC pins.
#define OLED_VCC      2
#define OLED_MOSI     4 
#define OLED_CLK      3
#define OLED_DC       6
#define OLED_CS       12
#define OLED_RESET    5 
#define DS3234_CS_PIN 10

// Display controller.
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

// This is the secret key.
uint8_t secretKey[]={ 0x32, 0x83, 0x56, 0x4f, 0x33, 0x9a, 0xac, 0x21, 0x94, 0x82, 0xcc, 0x3f, 0x48, 0x67, 0x12, 0x3c };

////////////////////////////////////////////////////////////////////////////////
// Application setup after reset.
//
void setup()
{
}

////////////////////////////////////////////////////////////////////////////////
// Application entry point after setup() has executed.
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
  //setDateAndTime(28,3,14,19,49,00);
  
  // We use the current UTC time expressed as unix epoch (time) divided by 30. So, 
  //  in a sense this is not exactly a one time password, the generated value will
  //  stay valid for 30s. 
  long time = GetUnixTime() / 30;
  
  // Power up display.
  pinMode(OLED_VCC, OUTPUT);
  digitalWrite(OLED_VCC,HIGH);

  // generate the high voltage from the 3.3v line internally.
  display.begin(SSD1306_SWITCHCAPVCC);
  display.clearDisplay();   // clears the screen and buffer
  

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
   
  display.setTextSize(2);
  display.setTextColor(WHITE);
  
   // Show the number until the TOTP expires.
  long expiryTime = (time + 1) * 30;
  long timeLeft = expiryTime - GetUnixTime();
  while(timeLeft > 1)
  {
      // First we print the TOTP
      display.setCursor(29,10);
      display.clearDisplay();
      for(int p=5;p>0;p--)
      {
        display.print((long)floor(otp / pow(10,p))%10);
      }
      display.println(otp%10);
      
      display.drawRect(34,40, 60, 10 ,WHITE);
      display.fillRect(34,40, timeLeft*2, 10 ,WHITE);
      display.display();  
      delay(1000);
      timeLeft = expiryTime - GetUnixTime();
  }
   
  // Change all lines connected to display to inputs, if we
  //  ground VCC then current will flow trough the clamps
  //  inside the display and take power also when the device is off.
  for(int p=2;p<=6;p++)
    pinMode(p, INPUT);

  // Power down the RTC so it doesn't take power.
  Disable_DS3234(); 
   
  // Power down the processor. To show a new token user will reset the MCU by pressing
  //  the onboard reset button.
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_cpu(); 
}


////////////////////////////////////////////////////////////////////////////////
// Prepare DS3234 for operation.
// 
int initialize_DS3234()
{
  // We use two pins to power the RTC so we
  //  can programmatically shut it down to preserve
  //  battery. Its onboard battery will keep the time.
  pinMode(A2,OUTPUT);
  pinMode(A1,OUTPUT);
  digitalWrite(A1, HIGH);  // VCC
  digitalWrite(A2, LOW);   // GND
  
  // Preapare SPI bus. According to DS3234 datasheed we need MSB first and Mode 1 
  pinMode(DS3234_CS_PIN,OUTPUT);
  SPI.begin();
  SPI.setBitOrder(MSBFIRST); 
  SPI.setDataMode(SPI_MODE1);
  
  // Control register. See DS3234 datasheet.
  // We basically disable all extra features (square wave output etc) but
  //  keep the oscillator running on backup battery (bit7 to zero).
  writeDS3234Register(0x0E, 0x00);
}

int setDateAndTime(int v_day, int v_month, int v_year, int v_hour, int v_minute, int v_second)
{
  writeDS3234BCDRegister(0x00, v_second);
  writeDS3234BCDRegister(0x01, v_minute);
  writeDS3234BCDRegister(0x02, v_hour);
  writeDS3234BCDRegister(0x04, v_day);
  writeDS3234BCDRegister(0x05, v_month);
  writeDS3234BCDRegister(0x06, v_year);
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
  byte bcdValue = ((value/10) << 4) + (value % 10);
  
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
  digitalWrite(A1, LOW);
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

