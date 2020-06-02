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
<ul><b>http://&#60;@IP of ESP&#62;/set?</b>&#60;command1&#62;<b>&</b>&#60;command2&#62;<b>&</b>&#60;command3&#62;...</ul>
commands can be :
<ul>
	<li> power_bton=1		: trigger power button for 600ms (power on / off / sleep computer)</li>
	<li> power_bton=9		: trigger power button for 32s (force power off computer)</li>
	<li> on			: starts animation</li>
	<li> off			: power off leds</li>
	<li> reset			: restarts ESP</li>
	<li> trig_mode=0  		: trigger mode disabled</li>
	<li> trig_mode=1  		: HDD led triggers animation</li>
	<li> trig_mode=2  		: audio level triggers animation (default)</li>
	<li> trig=1		: triggers the "random fireworks" animation</li>
	<li> trig			: triggers the current animation</li>
	<li> m=&#60;0-56/59&#62;		: switch to animation (see http://&#60;IP of ESP&#62;/ for numbers)</li>
	<li> b=&#60;0-255&#62;		: sets brightness</li>
	<li> s=&#60;number&#62;		: sets animation speed</li>
	<li> c=&#60;000000-ffffff&#62;	: sets color</li>
	<li> a=+			: autocycle = on</li>
	<li> a=-			: autocycle = off</li>
</ul>

TODO : 
<ul>
	<li>edit the 4 lines (50+51, 70+71) with your values :</li>
	<ul><i>
		<li>#define WIFI_SSID "<b>YOUR SSID</b>"     // WiFi network</li>
		<li>#define WIFI_PASSWORD "<b>YOUR WIFI PASSWORD</b>" // WiFi network password</li>
	<br>
		<li>#define LED_COUNT <b>60</b>        // number of leds of your strip N°1</li>
		<li>#define LED_COUNT2 <b>80</b>		// number of leds of your strip N°2 </li>
	</ul>
	<br>
	<li>Disable MQTT support if you don't want it (comment line 23) OR Update MQTT Settings  (broker, port, topics from line 24 to 27)</li>
	<li>/!\ by default, when your computer is not ON (power led = off), no animation is shown (except some trigger by jack or hdd led if configured so), </li>
	<br>	So if you don't see any led animation, check that power led pin is connected and on the right way (check polarity)<br>
	<br>	to disable this behaviour, comment lines from 276 to 294</li>
	<br>
	<li>print your case (to avoid electrical shorts ^^), 1 available here : <a href="https://www.thingiverse.com/thing:4238962">https://www.thingiverse.com/thing:4238962</a></li>
</ul>

