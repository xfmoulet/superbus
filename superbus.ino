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

// STAR search parameters 
#define IDLIGNE "0002"
#define SENSLIGNE "0"
#define ARRETLIGNE "1043"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#include "secret.h" // SSID, APPWD, APIKEY
#include "gfx.c" // include directly as C 

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
  
const int DEBUG=1;

ESP8266WiFiMulti WiFiMulti;


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  USE_SERIAL.begin(9600);
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
  delay(100);
  // init display

}

int time2min(const char *timestr)
{
  // 2018-09-22T01:13:00+02:00
  // no need to keep constants such as '0' or timezones since we're doing a diff
  return 60*(timestr[11]*10+timestr[12])+10*timestr[14]+timestr[15];
}

void blink_leds(int n)
{
  // blink LED N times
  for (int i=0;i<n;i++) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(250);
      digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on 
      delay(100);
  }
  digitalWrite(LED_BUILTIN, HIGH); // Off
}

void disp_intro(void) {
    USE_SERIAL.println("pre Xbitmap");

    for (int i=-20;i<0;i+=2) {
      display.clearDisplay();     // no logo !
      display.drawXBitmap(i, 0,superbus_bits, superbus_width, superbus_height,WHITE);
      display.display();
      delay(100);
    }
    USE_SERIAL.println("post Xbitmap");
    delay(500);

    display.clearDisplay();
    display.setTextSize(1);             // Normal 1:1 pixel scale
    display.setTextColor(WHITE);        // Draw white text
    display.setCursor(0,0);             // Start at top-left corner
}



void loop() {
  disp_intro();
  
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
      "&refine.idligne=" IDLIGNE "&refine.sens=" SENSLIGNE "&refine.idarret=" ARRETLIGNE // C2 saint martin dir haut sancé 
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

        //  header : logo, heure, page
        display.println(F("C2 Haut sance [HH:MM]"));
        display.println(F(" ---"));
        
        int rec_id;
        for (rec_id=0;rec_id<json["records"].size();rec_id++)
        {
        
          JsonObject& rec0 = json["records"][rec_id]; // heure, diffmin
          
          // heures premiere arrivée
          const char* depart    = rec0["fields"]["depart"]; // "2018-09-22T01:13:00+02:00"
          const char* timestamp = rec0["record_timestamp"]; // "2018-09-21T23:59:00+02:00"
          int mins = time2min(depart)-time2min(timestamp);
          
          display.print("heure:  ");    display.println(timestamp+11);
          display.print("depart: ");    display.println(depart+11);
          display.print("diffmin:");    display.println(mins);
        }
        
        if (!rec_id) {
          display.println("\nPas de bus\npour l'instant :(");
        }
        
        display.display();


        delay(4000); // let time to show results
      }
    } else {
      USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
    USE_SERIAL.println("Dodo ! Please reset now.");
    display.ssd1306_command(SSD1306_DISPLAYOFF); // display Off
    ESP.deepSleep(0);
  }
  delay(100); 
  
}
