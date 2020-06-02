/*
    TODO : 
    - update ssid / password
    - update mqtt settings (or disable it below)
    
    - tidy code
    - add a function to control ws2812 settings / animations by mqtt subscribe

*/
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WS2812FX.h>
#include <EEPROM.h>
#include <ArduinoMqttClient.h>

extern const char index_html[];
extern const char main_js[];

#define WIFI_SSID "YOUR SSID"     // WiFi network
#define WIFI_PASSWORD "YOUR PASSWORD" // WiFi network password

// MQTT Settings
#define MQTT_ON     // to disable mqtt, comment this line
char* pubchan = "light_control/out";
const char* subchan = "light_control/in";
const char* mqtt_server = "192.168.200.254";
const uint16_t mqtt_port = 1883;


//#define STATIC_IP                       // uncomment for static IP, set IP below
#ifdef STATIC_IP
  IPAddress ip(192,168,0,123);
  IPAddress gateway(192,168,0,1);
  IPAddress subnet(255,255,255,0);
#endif

// QUICKFIX...See https://github.com/esp8266/Arduino/issues/263
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

#define LED_PIN 2                       // 0 = GPIO0, 2=GPIO2
#define LED_PIN2 0                       // 0 = GPIO0, 2=GPIO2
#define HDD_LED 13
#define PWR_BTON 12
#define PWR_LED 14
#define LED_COUNT 60        // 52
#define LED_COUNT2 80

bool pc_state,pwr_led_state,hdd_led_state = 0;

int trig_mode = 2;    // trig_mode : 0 = no trigger, 1= hdd_led trigger, 2 = Analog pin trigger
int analog_thr = 200;
int *panalog_thr = &analog_thr;

#define WIFI_TIMEOUT 30000              // checks WiFi every ...ms. Reset after this time, if WiFi cannot reconnect.
#define HTTP_PORT 80

#define DEFAULT_COLOR 0xFF5900
#define DEFAULT_BRIGHTNESS 128
#define DEFAULT_SPEED 3500
#define DEFAULT_MODE FX_MODE_COLOR_WIPE_RANDOM
// eeprom stored values
int pos_trig_anim = 0;
int trig_anim = FX_MODE_FIREWORKS_RANDOM;

int pos_trig_speed = pos_trig_anim + sizeof(trig_anim);
int trig_speed = 1750;
int *ptrig_speed = &trig_speed;

int pos_default_anim = pos_trig_speed+ sizeof(trig_speed);
int default_anim = DEFAULT_MODE;

int pos_default_speed = pos_default_anim+ sizeof(default_anim);
int default_speed = DEFAULT_SPEED;
int *pdefault_speed = &default_speed;
int pos_default_bright = pos_default_speed+ sizeof(default_speed);
int default_bright = DEFAULT_BRIGHTNESS;
int *pdefault_bright = &default_bright;

int pos_analog_trig = pos_default_bright+ sizeof(default_bright);

// -------------------

unsigned long auto_last_change = 0;
unsigned long last_wifi_check_time = 0;
unsigned long last_trig_time = 0;
unsigned long last_trig_check = 0;
String modes = "";
String form_eeprom = "";
uint8_t myModes[] = {}; // *** optionally create a custom list of effect/mode numbers
boolean auto_cycle = false;
boolean trigger_bool = false;

int lastBright, lastMode, lastSpeed = 0;

