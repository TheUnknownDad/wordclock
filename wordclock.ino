// Compiler Setup WeMos D1 R2 & mini, 80Mhz, 4M (1MB SPIFFS), 230400
#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_ESP8266_D1_PIN_ORDER
#include "FastLED.h"

FASTLED_USING_NAMESPACE

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

#include <TimeLib.h>
#include <Timezone.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

const char compiletime[] = __TIME__;
const char compiledate[] = __DATE__;
#define SW_VERSION "1.0.10"

#define DEBUG   //If you comment this line, the DPRINT & DPRINTLN lines are defined as blank.
#ifdef DEBUG    //Macros are usually in all capital letters.
  #define DPRINT(...)    Serial.print(__VA_ARGS__)     //DPRINT is a macro, debug print
  #define DPRINTLN(...)  Serial.println(__VA_ARGS__)   //DPRINTLN is a macro, debug print with new line
#else
  #define DPRINT(...)     //now defines a blank line
  #define DPRINTLN(...)   //now defines a blank line
#endif

// button setup
#define BUTTON_LEFT       D7
#define BUTTON_RIGHT      D5
#define FN_NONE           0
#define FN_SETUP_AP       1
#define FN_TOGGLE_OTA     2
#define STATE_PRESSED     1
#define STATE_UNPRESSED   0
byte button_left_function  = FN_SETUP_AP;
byte button_right_function = FN_TOGGLE_OTA;
byte button_left_last_state  = STATE_UNPRESSED;
byte button_right_last_state = STATE_UNPRESSED;

// wifi definitions
const char* ssid = "StayAway";
const char* password = "DasKannIchMirNichtMerk3n.";
const char* myHostname = "wordclock";

// NTP definitions
static const char ntpServerName[] = "de.pool.ntp.org";
const int timeZone = 0;     // Central European Time
WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets
time_t getNtpTime();

// timezone definitions
TimeChangeRule CEST = { "CEST", Last, Sun, Mar, 2, 120 };     //Central European Summer Time
TimeChangeRule CET = { "CET ", Last, Sun, Oct, 3, 60 };       //Central European Standard Time
Timezone CE(CEST, CET);
TimeChangeRule *tcr;        //pointer to the time change rule, use to get the TZ abbrev
time_t utc, local;

// LED defintions
#define DATA_PIN    D2
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS    110
CRGB leds[NUM_LEDS];
CRGB activeColor;
CRGB lastColor;
CRGB fadeColor;

#define LED_BRIGHTNESS      255
#define FRAMES_PER_SECOND   30

// LED matrix definitions
#define LED_SEGMENTS         27
#define LED_SEGMENT_ES        0
#define LED_SEGMENT_VIERTEL   1
#define LED_SEGMENT_ZEHN      2
#define LED_SEGMENT_FUENF     3 
#define LED_SEGMENT_HALB      4
#define LED_SEGMENT_NACH      5
#define LED_SEGMENT_Z_EIN     6
#define LED_SEGMENT_VOR       7
#define LED_SEGMENT_Z_EINS    8
#define LED_SEGMENT_Z_ZEHN    9
#define LED_SEGMENT_Z_SIEBEN 10
#define LED_SEGMENT_Z_ZWEI   11
#define LED_SEGMENT_Z_FUENF  12
#define LED_SEGMENT_Z_SECHS  13
#define LED_SEGMENT_Z_ACHT   14
#define LED_SEGMENT_Z_ELF    15
#define LED_SEGMENT_Z_NEUN   16
#define LED_SEGMENT_Z_VIER   17
#define LED_SEGMENT_Z_ZWOELF 18
#define LED_SEGMENT_Z_DREI   19
#define LED_SEGMENT_IST      20
#define LED_SEGMENT_DREI     21
#define LED_SEGMENT_ZWANZIG  22
#define LED_SEGMENT_UHR      23
#define LED_SEGMENT_WIFI     24
#define LED_SEGMENT_AN       25
#define LED_SEGMENT_AUS      26

