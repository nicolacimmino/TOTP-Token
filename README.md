HOTP-Token
==========

A simple implementation of a time based OTP token based on HOTP open standard

Background
==========

While working at another project I came aross HMAC-SHA1 (RFC 2104) and HOTP (HMAC-based One Time Password) algorithm (RFC 4226). The same day I was browsing trough my junk box and found an 8 digits LED dispaly that was salvaged from an old calculator. I couldn't resist and I had to get my hands dirty and whip up an hardware token. I was going for the one time password seeded by a rolling code when I found also an RTC breakout board in the same box, so I decided it would have been more cool to make a time based token.

Few hours later the prototype shown below was ready and, with great amazement, a software token on Android programmed with the same key kept constantly spitting out the same token as the display!

On the front of the board is the ancient display. Note that the right most digit was busted and the left most glass bubble is not a digit in truth, anyhow enough digits left to show the 6 digits version of the HOTP. At the bottom an Arduino nano mounting an ATMEL ATmega168.

![Proto Front](https://raw.github.com/nicolacimmino/HOTP-Token/master/images/ProtoFront.jpg)

On the back the RTC breakout board with the backup battery visible. The actual RTC chip (not visible as it is on the bottom of the breakout board) is a DS3234.

![Proto Back](https://raw.github.com/nicolacimmino/HOTP-Token/master/images/ProtoBack.jpg)

I make use of the Arduino Cryptolib (https://github.com/Cathedrow/Cryptosuite) which sped up te job incredibly as it offers ready implementation for the HMAC-SHA1 so all I had to do was to drive the RTC, the display and implement the actual HOTP calculation.
 

