
/*
   Binary clock by Ruud van Vliet ruud@vliet.io
   Wemos D1 mini version
*/

#include <Timezone.h>

#include <SPI.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <BH1750.h>

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

#include <Ticker.h>
Ticker ticker;

//#include "arduino_secrets.h"

// NTP Servers:
static const char ntpServerName[] = "us.pool.ntp.org";
IPAddress timeServer(17, 253, 34, 125); // time.nist.gov NTP server


// Timezone configuration
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     // Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};       // Central European Standard Time
Timezone CE(CEST, CET);
TimeChangeRule *tcr;        // pointer to the time change rule, use to get TZ abbrev

BH1750 lightMeter(0x23);

// NeoPixel configuration

#define LED_PIN    D6
#define LED_COUNT 64
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
boolean alert = false;
int brightness = 5;

struct int6 {
  int i[6];
};
struct char6 {
  char c[6];
};
struct t {
  int h = 10;
  int m = 0;
};

int lAlert[] = {18};
//int l1[] = {19};
//int l2[] = {26, 27};
//int l4[] = {16, 17, 24, 25};
//int l8[] = {0, 1, 2, 3, 8, 9, 10, 11};
//int l16[] = {4, 5, 6, 7, 12, 13, 14, 15, 20, 21, 22, 23, 28, 29, 30, 31};
// gespiegeld voor Robbert:
int l1[] = {20};
int l2[] = {28, 29};
int l4[] = {22, 23, 30, 31};
int l8[] = {4, 5, 6, 7, 12, 13, 14, 15};
int l16[] = {0, 1, 2, 3, 8, 9, 10, 11, 16, 17, 18, 19, 24, 25, 26, 27};
// identiek
int l32[] = {32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63};

int smiley[] = { 18, 21, 26, 29, 33, 38, 41, 46, 50, 53, 59, 60}; //12
int frowney[] = {9, 10, 13, 14, 18, 21, 35, 36, 42, 45, 49, 54, 57, 62}; //14
int key[] = {9, 16, 18, 24, 26, 27, 28, 29, 30, 31, 32, 34, 38, 39, 41, 46, 47}; //17
int questionMark[] = {3, 4, 10, 13, 21, 28, 35, 43, 59}; //9


WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets

time_t getNtpTime();
void digitalClockDisplay();
void printDigits(int digits);
void sendNTPpacket(IPAddress &address);

void setup()
{
  Serial.begin(9600);

  Wire.begin();

  strip.begin();
  strip.setBrightness(brightness); //  (max = 255)
  setL(questionMark, 9, 'W');
  strip.show();


  //set led pin as output
  pinMode(BUILTIN_LED, OUTPUT);
  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing
//  wifiManager.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("Ruuds binary clock")) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }

  //if you get here you have connected to the WiFi
  strip.clear();
  setL(smiley, 12, 'W');
  strip.show();
  Serial.println("connected...yeey :)");
  ticker.detach();
  //keep LED on
  digitalWrite(BUILTIN_LED, HIGH);


  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println(F("BH1750 Advanced begin"));
  }
  else {
    Serial.println(F("Error initialising BH1750"));
  }

  Serial.print("IP number ");
  Serial.println(WiFi.localIP());
  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  //  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(60);
}

time_t prevDisplay = 0; // when the digital clock was displayed

void loop() {

  uint16_t lux = lightMeter.readLightLevel();

  if (lux > 2000) {
    brightness = 201;
  } else {
    brightness = (lux / 10) + 1;
  }

  strip.setBrightness(brightness); //  (max = 255)
  strip.show();
  //  logLuxBrightness(lux, brightness);

  delay(1000);

  if (timeStatus() != timeNotSet) {
    if ( now() != prevDisplay && (second() == 0 || now() - prevDisplay > 60 )) { //update the display every minute
      prevDisplay = now();
      digitalClockDisplay();
      logLuxBrightness(lux, brightness);
    }
  } else {
    Serial.println("Time not yet set ");
    delay(5000);
    getNtpTime();
  }
}