#define LED_MAX_POWER_MILLIAMP        600
#define LED_VOLTAGE                   5
int led_segment_pos[LED_SEGMENTS] =    { 9,22,11, 0,51,40,55,33,55,99,93,62,44,77,84,47,102,66,88,73, 5,29,15,107,36,59,70};
int led_segment_length[LED_SEGMENTS] = { 2, 7, 4, 4, 4, 4, 3, 3, 4, 4, 6, 4, 4, 5, 4, 3,  4, 4, 5, 4, 3, 4, 7,  3, 4, 2, 3};
int led_max_power_milliamp = LED_MAX_POWER_MILLIAMP;
unsigned long led_segment_state = 0;
unsigned long led_segment_last_state = 0;

// menu und animation definitions and variables
bool show_clock = true;
bool show_effect_breathe = true;
bool show_effect_colorchange = true;
bool show_effect_smooth_fade = true;
int effect_smooth_fade_time_ms = 5000;
long effect_smooth_fade_start = 0;
byte effect_smooth_fade_fader = 0;
long effect_time = 0;
int effect_breathe_breathe_out_time_ms = 1500;
int effect_breathe_breathe_in_time_ms = 2500;
int effect_breathe_next_breathe_in_ms = 6000;
int effect_breathe_relative_brightness = 90;
byte effect_breathe_phase = 0;
long effect_breathe_next_phase = 0;
long effect_breathe_this_phase = 0;
int led_brightness = LED_BRIGHTNESS;
CRGB led_standard_color = CRGB::White;
long next_frame = 0;
long show_ota_message = 0;
#define OTA_MESSAGE_TIME 5000

// other definitions
const char* ota_host = "OTAWordClock";
#define OTA_ACTIVE    1
#define OTA_PASSIVE   0
byte ota_mode = OTA_PASSIVE;
bool ota_in_progress = false;
bool ota_on_boot = false;
bool shouldSaveConfig = false;

typedef struct {
  char  led_standard_color[7]      = "ffffff";
  char  led_brightness[4]          = "255";
  char  led_max_power_milliamp[5]  = "1700";
  char  show_effect_breathe[2]     = "1";
  char  show_effect_colorchange[2] = "1";
  char  ota_on_boot[2]             = "1";
} MySettings;

MySettings settings;
// *******************  WLAN Funktionen

#define FPM_SLEEP_MAX_TIME 0xFFFFFFF

void handleReboot() {
  ESP.restart();
}

void wlan_off() {
  DPRINTLN(F("WLAN off"));
  wifi_station_disconnect();
  wifi_set_opmode(NULL_MODE);
  wifi_set_sleep_type(MODEM_SLEEP_T);
  wifi_fpm_open();
  wifi_fpm_do_sleep(FPM_SLEEP_MAX_TIME);
}

void wlan_connect() {
  WiFi.mode(WIFI_STA);
  WiFi.hostname (myHostname);
  WiFi.begin ( ssid, password );

  int i = 300; // *100 ms = 16s Wartezeit
  while ( (WiFi.status() != WL_CONNECTED) && (i > 0) ) {
    delay ( 100 );
    i--;
  }
  if (WiFi.status() != WL_CONNECTED) {
    handleReboot();
  }
  DPRINTLN("My IP: " + WiFi.localIP().toString());
}

void wlan_on() {
  DPRINTLN(F("WLAN on"));
  wifi_fpm_do_wakeup();
  wifi_fpm_close();
  wifi_set_opmode(STATION_MODE);
  wifi_station_connect();
  wlan_connect();
}


// *******************  NTP functions

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

