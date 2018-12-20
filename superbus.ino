#define DEBUG_ESP_HTTP_CLIENT

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#include "secret.h" // SSID, APPWD, APIKEY

#define USE_SERIAL Serial

// Declaration for SSD1306 display connected using software SPI (default case):
#define OLED_MOSI   13
#define OLED_CLK   14
#define OLED_DC    4
#define OLED_CS    15
#define OLED_RESET 5


Adafruit_SSD1306 display(
  SCREEN_WIDTH, SCREEN_HEIGHT,
  OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS
  );
  
const int DEBUG=0;

ESP8266WiFiMulti WiFiMulti;


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  USE_SERIAL.begin(115200);
  // USE_SERIAL.setDebugOutput(true);

  USE_SERIAL.println();
  USE_SERIAL.println();
  USE_SERIAL.println();

  USE_SERIAL.printf("[SETUP] WAIT ...\n");
  USE_SERIAL.flush();
  delay(500);

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(SSID, APPWD);


  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // init display
  display.clearDisplay();
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner

}

int time2min(const char *timestr)
{
  // 2018-09-22T01:13:00+02:00
  // no need to keep constants such as '0' or timezones
  return 60*(timestr[11]*10+timestr[12])+10*timestr  [14]+timestr  [15];
  
}


void loop() {
  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) {

    HTTPClient http;

    USE_SERIAL.print("[HTTP] begin...\n");
    // configure traged server and url
    //http.begin("https://192.168.1.12/test.html", "7a 9c f4 db 40 d3 62 5a 6e 21 bc 5c cc 66 c8 3e a1 45 59 38"); //HTTPS
    //http.begin("http://example.com/"); //HTTP
    http.begin(
      "http://data.explore.star.fr/api/records/1.0/search/"
      "?dataset=tco-bus-circulation-passages-tr&rows=3&sort=-depart&facet=precision"
      "&refine.idligne=0002&refine.sens=0&refine.idarret=1043" // C2 saint martin dir haut sancé 
      "&timezone=Europe%2FParis&apikey=" APIKEY
      );
    USE_SERIAL.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);
      
      DynamicJsonBuffer jsonBuffer(5000);
      
      if (httpCode == HTTP_CODE_OK ) {
        String payload = http.getString();
        if (DEBUG) USE_SERIAL.println(payload);
        JsonObject& json = jsonBuffer.parseObject(payload);
        if (DEBUG) USE_SERIAL.println(json["nhits"].as<int>());
        USE_SERIAL.println("found records: "); USE_SERIAL.println(json["records"].size());

        // TODO : check minimum positive / 2 minimums, variable number
        int next=999999; // minutes til the next one. must be positive.
        for (int rec_id=0;rec_id<json["records"].size();rec_id++)
        {
        
          JsonObject& rec0 = json["records"][rec_id];
          
          // heures premiere arrivée
          const char* depart    = rec0["fields"]["depart"]; // "2018-09-22T01:13:00+02:00"
          const char* timestamp = rec0["record_timestamp"]; // "2018-09-21T23:59:00+02:00"
          int mins = time2min(depart)-time2min(timestamp);
          if (mins>0 && mins < next)
            next = mins;
          
          USE_SERIAL.print("heure:  ");    USE_SERIAL.println(timestamp);
          USE_SERIAL.print("depart: ");    USE_SERIAL.println(depart);
          USE_SERIAL.print("diffmin:");    USE_SERIAL.println(mins);
          USE_SERIAL.print("next:   ");    USE_SERIAL.println(next);
        }

        // blink LED N times
        for (int i=0;i<next;i++) {
            digitalWrite(LED_BUILTIN, HIGH);
            delay(250);
            digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on 
            delay(100);
        }
        digitalWrite(LED_BUILTIN, HIGH); // Off

        display.println(F("Prochain bus dans"));
        display.println(next);
        display.println(F("minutes !"));
        display.display();

        delay(2000); // 
      }
    } else {
      USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
    USE_SERIAL.println("Dodo ! Please reset now.");
    ESP.deepSleep(0);
  }
  delay(100); // wait cnx ...
}
