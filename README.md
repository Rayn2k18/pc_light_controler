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
- WS2812FX
- EEPROM

TODO : 
- edit the 2 lines (50 + 51) with your values :

#define WIFI_SSID "YOUR SSID"     // WiFi network

#define WIFI_PASSWORD "YOUR WIFI PASSWORD" // WiFi network password

- print your case (to avoid electrical shorts ^^), 1 available here : https://www.thingiverse.com/thing:4238962