void tick()
{
  //toggle state
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  strip.clear();
  setL(key, 17, 'W');
  strip.show();

  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}


void digitalClockDisplay() {
  displayTime(hour(), minute());
  // digital clock display of the time
  Serial.print(year());
  Serial.print("-");
  Serial.print(month());
  Serial.print("-");
  Serial.print(day());
  Serial.print("T");
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" (");
  Serial.print(now());
  Serial.print(")");
  Serial.println();
}

void printDigits(int digits)
{
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime() {
//  if (WiFi.status() != WL_CONNECTED) {
//    Serial.print("Connection lost, reconnecting to SSID: ");
//    Serial.println(ssid);
//    WiFi.begin(ssid, pass);
//    if (WiFi.status() != WL_CONNECTED) {
//      Serial.println("Still no connection, skipping for now.");
//      setSyncInterval(10);
//      alert = true;
//      setL(lAlert, 1, 'W');
//      strip.show();
//      return 0;
//    } else {
//      Serial.print("Connection regained, connected to SSID: ");
//      Serial.println(ssid);
//      setSyncInterval(60);
//      alert = false;
//      setL(lAlert, 1, 'X');
//      strip.show();
//    }
//  }

  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  //  sendNTPpacket(ntpServerIP);
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 2500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return CE.toLocal(secsSince1900 - 2208988800UL, &tcr);
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
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

// dsiplaying binary time to the 8x8 ledpanel
void displayTime(int h, int m) {
  int6 hours = bin(h);
  int6 minutes = bin(m);
  char6 color2display;
  for (unsigned i = 0; i < 6; i++) {
    color2display.c[i] = 'X';
    if (hours.i[i])  color2display.c[i] = 'R';
    if (minutes.i[i])  color2display.c[i] = 'B';
    if (hours.i[i] && minutes.i[i])  color2display.c[i] = 'Y';
  }

  if (alert) {
    setL(lAlert, 1, 'W');
  } else {
    setL(lAlert, 1, 'X');
  }
  setL(l1, 1, color2display.c[5]);
  setL(l2, 2, color2display.c[4]);
  setL(l4, 4, color2display.c[3]);
  setL(l8, 8, color2display.c[2]);
  setL(l16, 16, color2display.c[1]);
  setL(l32, 32, color2display.c[0]);
  strip.show();
}

void setL(int ls[], int len, char colour) {
  for (unsigned int i = 0; i < len; i++) {
    switch (colour) {
      case 'W': strip.setPixelColor(ls[i], strip.Color(255, 255, 255));
        break;
      case 'R': strip.setPixelColor(ls[i], strip.Color(255,   0,   0));
        break;
      case 'B': strip.setPixelColor(ls[i], strip.Color(0,   0,   255));
        break;
      case 'M': strip.setPixelColor(ls[i], strip.Color(255,   0,   255));
        break;
      case 'G': strip.setPixelColor(ls[i], strip.Color(0,   255,   0));
        break;
      case 'Y': strip.setPixelColor(ls[i], strip.Color(255,   255,   0));
        break;
      default: strip.setPixelColor(ls[i], strip.Color(0,   0,   0));
        break;
    }
  };
};

int6 bin(unsigned n) {
  int6 a;
  unsigned i;
  unsigned j = 0;
  for (i = 1 << 5; i > 0; i = i / 2) {
    (n & i) ? a.i[j] = 1 : a.i[j] = 0;
    j++;
  };
  return a;
}

void printDateTime(time_t t, const char *tz)
{
  char buf[32];
  char m[4];    // temporary storage for month string (DateStrings.cpp uses shared buffer)
  strcpy(m, monthShortStr(month(t)));
  sprintf(buf, "%.2d:%.2d:%.2d %s %.2d %s %d %s",
          hour(t), minute(t), second(t), dayShortStr(weekday(t)), day(t), m, year(t), tz);
  Serial.println(buf);
}

void logLuxBrightness(uint16_t lux, int brightness) {
  Serial.print("Light: ");
  Serial.print(lux);
  Serial.println(" lx");

  Serial.print("Brightness: ");
  Serial.println(brightness);

}