WS2812FX ws2812fx = WS2812FX(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
WS2812FX ws2812fx2 = WS2812FX(LED_COUNT2, LED_PIN2, NEO_GRB + NEO_KHZ800);
ESP8266WebServer server(HTTP_PORT);

#ifdef MQTT_ON 
WiFiClient wificlient;
MqttClient mqttClient(wificlient);
#endif

/*uint16_t flashRB(void) {
    for(uint16_t i=SEGMENT.start; i <= SEGMENT.stop; i++) {
      setPixelColor(i, BLACK);
    }

    uint16_t delay = 200 + ((9 - (SEGMENT.speed % 10)) * 100);
    uint16_t count = 2 * ((SEGMENT.speed / 100) + 1);
    if(SEGMENT_RUNTIME.counter_mode_step < count) {
      uint32_t color = ((SEGMENT_RUNTIME.counter_mode_step % 4) == 0) ? RED : BLUE;
      uint32_t color2 = ((SEGMENT_RUNTIME.counter_mode_step % 4) == 0) ? BLUE : RED;
      if((SEGMENT_RUNTIME.counter_mode_step %4 ) == 0 || (SEGMENT_RUNTIME.counter_mode_step %4 ) == 2) {
        
        for(uint16_t i=SEGMENT.start; i <= SEGMENT.stop/2; i++) {
          setPixelColor(i, color);
        }
        for(uint16_t i=SEGMENT.stop/2; i <= SEGMENT.stop; i++) {
          setPixelColor(i, color2);
        }
        delay = 100;
      } else {
        delay = 100;
      }
    }
    SEGMENT_RUNTIME.counter_mode_step = (SEGMENT_RUNTIME.counter_mode_step + 1) % (count + 1);
    return delay;
}*/

void eeWriteInt(int pos, int val) {   // write to eeprom
    EEPROM.begin(512);
    byte* p = (byte*) &val;
    EEPROM.write(pos, *p);
    EEPROM.write(pos + 1, *(p + 1));
    EEPROM.write(pos + 2, *(p + 2));
    EEPROM.write(pos + 3, *(p + 3));
    EEPROM.commit();
    EEPROM.end();
}

int eeGetInt(int pos) {   // read from eeprom
  int val;
  byte* p = (byte*) &val;
  EEPROM.begin(512);
  *p        = EEPROM.read(pos);
  *(p + 1)  = EEPROM.read(pos + 1);
  *(p + 2)  = EEPROM.read(pos + 2);
  *(p + 3)  = EEPROM.read(pos + 3);
  EEPROM.end();
  return val;
}

void setup(){
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.println("Starting...");
  
  trig_anim = eeGetInt(pos_trig_anim);  // read eeprom
  Serial.print("trig_anim = ");Serial.println(trig_anim);
  if (trig_anim < 0 or trig_anim > 59) trig_anim = FX_MODE_FIREWORKS_RANDOM;
  trig_speed = eeGetInt(pos_trig_speed);  // read eeprom
  Serial.print("trig_speed = ");Serial.println(trig_speed);
  if (trig_speed < 2 or trig_speed > 20000) trig_speed = 1750;
  
  default_anim = eeGetInt(pos_default_anim);  // read eeprom
  Serial.print("default_anim = ");Serial.println(default_anim);
  if (default_anim < 0 or default_anim > 59) default_anim = FX_MODE_COLOR_WIPE_RANDOM;
  default_speed = eeGetInt(pos_default_speed);  // read eeprom
  Serial.print("default_speed = ");Serial.println(default_speed);
  if (default_speed < 2 or default_speed > 20000) default_speed = 2500;
  default_bright = eeGetInt(pos_default_bright);  // read eeprom
  Serial.print("default_bright = ");Serial.println(default_bright);
  if (default_bright < 0 or default_bright > 255) default_bright = 128;
  
  analog_thr = eeGetInt(pos_analog_trig);  // read eeprom
  Serial.print("analog_thr = ");Serial.println(analog_thr);
  if (analog_thr < 0 or analog_thr > 1023) analog_thr = 200;
  
  modes.reserve(5000);
  modes_setup();

  Serial.println("WS2812FX setup");
  ws2812fx.init();
  ws2812fx.setMode(default_anim);
  ws2812fx.setColor(DEFAULT_COLOR);
  ws2812fx.setSpeed(default_speed);
  ws2812fx.setBrightness(default_bright);
  //ws2812fx.setCustomMode(flashRB);
  ws2812fx.start();
  
  ws2812fx2.init();
  ws2812fx2.setMode(default_anim);
  ws2812fx2.setColor(DEFAULT_COLOR);
  ws2812fx2.setSpeed(default_speed);
  ws2812fx2.setBrightness(default_bright);
  //ws2812fx.setCustomMode(flashRB);
  ws2812fx2.start();

  Serial.println("Wifi setup");
  wifi_setup();
  
  // MQTT ----------------------------------------------------
  #ifdef MQTT_ON 
    Serial.print("Attempting to connect to the MQTT broker: ");
    Serial.println(mqtt_server);

    if (!mqttClient.connect(mqtt_server, mqtt_port)) {
      Serial.print("MQTT connection failed! Error code = ");
      Serial.println(mqttClient.connectError());

      while (1);
    }

    Serial.println("You're connected to the MQTT broker!");
    Serial.println();
    
     // set the message receive callback
    mqttClient.onMessage(mqtt_callback);

    Serial.print("Subscribing to topic: ");
    Serial.println(subchan);
    Serial.println();

    // subscribe to a topic
    mqttClient.subscribe(subchan);
  #endif
 //--------------------------------------------------------
 
  Serial.println("HTTP server setup");
  server.on("/", srv_handle_index_html);
  server.on("/main.js", srv_handle_main_js);
  server.on("/modes", srv_handle_modes);
  server.on("/form_eeprom", srv_handle_form);
  server.on("/set", srv_handle_set);
  server.onNotFound(srv_handle_not_found);
  server.begin();
  Serial.println("HTTP server started.");
  pinMode(HDD_LED,INPUT_PULLUP);
  pinMode(PWR_LED,INPUT_PULLUP);
  pinMode(PWR_BTON,OUTPUT);
  digitalWrite(PWR_BTON, LOW); 
  
  pc_state = digitalRead(PWR_LED);
  Serial.print("PC Status : ");Serial.println(pc_state);
  Serial.println("ready!");
  
}


void loop() {
  unsigned long now = millis();

  server.handleClient();
  #ifdef MQTT_ON 
    mqttClient.poll();
  #endif
  ws2812fx.service();
  ws2812fx2.service();

  if(now - last_wifi_check_time > WIFI_TIMEOUT) {
    Serial.print("Checking WiFi... Wifi.Status = ");Serial.println(WiFi.status());
    if(WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi connection lost. Reconnecting...");
      wifi_setup();
    } else {
      Serial.println("OK");
    }
    last_wifi_check_time = now;
  }
  
  pwr_led_state = not digitalRead(PWR_LED);
  hdd_led_state = not digitalRead(HDD_LED);
  
  if(pc_state != pwr_led_state) {
      Serial.print("PC Status CHANGED : ");Serial.println(pwr_led_state);
      pc_state = pwr_led_state;
      #ifdef MQTT_ON 
        mqtt_send((String)pubchan+"/power", String(pwr_led_state));    // MQTT PUB
      #endif
      
      if(pc_state == 1) {
          ws2812fx.start();
          ws2812fx2.start();
          trig_mode = 2;
          trigger_show(true);
      } else {
          ws2812fx.stop();
          ws2812fx2.stop();          
      }
  }

  if(auto_cycle && (now - auto_last_change > 10000)) { // cycle effect mode every 10 seconds
    uint8_t next_mode = (ws2812fx.getMode() + 1) % ws2812fx.getModeCount();
    if(sizeof(myModes) > 0) { // if custom list of modes exists
      for(uint8_t i=0; i < sizeof(myModes); i++) {
        if(myModes[i] == ws2812fx.getMode()) {
          next_mode = ((i + 1) < sizeof(myModes)) ? myModes[i + 1] : myModes[0];
          break;
        }
      }
    }
    ws2812fx.setMode(next_mode);
    ws2812fx2.setMode(next_mode);
    Serial.print("mode is "); Serial.println(ws2812fx.getModeName(ws2812fx.getMode()));
    auto_last_change = now;
  }
  if (trig_mode != 0 and now - last_trig_check > 180) {
      last_trig_check = now;
      if (trig_mode == 1) { // HDD_LED mode
        
        Serial.print("HDD_LED : ");Serial.println(hdd_led_state);
        Serial.print("PWR_LED : ");Serial.println(pwr_led_state);
        if ( hdd_led_state == 1 ) {
          Serial.println("** TRIG ** - HDD : ");
          Serial.print("HDD_LED : ");Serial.println(hdd_led_state);
          Serial.print("PWR_LED : ");Serial.println(pwr_led_state);
          trigger_show(true); // trigger fireworks
        }
      } else { 
        int analog = analogRead(A0);
        if (analog > *panalog_thr) {
          trigger_show(true); // trigger fireworks
          Serial.print("** TRIG ** - Analog Level : ");Serial.println(analog);
        } else {
          //Serial.print("Analog Level : ");Serial.println(analog);
        }
      }
  }
  if (trigger_bool == true) {
    if(now - last_trig_time > 2500) {
      restore_show();
      trigger_bool == false;
    }
  }

    
}



/*
 * Connect to WiFi. If no connection is made within WIFI_TIMEOUT, ESP gets resettet.
 */
void wifi_setup() {
  if(WiFi.isConnected()) {
      Serial.println("false positive, return to normal ...");
      return ;
  }
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.mode(WIFI_STA);
  #ifdef STATIC_IP  
    WiFi.config(ip, gateway, subnet);
  #endif

  unsigned long connect_start = millis();
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");

    if(millis() - connect_start > WIFI_TIMEOUT) {
      Serial.println();
      Serial.print("Tried ");
      Serial.print(WIFI_TIMEOUT);
      Serial.print("ms. Resetting ESP now.");
      ESP.reset();
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}


/*
 * Build <li> string for all modes.
 */
void modes_setup() {
  modes = "";
  uint8_t num_modes = sizeof(myModes) > 0 ? sizeof(myModes) : ws2812fx.getModeCount();
  for(uint8_t i=0; i < num_modes; i++) {
    uint8_t m = sizeof(myModes) > 0 ? myModes[i] : i;
    modes += "<li><a href='#' class='m' id='";
    modes += m;
    modes += "'><span align=left>";
    modes += m;
    modes += "</span> - &nbsp;";
    modes += ws2812fx.getModeName(m);
    modes += "</a></li>";
  }
}

String list_anim(int def_val) {
  uint8_t num_modes = sizeof(myModes) > 0 ? sizeof(myModes) : ws2812fx.getModeCount();
  String text = "";
  for(uint8_t i=0; i < num_modes; i++) {
    uint8_t m = sizeof(myModes) > 0 ? myModes[i] : i;
    text += "<option value='";
    text += m;
    text += "'";
    if(def_val == int(m)) text +=" selected ";
    text += ">";
    text += m;
    text += " - &nbsp;";
    text += ws2812fx.getModeName(m); 
    text += "</option>";
  }
  
  return text;
}

String html_eeprom_setup() {
  form_eeprom = 
  "<form action='set' method='get' >"
  "   Default Speed : <input name='def_speed' type='range' min='1' max='20000' value='";
  form_eeprom += *pdefault_speed;
  form_eeprom += "' step='10' Onchange=\"showValue(this.value,'range_def_spd')\"><span id='range_def_spd'>";
  form_eeprom += *pdefault_speed;
  form_eeprom += "</span><br>"
  "   Default Anim : <select name='def_anim'>";
  form_eeprom += list_anim(default_anim);
  form_eeprom +="</select><br>"
  "    Default Brightness : <input name='def_bright' type='range' min='0' max='255' value='";
  form_eeprom += *pdefault_bright;
  form_eeprom += "' Onchange=\"showValue(this.value,'range_def_bri')\"><span id='range_def_bri'>";
  form_eeprom += *pdefault_bright;
  form_eeprom += "</span><br>"
  "    Trigger Speed : <input name='trig_speed' type='range' min='1' max='20000' value='";
  form_eeprom += *ptrig_speed;
  form_eeprom += "' step='10' Onchange=\"showValue(this.value,'range_trig_spd');\"><span id='range_trig_spd'>";   
  form_eeprom += *ptrig_speed;
  form_eeprom += "</span><br>"
  "   Trigger Anim : <select name='trig_anim'>";
  form_eeprom += list_anim(trig_anim);
  form_eeprom +="</select><br>"
  "   Trigger Analog Level : <input name='trig_lvl' type='range' min='1' max='1023' value='";
  form_eeprom += *panalog_thr;
  form_eeprom += "' step='10' Onchange=\"showValue(this.value,'range_ana_lvl');submitVal('trig_lvl', this.value);\"><span id='range_ana_lvl'>";   
  form_eeprom += *panalog_thr;
  form_eeprom += "</span>"
  "    <input type='submit' name='valid' value='Store to eeprom'>"
  "  </form>"
  "</div>";
  return form_eeprom;
}

/* #####################################################
#  Webserver Functions
##################################################### */

void srv_handle_not_found() {
  server.send(404, "text/plain", "File Not Found");
}

void srv_handle_index_html() {
  server.send_P(200,"text/html", index_html);
}

void srv_handle_main_js() {
  server.send_P(200,"application/javascript", main_js);
}

void srv_handle_modes() {
  server.send(200,"text/plain", modes);
}

void srv_handle_form() {
  form_eeprom = html_eeprom_setup();
  server.send(200,"text/plain", form_eeprom);
}


void trigger_show(bool trig_fw) {
    last_trig_time = millis();
    if (trigger_bool == false) {
      lastBright = ws2812fx.getBrightness();
      lastSpeed = ws2812fx.getSpeed();
      lastMode = ws2812fx.getMode();
    }
    Serial.println(" *** TRIGGER ! ***");
   if(trig_fw == 1) {
      if (trig_mode == 2) {
        ws2812fx.setBrightness(255);
        ws2812fx.setSpeed(1750);
        ws2812fx.setMode(FX_MODE_FIREWORKS_RANDOM); // fireworks random
        
        ws2812fx2.setBrightness(255);
        ws2812fx2.setSpeed(1750);
        ws2812fx2.setMode(FX_MODE_FIREWORKS_RANDOM); // fireworks random
      } else {
        ws2812fx.setBrightness(255);
        ws2812fx.setSpeed(1750);
        ws2812fx.setMode(FX_MODE_FIREWORKS_RANDOM); // fireworks
        
        ws2812fx2.setBrightness(255);
        ws2812fx2.setSpeed(1750);
        ws2812fx2.setMode(FX_MODE_FIREWORKS_RANDOM); // fireworks
      }
      //ws2812fx.service();
      Serial.print("lastmode is ");Serial.println(lastMode);
      ws2812fx.trigger();
      ws2812fx2.trigger();
  } else {
      ws2812fx.setBrightness(255);
      ws2812fx.setSpeed(1700);
      ws2812fx.trigger();
      
      ws2812fx2.setBrightness(255);
      ws2812fx2.setSpeed(1700);
      ws2812fx2.trigger();
  }
  trigger_bool = true;
}

void restore_show() {
  Serial.print("-- Restore : bright = ");Serial.print(lastBright);Serial.print(", speed = ");Serial.print(lastSpeed);Serial.print(", mode = ");Serial.println(lastMode);
  ws2812fx.setBrightness(lastBright);
  ws2812fx.setSpeed(lastSpeed);
  ws2812fx.setMode(lastMode);
  
  ws2812fx2.setBrightness(lastBright);
  ws2812fx2.setSpeed(lastSpeed);
  ws2812fx2.setMode(lastMode);
  trigger_bool = false;
}

void srv_handle_set() {
  String str_valid = server.arg("valid");
  bool valid = false;
  
  if (str_valid != "") valid = true;
  Serial.print("str_valid = ");Serial.print(str_valid);Serial.print(" / valid : ");Serial.println(valid);
  for (uint8_t i=0; i < server.args(); i++){
      
    #ifdef MQTT_ON 
      mqtt_send((String)pubchan+"/"+server.argName(i), server.arg(i).c_str());    // MQTT PUB
    #endif
    
    if(server.argName(i) == "c") {
      uint32_t tmp = (uint32_t) strtol(server.arg(i).c_str(), NULL, 16);
      if(tmp >= 0x000000 && tmp <= 0xFFFFFF) {
        ws2812fx.setColor(tmp);
        ws2812fx2.setColor(tmp);
      }
    }

    if(server.argName(i) == "m") {
      uint8_t tmp = (uint8_t) strtol(server.arg(i).c_str(), NULL, 10);
      ws2812fx.setMode(tmp % ws2812fx.getModeCount());
      ws2812fx2.setMode(tmp % ws2812fx.getModeCount());
      Serial.print("mode is "); Serial.println(ws2812fx.getModeName(ws2812fx.getMode()));
    }

    if(server.argName(i) == "b") {
      if(server.arg(i)[0] == '-') {
        ws2812fx.setBrightness(ws2812fx.getBrightness() * 0.8);
        ws2812fx2.setBrightness(ws2812fx.getBrightness() * 0.8);
      } else if(server.arg(i)[0] == ' ') {
        ws2812fx.setBrightness(min(max(ws2812fx.getBrightness(), 5) * 1.2, 255));
        ws2812fx2.setBrightness(min(max(ws2812fx.getBrightness(), 5) * 1.2, 255));
      } else { // set brightness directly
        uint8_t tmp = (uint8_t) strtol(server.arg(i).c_str(), NULL, 10);
        ws2812fx.setBrightness(tmp);
        ws2812fx2.setBrightness(tmp);
      }
      Serial.print("brightness is "); Serial.println(ws2812fx.getBrightness());
    }

    if(server.argName(i) == "s") {
      if(server.arg(i)[0] == '-') {
        ws2812fx.setSpeed(max(ws2812fx.getSpeed(), 5) * 1.2);
        ws2812fx2.setSpeed(max(ws2812fx.getSpeed(), 5) * 1.2);
      } else if(server.arg(i)[0] == ' ') {
        ws2812fx.setSpeed(ws2812fx.getSpeed() * 0.8);
        ws2812fx2.setSpeed(ws2812fx.getSpeed() * 0.8);
      } else {
        uint16_t tmp = (uint16_t) strtol(server.arg(i).c_str(), NULL, 10);
        ws2812fx.setSpeed(tmp);
        ws2812fx2.setSpeed(tmp);
      }
      Serial.print("speed is "); Serial.println(ws2812fx.getSpeed());
    }

    if(server.argName(i) == "a") {
      if(server.arg(i)[0] == '-') {
        auto_cycle = false;
      } else {
        auto_cycle = true;
        auto_last_change = 0;
      }
    }
     if(server.argName(i) == "trig") {
       if(server.arg(i)[0] == '1') {
          trigger_show(true); // trigger fireworks
      } else {
          trigger_show(false);  //trigger current animation
      }
    }
    if(server.argName(i) == "trig_mode") {
      if(server.arg(i)[0] == '1') {   // hdd_led triger
          trig_mode = 1;
          Serial.println("trigger mode is HDD_LED");
      } else if(server.arg(i)[0] == '2') {   // analog pin triger
          trig_mode = 2;
          Serial.println("trigger mode is Analog");
      } else {
          trig_mode = 0;    // no auto trigger
          Serial.println("trigger mode is disabled");
      }
    }
    if(server.argName(i) == "power_bton") {
      if(server.arg(i)[0] == '1') {   // power on
          digitalWrite(PWR_BTON,HIGH);
          delay(600);
          digitalWrite(PWR_BTON, LOW);
          Serial.println("**** POWER ON BUTON ! ****");
      }
      if(server.arg(i)[0] == '9') {   // force power off
          digitalWrite(PWR_BTON,HIGH);
          delay(32000);
          digitalWrite(PWR_BTON, LOW);
          Serial.println("**** FORCE POWER OFF !! ****");
      }
    }
    
      if(server.argName(i) == "def_speed") {
          int tmp = server.arg(i).toInt();
          if(abs(tmp - *pdefault_speed) > 2 and valid){
              Serial.print("default speed value is different ... STORING ");Serial.print(tmp);Serial.print(" , old val : ");Serial.println(*pdefault_speed);
              eeWriteInt(pos_default_speed, tmp);
          } else {
              Serial.print("default speed value is the same ... SET "); Serial.println(tmp);
              ws2812fx.setSpeed(tmp);
              ws2812fx2.setSpeed(tmp);
              *pdefault_speed = tmp;
          }
      }
      if(server.argName(i) == "def_anim") {
          uint8_t tmp = (int) strtol(server.arg(i).c_str(), NULL, 10);
          if(tmp != default_anim and valid){
              Serial.print("default anim value is different ... STORING ");Serial.print(tmp);Serial.print(" , old val : ");Serial.println(default_anim);
              eeWriteInt(pos_default_anim, tmp);
          } else {
              Serial.print("default anim value is the same ... no action. "); Serial.println(tmp);
          }
      }
      if(server.argName(i) == "def_bright") {
          uint8_t tmp = (int) strtol(server.arg(i).c_str(), NULL, 10);
          if(tmp != default_bright and valid){
              Serial.print("default brightness value is different ... STORING ");Serial.print(tmp);Serial.print(" , old val : ");Serial.println(default_bright);
              eeWriteInt(pos_default_bright, tmp);
          } else {
              Serial.print("default brightness value is the same ... SET "); Serial.println(tmp);
              ws2812fx.setBrightness(tmp);
              ws2812fx2.setBrightness(tmp);
              *pdefault_bright = tmp;
          }
      }
      if(server.argName(i) == "trig_speed") {
          int tmp = server.arg(i).toInt();
          if(abs(tmp - trig_speed) > 2  and valid){
              Serial.print("trigger speed value is different ... STORING ");Serial.print(tmp);Serial.print(" , old val : ");Serial.println(trig_speed);
              eeWriteInt(pos_trig_speed, tmp);
          } else {
              Serial.print("trigger speed value is the same ... no action. "); Serial.println(tmp);
              *ptrig_speed = tmp;
          }
      }
      if(server.argName(i) == "trig_anim") {
          uint8_t tmp = (int) strtol(server.arg(i).c_str(), NULL, 10);
          if(tmp != trig_anim  and valid){
              Serial.print("trigger anom value is different ... STORING ");Serial.print(tmp);Serial.print(" , old val : ");Serial.println(trig_anim);
              eeWriteInt(pos_trig_anim, tmp);
          } else {
              Serial.print("trigger anim value is the same ... no action. "); Serial.println(tmp);
              trig_anim = tmp;
          }
      }
      if(server.argName(i) == "trig_lvl") {
          int tmp = server.arg(i).toInt();
          if(abs(tmp - eeGetInt(pos_analog_trig)) > 2  and valid){
              Serial.print("Analog trigger value is different ... STORING ");Serial.print(tmp);Serial.print(" , old val : ");Serial.println(analog_thr);
              eeWriteInt(pos_analog_trig, tmp);
          } else {
              Serial.print("Analog trigger value is the same ... no action. "); Serial.println(tmp);
              *panalog_thr = tmp;
          }
      }
    
    if(server.argName(i) == "reset") {
      server.send(200, "text/plain", "OK");
      delay(2000);
      ESP.reset();
      exit(0);
    }
    
    if(server.argName(i) == "off") {
      trig_mode = 0;
      ws2812fx.stop();
      ws2812fx2.stop();
    }
    
    if(server.argName(i) == "on") {
      ws2812fx.start();
      ws2812fx2.start();
      trig_mode = 2;
    }
  }
  server.send(200, "text/plain", "OK");
}

#ifdef MQTT_ON 
 // MQTT Functions ------------------------------
void mqtt_send(String topic, String message) {
    Serial.print("Sending message to topic: ");
    Serial.println(topic);
    Serial.print(message);
    Serial.println();

    // send message, the Print interface can be used to set the message contents
    mqttClient.beginMessage(topic);
    mqttClient.print(message);
    mqttClient.endMessage();

    Serial.println();

}

void mqtt_callback(int messageSize) {
  // we received a message, print out the topic and contents
  Serial.println("Received a message with topic '");
  Serial.print(mqttClient.messageTopic());
  Serial.print("', length ");
  Serial.print(messageSize);
  Serial.println(" bytes:");
  String value_mqtt;

  // use the Stream interface to print the contents
  while (mqttClient.available()) {
    value_mqtt = mqttClient.read();
    Serial.print(value_mqtt);
  }
  Serial.println();
  
  Serial.print("Wifi Status : ");
  Serial.print(WiFi.status());
  Serial.print(" / ");
  Serial.print(wificlient.status());
  Serial.print(" -> ");
  Serial.println(WiFi.isConnected());
  
  Serial.println();
}
#endif
// -------------------------------------------------