time_t getNtpTime() {
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  DPRINTLN(F("Transmit NTP Request"));
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  DPRINT(ntpServerName);
  DPRINT(F(": "));
  DPRINTLN(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      DPRINTLN(F("Receive NTP Response"));
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  DPRINTLN(F("No NTP Response :-("));
  return 0; // return 0 if unable to get the time
}

int freeRam() {
  return ESP.getFreeHeap();
}

void debugRAM() {
  DPRINT(F("free RAM: "));
  DPRINTLN(freeRam());    
}

void readConfig(String filename) {
  //read configuration from FS json
  DPRINTLN(F("mounting FS..."));

  if (SPIFFS.begin()) {
    DPRINTLN(F("mounted file system"));
    if (SPIFFS.exists(filename)) {
      //file exists, reading and loading
      DPRINTLN(F("reading config file"));
      File configFile = SPIFFS.open(filename, "r");
      if (configFile) {
        DPRINTLN(F("opened config file"));
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
#ifdef DEBUG
        json.printTo(Serial);
#endif
        if (json.success()) {
          DPRINTLN(F("\nparsed json"));

            led_brightness = json["led_brightness"];
            led_max_power_milliamp = json["led_max_power_milliamp"];
            show_effect_breathe = json["show_effect_breathe"];
            show_effect_colorchange = json["show_effect_colorchange"];
            ota_on_boot = json["ota_on_boot"];

        } else {
          DPRINTLN(F("failed to load json config"));
        }
      }
    }
  } else {
    DPRINTLN(F("failed to mount FS, reformating it"));
    SPIFFS.format();
  }
}

void writeConfig(String filename) {
  DPRINTLN(F("saving config"));
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();

  json["led_brightness"] = led_brightness;
  json["led_max_power_milliamp"] = led_max_power_milliamp;
  json["show_effect_breathe"] = show_effect_breathe;
  json["show_effect_colorchange"] = show_effect_colorchange;
  json["ota_on_boot"] = ota_on_boot;

  File configFile = SPIFFS.open(filename, "w");
  if (!configFile) {
    DPRINTLN(F("failed to open config file for writing"));
  }

#ifdef DEBUG
  json.printTo(Serial);
#endif
  json.printTo(configFile);
  configFile.close();  
}

void useConfig() {
  FastLED.setBrightness(led_brightness);
  FastLED.setMaxPowerInVoltsAndMilliamps(LED_VOLTAGE,led_max_power_milliamp);
}

void setup() {
  // Open serial communications and wait for port to open
  #ifdef DEBUG
  Serial.begin(57600);
  #endif 
  DPRINTLN(F("wordclock by TheUnknownDad."));
  DPRINT(F("version "));
  DPRINTLN(SW_VERSION);
  DPRINT(F("compiled "));
  DPRINT(compiledate);
  DPRINT(F(" at "));
  DPRINTLN(compiletime);
  DPRINTLN(F("starting..."));

  // pin setup
  pinMode(BUTTON_LEFT, INPUT);
  pinMode(BUTTON_RIGHT, INPUT);

  // read config file
  readConfig("/config.json");
  useConfig();
  show_ota_message = millis() + OTA_MESSAGE_TIME;

  // led setup
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(led_brightness);
  FastLED.setDither(0);
  FastLED.setTemperature(UncorrectedTemperature);
  FastLED.setMaxPowerInVoltsAndMilliamps(LED_VOLTAGE,led_max_power_milliamp);
  
  // wifi connection
  wlan_on();

  // ntp sync
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(600);

}

// led functions
void clear_all() {
  for (int i=0; i<NUM_LEDS; i++) {
    leds[i] = CRGB::Black;
  }
}

void clear_pixel(int x) {
  if ((x>=0) && (x<NUM_LEDS)) {
    leds[x] = CRGB::Black;
  }
}

void draw_pixel(int x, CRGB col) {
  if ((x>=0) && (x<NUM_LEDS)) {
    leds[x] = col;
  }
}

void draw_pixel_add(int x, CRGB col) {
  if ((x>=0) && (x<NUM_LEDS)) {
    leds[x] += col;
  }
}

void draw_segment_leds(int segment) {
  if ((segment>=0) && (segment<LED_SEGMENTS)) {
    for (int x = 0; x < led_segment_length[segment]; x++) {
      draw_pixel_add(x + led_segment_pos[segment], activeColor);
    }
  }  
}

void draw_segment(int segment) {
  bitSet(led_segment_state, segment);
}

void draw_segments(unsigned long segments) {
  for (int i=0; i<LED_SEGMENTS; i++) {
    if (bitRead(segments, i)) {
      draw_segment_leds(i);
    }
  }
}

void digitalClockDisplay(time_t l) {
  // digital clock display of the time
  DPRINT(hour(l));
  DPRINT(":");
  DPRINT(minute(l));
  DPRINT(":");
  DPRINT(second(l));
  DPRINT(" ");
  DPRINT(day(l));
  DPRINT(".");
  DPRINT(month(l));
  DPRINT(".");
  DPRINT(year(l));
  DPRINTLN();
}

void drawWordClockDisplay(time_t l) {
  draw_segment(LED_SEGMENT_ES);
  draw_segment(LED_SEGMENT_IST);
  int m = minute(l);
  int h = (hour(l) + 11) % 12 + 1;
  if (m<5) {
    draw_segment(LED_SEGMENT_UHR);
  } else if (m<10) {
    draw_segment(LED_SEGMENT_FUENF);
    draw_segment(LED_SEGMENT_NACH);
  } else if (m<15) {
    draw_segment(LED_SEGMENT_ZEHN);
    draw_segment(LED_SEGMENT_NACH);
  } else if (m<20) {
    draw_segment(LED_SEGMENT_VIERTEL);
    draw_segment(LED_SEGMENT_NACH);
  } else if (m<25) {
    draw_segment(LED_SEGMENT_ZWANZIG);
    draw_segment(LED_SEGMENT_NACH);
  } else if (m<30) {
    draw_segment(LED_SEGMENT_FUENF);
    draw_segment(LED_SEGMENT_VOR);
    draw_segment(LED_SEGMENT_HALB);
    h = (h + 12) % 12 + 1;
  } else if (m<35) {
    draw_segment(LED_SEGMENT_HALB);
    h = (h + 12) % 12 + 1;
  } else if (m<40) {
    draw_segment(LED_SEGMENT_FUENF);
    draw_segment(LED_SEGMENT_NACH);
    draw_segment(LED_SEGMENT_HALB);
    h = (h + 12) % 12 + 1;
  } else if (m<45) {
    draw_segment(LED_SEGMENT_ZWANZIG);
    draw_segment(LED_SEGMENT_VOR);
    h = (h + 12) % 12 + 1;
  } else if (m<50) {
    draw_segment(LED_SEGMENT_VIERTEL);
    draw_segment(LED_SEGMENT_DREI);
    h = (h + 12) % 12 + 1;
  } else if (m<55) {
    draw_segment(LED_SEGMENT_ZEHN);
    draw_segment(LED_SEGMENT_VOR);
    h = (h + 12) % 12 + 1;
  } else {
    draw_segment(LED_SEGMENT_FUENF);
    draw_segment(LED_SEGMENT_VOR);
    h = (h + 12) % 12 + 1;
  }
  if (h==1) {
    if (m<5) {
      draw_segment(LED_SEGMENT_Z_EIN);
    } else {
      draw_segment(LED_SEGMENT_Z_EINS);
    }
  } else if (h==2) {
    draw_segment(LED_SEGMENT_Z_ZWEI);
  } else if (h==3) {
    draw_segment(LED_SEGMENT_Z_DREI);
  } else if (h==4) {
    draw_segment(LED_SEGMENT_Z_VIER);
  } else if (h==5) {
    draw_segment(LED_SEGMENT_Z_FUENF);
  } else if (h==6) {
    draw_segment(LED_SEGMENT_Z_SECHS);
  } else if (h==7) {
    draw_segment(LED_SEGMENT_Z_SIEBEN);
  } else if (h==8) {
    draw_segment(LED_SEGMENT_Z_ACHT);
  } else if (h==9) {
    draw_segment(LED_SEGMENT_Z_NEUN);
  } else if (h==10) {
    draw_segment(LED_SEGMENT_Z_ZEHN);
  } else if (h==11) {
    draw_segment(LED_SEGMENT_Z_ELF);
  } else if (h==12) {
    draw_segment(LED_SEGMENT_Z_ZWOELF);
  }
}

uint8_t gHue = 0; 

void showPinState(int p) {
  DPRINT(F("pin state for pin "));
  DPRINT(p);
  if ( digitalRead(p) == HIGH ) {
    DPRINTLN(F(": high"));
  } else {
    DPRINTLN(F(": low"));
  }
}

void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void functionSetupAP() {
  shouldSaveConfig = false;
  DPRINTLN(F("config mode started"));
  // pre-init phase - adjust variables to match active configuration
  sprintf(settings.led_brightness, "%3i", led_brightness);
  sprintf(settings.led_max_power_milliamp, "%4i", led_max_power_milliamp);
  if (show_effect_breathe) {
    sprintf(settings.show_effect_breathe, "%1i", 1);
  } else {
    sprintf(settings.show_effect_breathe, "%1i", 0);
  }
  if (show_effect_colorchange) {
    sprintf(settings.show_effect_colorchange, "%1i", 1);
  } else {
    sprintf(settings.show_effect_colorchange, "%1i", 0);
  }
  if (ota_on_boot) {
    sprintf(settings.ota_on_boot, "%1i", 1);
  } else {
    sprintf(settings.ota_on_boot, "%1i", 0);
  }

  WiFiManagerParameter custom_led_text("<h2><center>LED Setup</center></h2></br>");
  WiFiManagerParameter custom_led_brightness("led_brightness", "Helligkeit (0-255)", settings.led_brightness, 4);
  WiFiManagerParameter custom_led_max_power_milliamp("led_max_power_milliamp", "Netzteilst&auml;rke in mA (400-9999)", settings.led_max_power_milliamp, 5);
  WiFiManagerParameter custom_show_effect_breathe("show_effect_breathe", "Zeige Atmen-Effekt (0/1)", settings.show_effect_breathe, 2);
  WiFiManagerParameter custom_show_effect_colorchange("show_effect_colorchange", "tagesabh&auml;ngige Farben (0/1)", settings.show_effect_colorchange, 2);
  WiFiManagerParameter custom_ota_on_boot("ota_on_boot", "OTA aktiv nach Boot (0/1)", settings.ota_on_boot, 2);

  WiFiManager wifiManager;
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.addParameter(&custom_led_text);
  wifiManager.addParameter(&custom_led_brightness);
  wifiManager.addParameter(&custom_led_max_power_milliamp);
  wifiManager.addParameter(&custom_show_effect_breathe);
  wifiManager.addParameter(&custom_show_effect_colorchange);
  wifiManager.addParameter(&custom_ota_on_boot);

  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  wifiManager.setTimeout(300);

  //it starts an access point with the specified name
  //and goes into a blocking loop awaiting configuration
 
  if (!wifiManager.startConfigPortal("WordClockAP")) {
    DPRINTLN(F("failed to connect and hit timeout"));
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }
  // use new config values
  int val = atoi(custom_led_brightness.getValue());
  if (val>=0 && val<256) {
    DPRINTLN(F("setting led brightness"));
    led_brightness = val;
  }
  val = atoi(custom_led_max_power_milliamp.getValue());
  if (val>=400 && val<10000) {
    DPRINTLN(F("setting led max power"));
    led_max_power_milliamp = val;
  }
  val = atoi(custom_show_effect_breathe.getValue());
  if (val==0 || val==1) {
    DPRINTLN(F("setting effect breathe"));    
    show_effect_breathe = (val==1);
  }
  val = atoi(custom_show_effect_colorchange.getValue());
  if (val==0 || val==1) {
    DPRINTLN(F("setting effect colorchange on time"));    
    show_effect_colorchange = (val==1);
  }
  val = atoi(custom_ota_on_boot.getValue());
  if (val==0 || val==1) {
    DPRINTLN(F("setting ota on boot"));    
    ota_on_boot = (val==1);
  }

  useConfig();
  //if you get here you have connected to the WiFi
  DPRINTLN(F("connected :)"));
  if (shouldSaveConfig) {
    writeConfig("/config.json");
  }
}

void functionToggleOTA() {
  show_ota_message = millis() + OTA_MESSAGE_TIME;
  if (ota_mode == OTA_PASSIVE) {
    DPRINTLN(F("activating ota updates."));
    ota_mode = OTA_ACTIVE;
    ota_on_boot = true;
    writeConfig("/config.json");
    ArduinoOTA.setHostname(ota_host);
    ArduinoOTA.onStart([]() { // starting upgrade
      clear_all();
      activeColor = CRGB::Green;
      draw_segment_leds(LED_SEGMENT_WIFI);
      FastLED.setBrightness(led_brightness);
      FastLED.show();
      ota_in_progress = true;
      delay(200);
    });
    ArduinoOTA.onEnd([]() { // ending upgrade
      for (int a = 0; a<5; a++) { 
        activeColor = CRGB::Black;
        draw_segment_leds(LED_SEGMENT_WIFI);
        FastLED.show();
        delay(500);
        activeColor = CRGB::Green;
        draw_segment_leds(LED_SEGMENT_WIFI);
        FastLED.show();
        delay(500);
      }
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      draw_pixel(55+(progress / (total / 10)), CRGB::Blue);
      draw_pixel(76-(progress / (total / 10)), CRGB::Blue);
      FastLED.show();
    });    
    ArduinoOTA.onError([](ota_error_t error) { 
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
      for (int a = 0; a<5; a++) { 
        activeColor = CRGB::Black;
        draw_segment_leds(LED_SEGMENT_WIFI);
        FastLED.show();
        delay(500);
        activeColor = CRGB::Red;
        draw_segment_leds(LED_SEGMENT_WIFI);
        FastLED.show();
        delay(500);
      }
//      handleReboot();
//      ota_mode = OTA_PASSIVE;
      ota_in_progress = false;
    });
    ArduinoOTA.begin();     
  } else {
    DPRINTLN(F("deactivating ota updates."));
    ota_mode = OTA_PASSIVE;    
    ota_on_boot = false;
    writeConfig("/config.json");
  }
}

void startFunction(byte func) {
  if (func == FN_SETUP_AP) {
    clear_all();
    activeColor = CRGB::Red;
    draw_segment_leds(LED_SEGMENT_WIFI);
    FastLED.setBrightness(led_brightness);
    FastLED.show();
    functionSetupAP();
  } else if (func == FN_TOGGLE_OTA) {
    functionToggleOTA();
  } else {
    DPRINTLN(F("no function defined."));
  }
}

void loop() {
  // button handling
  if (digitalRead(BUTTON_LEFT) == HIGH) {
    if (button_left_last_state == STATE_UNPRESSED) {
      button_left_last_state = STATE_PRESSED;
      startFunction(button_left_function);
    }
  } else {
    button_left_last_state = STATE_UNPRESSED;
  }
  if (digitalRead(BUTTON_RIGHT) == HIGH ) {
    if (button_right_last_state == STATE_UNPRESSED) {
      button_right_last_state = STATE_PRESSED;
      startFunction(button_right_function);
    }
  } else {
    button_right_last_state = STATE_UNPRESSED;
  }

  clear_all();
  effect_time = millis();

  // network handling
  if (ota_mode == OTA_ACTIVE) {
    ArduinoOTA.handle();
  } else {
    if (ota_on_boot) {
      functionToggleOTA();
    }
  }
  if (show_ota_message>effect_time) {
    activeColor = CRGB::Yellow;
    draw_segment_leds(LED_SEGMENT_WIFI);
    if (ota_mode == OTA_ACTIVE) {
      draw_segment_leds(LED_SEGMENT_AN);
    } else {
      draw_segment_leds(LED_SEGMENT_AUS);      
    }
  }


  // effect handling
  gHue++;
  if (effect_time>next_frame) {
    if (! ota_in_progress) {
      if (show_effect_breathe) {
        if (effect_time > effect_breathe_next_phase) {
          effect_breathe_phase = (effect_breathe_phase + 1) % 3;
          effect_breathe_this_phase = effect_time;
          if (effect_breathe_phase == 1) {
            effect_breathe_next_phase = effect_time + effect_breathe_next_breathe_in_ms;
          } else if (effect_breathe_phase == 2) {
            effect_breathe_next_phase = effect_time + effect_breathe_breathe_in_time_ms;
          } else if (effect_breathe_phase == 0) {
            effect_breathe_next_phase = effect_time + effect_breathe_breathe_out_time_ms;
          }
        }
        if (effect_breathe_phase == 0) {
          led_brightness = LED_BRIGHTNESS;
        } else if (effect_breathe_phase == 2) {
          led_brightness = LED_BRIGHTNESS * (effect_time-effect_breathe_this_phase)/(effect_breathe_next_phase - effect_breathe_this_phase) + effect_breathe_relative_brightness * (effect_breathe_next_phase - effect_time)/(effect_breathe_next_phase - effect_breathe_this_phase);
        } else if (effect_breathe_phase == 1) {
          led_brightness = effect_breathe_relative_brightness * (effect_time-effect_breathe_this_phase)/(effect_breathe_next_phase - effect_breathe_this_phase) + LED_BRIGHTNESS * (effect_breathe_next_phase - effect_time)/(effect_breathe_next_phase - effect_breathe_this_phase);
        }
    
        FastLED.setBrightness(led_brightness);
      }
      activeColor = led_standard_color;
      local = CE.toLocal(now(), &tcr);
      if (show_effect_colorchange) {
        if (hour(local)<6) {
          activeColor = CRGB::Red;
        } else if (hour(local)>17) {
          activeColor = CRGB::Blue;
        } else if (hour(local)<12) {
          activeColor = CRGB::Green;
        }
      }
      if (show_clock) {
        led_segment_last_state = led_segment_state;
        led_segment_state = 0;
        drawWordClockDisplay(local);
        if ((led_segment_last_state != led_segment_state) && show_effect_smooth_fade) {
          // smooth fade
          if (effect_smooth_fade_start == 0) {
            effect_smooth_fade_start = effect_time;
            effect_smooth_fade_fader = 0;
          } else if (effect_time > effect_smooth_fade_start + effect_smooth_fade_time_ms) {
            effect_smooth_fade_start = 0;
            effect_smooth_fade_fader = 255;
          } else {
            effect_smooth_fade_fader = (effect_time - effect_smooth_fade_start) * 256 / effect_smooth_fade_time_ms;
          }
          fadeColor = activeColor;
          activeColor = lastColor.fadeToBlackBy(effect_smooth_fade_fader);
          draw_segments(led_segment_last_state);
          activeColor = fadeColor.fadeToBlackBy(255-effect_smooth_fade_fader);
          draw_segments(led_segment_state);
          if (effect_smooth_fade_fader < 255) {
            led_segment_state = led_segment_last_state;
          }
        } else {
          draw_segments(led_segment_state);
          lastColor = activeColor;
        }
      }
      
      // display loop
      FastLED.show();
      next_frame = millis() + 1000/FRAMES_PER_SECOND;
    } else {
      yield();
    }
  } else {
    yield();
  }
  // finally some occasional debug output
  if (gHue==1) { 
    digitalClockDisplay(local);
//    showPinState(D5);
//    showPinState(D7);
    debugRAM();
  }
}

