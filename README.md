# Simple_CW_keyer
Basic CW keyer for an Arduino Nano.

Wtitten by Paul VK3HN, 7 Apr 2017.  
A description and wiring diagram is at https://vk3hn.wordpress.com/Arduino-CW-keyer-for-a-BiTx-or-other-homebrew-rig

This code may be duplicated, used, and hacked for hobby or experimental purposes. Please acknowledge the source (VK3HN) if you do.

The scripts assumes the following use of Nano IO lines: 

D2 - keyed line, goes high with each key-down (dot or dash)  

D3 - PTT line, goes high while transmitting, then reverts to low after BREAK_IN_DELAY mS (set to 800)   

D8 - a square wave tone output at CW_TONE_HZ Hz (set to 700) 

D9 & 10 - Pushbuttons to earth for 2 (hardcoded) messages, you can add more

D11 & D12 - paddle left and right, paddle center earthed 

A3 - Potentiometer wiper across 5v for keyer speed control. 

You can connect an 8 ohm speaker to D8 with a 220 ohm series resistor for audible tone.
To switch a relay or LED on the keyed or PTT lines, take D2/D3 to the gate of a 2N7000, 100k resistor from gate to earth, source to earth, drain to +12v via load (relay, LED with 220 ohm current limiting resistor).   

All parameters are #define'd at the top and should be self-explanatory.

The script also provides a CW Ident, sends callsign every CW_IDENT_SECS seconds (set to 180).  
To activate this feature, #define CW_IDENT.

NOTE: The master code includes enums -- these may generate compile time errros like these:
  key:42: error: variable or field ‘set_key_state’ declared void
  key:42: error: ‘key_state_e’ was not declared in this scope
  key:43: error: variable or field ‘activate_state’ declared void
  key:43: error: ‘trx_state_e’ was not declared in this scope
  variable or field ‘set_key_state’ declared void
  
  A version without enums is on branch 'sans_enums' -- if you have problems compiling the master, use the code on this branch instead.  
