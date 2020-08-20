/* Arduino 'slimme meter' P1-port reader.
 
 This sketch reads data from a Dutch smart meter that is equipped with a P1-port.
 
 Baudrate 115200, 8N1.
 Transistor & 12k resistor is needed to make data readable (meter spits out inverted data)

 Arduino settings:
 board      : Generic ESP8266 module
 Baudrate   : 115200
 Flash mode : DOUT
 Reset mode : dtr (aka modemcu)
 Buikdin led: 0
 
 A .php file is requested (with consumption numbers in the GET request) every minute (interval set with DSMR_UPDATE)
 created by 'ThinkPad' @ Tweakers.net, september 2014
 
 http://gathering.tweakers.net/forum/list_messages/1601301
*/

//                                 +--------------------+
//                                 |  ()  ==========    |
//                                 |        LED GPIO2 * |
//                           Reset =1                 16= GPIO1  - TxD - SpiCS1
//                     ADC0 - Tout =2 +-------------+ 15= GPIO3  - RxD
//                  Enable - CH_PD =3 |   ESP8266   | 14= GPIO5  - SCL
//            USER - Wake - GPIO16 =4 |             | 13= GPIO4  - SDA
//            SCK/HspiClk - GPIO14 =5 |   ESP-12E   | 12= GPIO0  - Flash - SpiCS2
//             MISO/HspiQ - GPIO12 =6 |             | 11= GPIO2  - Tx1
// Rx0'- CTS - MOSI/HspiD - GPIO13 =7 |             | 10= GPI015 - RTS - Tx0' - HspiCS/SS
//                             VCC =8 +-------------+  9= GND
//                                 |      18  20  22    |
//                                 |    17| 19| 21|     |
//                                 +----|-|-|-|-|-|-----+

#include <FS.h>                                       //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>                              // https://github.com/esp8266/Arduino
//needed for library
#include <ESP8266WebServer.h>
#include <DNSServer.h>

#include <ArduinoOTA.h>
#include <WiFiManager.h>                              // https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>                              // https://github.com/bblanchon/ArduinoJson
#include <U8g2lib.h>                                  // oliver
#include <SPI.h>
#include <dsmr.h>                                     // Matthijs Kooijman

// flag for saving data
bool shouldSaveConfig = false;

// udp: local settings
unsigned int dsmr_port = 50505;                       // local port to listen on

// udp: remote connection settings
char debug_ip[16] = "";
unsigned int debug_port = 60606;

// server to send results to
char dsmr_ip[16] = "";

char oledTxt[25] = "";
int status = WL_IDLE_STATUS;                          // the Wifi radio's status
unsigned long last;                                   // used by dsmr reader

// I/O pin usage
#define TRIGGER_PIN   0                               // wifi init trigger pin
#define DSMR_REQPIN   16                              // dsmr data request I/O pin
#define DSMR_UPDATE   30000                           // dsmr update frequency in ms

WiFiUDP Udp;

// pin remapping with ESP8266 HW I2C
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, SCL, SDA);

#include "sm_functions.h"

P1Reader reader(&Serial, DSMR_REQPIN);

void setup(void) {
  Serial.begin(115200);
  Serial.println("\n Starting\nSmartmeter reader V2.0 20200817\n");

  Serial.println("start: init display");
  u8g2.begin();                                       // for oled display
  Serial.println("done: init display");
  
  pinMode(TRIGGER_PIN, INPUT);                        // manual wifi setup trigger

  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  digitalWrite(4, 0);
  digitalWrite(5, 0);

  // test trigger pin
//  pinMode(DSMR_REQPIN, OUTPUT);                     // set the digital pin as output
    
  Serial.println("start: OLED connect");
  OLED_connect();
  delay(5000);
  Serial.println("done: OLED connect");

  // dsmr: start a read right away
  reader.enable(true);
  last = millis();
}

void loop(void) {
  // check for Wifi trigger
  if (digitalRead(TRIGGER_PIN) == LOW ) {
    Serial.println("Wifi trigger pin low");

    //WiFiManager
    WiFiManagerParameter custom_dsmr_server("server", "dsmr server", dsmr_ip, 16);

    strcpy(dsmr_ip, debug_ip);

    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    wifiManager.setTimeout(120);                            // sets timeout until configuration portal gets turned off
    wifiManager.addParameter(&custom_dsmr_server);          // id/name, placeholder/prompt, default, length

    if (!wifiManager.startConfigPortal("DSMR-AP")) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.reset();
      delay(5000);
    }
    strcpy(dsmr_ip, custom_dsmr_server.getValue());
    Serial.print("DSMR IP Address: ");
    Serial.println(dsmr_ip);
    Serial.print("Degug IP Address: ");
    Serial.println(debug_ip);
    Serial.println("connected...)");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    OLED_connect();
    ArduinoOTA.handle();
    delay(5000);
  }  

  // Allow the reader to check the serial buffer regularly
  reader.loop();

  // Every minute, fire off a one-off reading
  unsigned long now = millis();
  if (now - last > DSMR_UPDATE) {
    reader.enable(true);
    last = now;
    // hartbeat
    Serial.print(".");
    Serial.print("DSMR IP Address: ");
    Serial.println(dsmr_ip);
  }

  if (reader.available()) {
    MyData data;
    String err;
    if (reader.parse(&data, &err)) {
      // Parse succesful, print result
      data.applyEach(Printer());
      Udp.beginPacket(dsmr_ip, dsmr_port);
      data.applyEach(UDPprinter());
      Udp.endPacket();
    } else {
      // Parser error, print error
      Serial.println(err);
      Serial.println("parse error");
    }
    // show dsmr results on OLED display
    OLED_dsmr(data);
  }
}
