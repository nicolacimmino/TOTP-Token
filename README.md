HOTP-Token
==========

A simple implementation of a time based OTP token based on HOTP open standard

Background
==========

While working at another project I came aross HMAC-SHA1 (RFC 2104) and HOTP (HMAC-based One Time Password) algorithm (RFC 4226). The same day I was browsing trough my junk box and found an 8 digits LED dispaly that was salvaged from an old calculator. I couldn't resist and I had to get my hands dirty and whip up an hardware token. I was going for the one time password seeded by a rolling code when I found also an RTC breakout board in the same box, so I decided it would have been more cool to make a time based token.

Few hours later the prototype shown below was ready and, with great amazement, a software token on Android programmed with the same key kept constantly spitting out the same token as the display!

![Proto Front](https://raw.com/nicolacimmino/HOTP-Token/master/images/ProtoFront.jpg)

![Proto Back](https://raw.com/nicolacimmino/HOTP-Token/master/images/ProtoBack.jpg)


