Aims : 
- control RGB lights from my computer case (rgb lights are basically ws2812b on 5v led strips)
- trigger animation from hdd led activity or audio output
- remember my settings :
	- which animation to trigger at start, 
	- on external trigger
	- animation speed
	- trigger level (for audio)
	- brightness
- power on / power off my PC (since wake on lan doesn't work on my MB)

Needed components : 
- 1x ESP8266 (in my case : lolin v3)
- 1x PCB (schema on kicad directory)
- 6x 470ohms resistors
- 3x DIP-4 817C Optocouplers
- 2x 4 pins connectors (1 pin is to be removed)
- 6x 2 pins connectors
- 1x jack 3.5 connector (audio trigger IN)

Needed librairies : 
- ESP8266WiFi
- ESP8266WebServer
- WS2812FX (code based on "esp8266_webinterface" example files from that library)
- EEPROM

Control URLs (GET) : <br>
<b>http://&#60;@IP of ESP&#62;/set?</b>&#60;command1&#62;<b>&</b>&#60;command2&#62;<b>&</b>&#60;command3&#62;...

commands can be :
	- power_bton=1		: trigger power button for 600ms (power on / off / sleep computer)
	- power_bton=9		: trigger power button for 32s (force power off computer)
	- on			: starts animation
	- off			: power off leds
	- reset			: restarts ESP
	- trig_mode=0  		: trigger mode disabled
	- trig_mode=1  		: HDD led triggers animation
	- trig_mode=2  		: audio level triggers animation (default)
	- trig=1		: triggers the "random fireworks" animation
	- trig			: triggers the current animation
	- m=&#60;0-56/59&#62;		: switch to animation (see http://&#60;IP of ESP&#62;/ for numbers)
	- b=&#60;0-255&#62;		: sets brightness
	- s=&#60;number&#62;		: sets animation speed
	- c=&#60;000000-ffffff&#62;	: sets color
	- a=+			: autocycle = on
	- a=-			: autocycle = off

TODO : 
- edit the 2 lines (50 + 51) with your values :

#define WIFI_SSID "YOUR SSID"     // WiFi network

#define WIFI_PASSWORD "YOUR WIFI PASSWORD" // WiFi network password

- print your case (to avoid electrical shorts ^^), 1 available here : https://www.thingiverse.com/thing:4238962
