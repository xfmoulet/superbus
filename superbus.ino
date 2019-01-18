#define DEBUG_ESP_HTTP_CLIENT

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>  // librairie pour le wifi basic
#include <ESP8266WiFiMulti.h> // librairie pour permettre plusieurs SSID
#include <ESP8266HTTPClient.h>

#include <NTPClient.h> // pour recuperation de l'heure
#include <WiFiUdp.h>   //

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "secret.h" // afin de ne pas avoir dans le code mes infos perso wifi et compte Star SSID, APPWD, APIKEY. A mettre dans le meme repertoire que le code
#include "BusStar.h" // Liste des bus que je veux recuperer
#include "gfx.c" // include directly as C

#define USE_SERIAL Serial

// Declaration des pins du SSD1306 connectée en SPI:
#define OLED_MOSI   13
#define OLED_CLK   14
#define OLED_DC    4
#define OLED_CS    15
#define OLED_RESET 5

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// initialisation de l'afficheur
Adafruit_SSD1306 display(
  SCREEN_WIDTH, SCREEN_HEIGHT,
  OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS
  );
 
const int DEBUG=0;
int rec_id;
int mins;
//void setRotation(uint8_t rotation);

ESP8266WiFiMulti WiFiMulti;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

void connect_wifi()    // Connexion au wifi
{
  WiFi.mode(WIFI_STA); // Se met en mode client wifi et non Access Point

      WiFiMulti.addAP(SSIDH,APPWDH); // wifi à la maison
      //WiFiMulti.addAP(SSIDM,APPWDM); // wifi sur mon tel
  
      USE_SERIAL.println("Connection ...");
      display.print("Connexion wifi en cours ..");
      display.display();

}

void wait_for_wifi()
{
        int i = 0;
        int y = -30;
        int z = -20;
           while (WiFi.status() != WL_CONNECTED) { // En attente de connection
              z = z + 10;
              y = y + 10;
              USE_SERIAL.print(++i); USE_SERIAL.print('.');
              disp_intro(y,z);
       //     delay(500); Le delai d'attente est remplacé par l'affichage du bus

            }

      USE_SERIAL.print("Connected to "); USE_SERIAL.println(WiFi.SSID());
      USE_SERIAL.print("IP address:\t"); USE_SERIAL.println(WiFi.localIP()); // utile pour recuperer l'IP et la rendre fixe sur notre Livebox
}

void init_display()
{
  //   display.clearDisplay();
  //   display.display();

    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
      if(!display.begin(SSD1306_SWITCHCAPVCC)) {
        USE_SERIAL.println(F("SSD1306 allocation failed"));
        for(;;); // Don't proceed, loop forever
      }
}

//**********************************

void disp_intro(int j, int k) {     // Dessine le petit bus
    USE_SERIAL.println("pre Xbitmap");

    for (int i=j;i<k;i+=2) {
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

#define BUFSIZE 1000
char buffer[BUFSIZE];

void get_bus(char *title, int rows,char *idligne,char *idarret ) {

     display.setCursor(0,0);            
     display.setTextSize(1);            
     display.println(title);
     USE_SERIAL.println(idligne);
     HTTPClient http;
                 
     USE_SERIAL.println("[HTTP] begin...\n");
                             
     timeClient.update();
      
     // definir l'url

     snprintf(buffer,BUFSIZE, "http://data.explore.star.fr/api/records/1.0/search/"
        "?dataset=tco-bus-circulation-passages-tr&rows=%d"
        "&sort=-depart&facet=precision&refine.idligne=%s"
        "&refine.idarret=%s"
        "&timezone=Europe%%2FParis&apikey=%s",rows,idligne,idarret,APIKEY );

     http.begin(buffer);
       

    USE_SERIAL.println("[HTTP] GET...\n");
                
    // lancer la requete
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
                             
                 USE_SERIAL.print("found records: "); USE_SERIAL.println(json["records"].size());
                           
                 http.end(); // libere les ressources
                           
                 int rec_id;
                      for ( rec_id=0;rec_id<json["records"].size();rec_id++)
                          {
                           JsonObject& rec0 = json["records"][rec_id];
                                       
                           // munites resstantes avant bus
                           const char* depart    = rec0["fields"]["depart"]; // "2018-09-22T01:13:00+02:00"
                           int dep = ((((depart[11]-48)*10) + (depart[12]-48))*60) + ((depart[14]-48)*10) + (depart[15]-48); // -48 pour passage code ascii en chiffre
                           int tstamp = (timeClient.getHours()*60)+ timeClient.getMinutes();
                           int mins = dep - tstamp;
                             
                           // Affichage des infos sur afficheur
                           
                           if (mins <= 0 )
                                 mins = 0;
                           USE_SERIAL.println(mins);
                           display.print("     "); display.print(mins); display.println("  minutes");
                           display.display();
                           }
                       if (!rec_id) {
                           display.println("\nPas de bus\npour l'instant");
                           display.display();
                           }
                            delay(500); //
                           }
        }
}

void diplay_type (int type) {
         USE_SERIAL.println(type);

        if (type == 7 ) {
            for (int x = 0; x<5; x++) {
              timeClient.update();
              display.clearDisplay();
              display.setCursor(10,25);
              display.setTextSize(2);
              // display.print(" il est ");
              display.println(timeClient.getFormattedTime());
              display.display();
              delay (1000);
            }
        } else if (type == 8 ) {
            for (int x = 0; x<5; x++) {
              timeClient.update();
              display.clearDisplay();
              display.setCursor(10,25);
              display.setTextSize(2);
              // display.print(" il est ");
              display.println(timeClient.getFormattedTime());
              display.setCursor(0,45);
              display.setTextSize(1);
              display.print(F("Appuyer sur le bouton "));
              display.print(" pour rafraichir");
              display.display();
              display.display();
              delay (1000);
            }
        }


/*  swtich (type) {
    case 1:
       USE_SERIAL.println("1");
       break;
    case 2:
       USE_SERIAL.println("2");
       break;      
    case 3:
       USE_SERIAL.println("3");
       break;
    case 4:
       USE_SERIAL.println("4");
       break;
    case 5:
       USE_SERIAL.println("5");
       break;
    case 6:
       USE_SERIAL.println("6");
       break;
    case 7:
       USE_SERIAL.println("7");
       break;
  } */
}

void get_bus2() {
     display.setCursor(0,35);            
     display.println(F("le 31"));
    
     HTTPClient http;
                   
     USE_SERIAL.println("[HTTP] begin...\n");
                             
     timeClient.update();
                 
     USE_SERIAL.println(timeClient.getFormattedTime());
           
     // definir l'url
     http.begin(
         "http://data.explore.star.fr/api/records/1.0/search/"
         "?dataset=tco-bus-circulation-passages-tr&rows=2"
         "&sort=-depart&facet=precision&refine.idligne=" IDLIGNE2
         "&refine.idarret=" IDARRET2
         "&timezone=Europe%2FParis&apikey=" APIKEY
               );
                 
     USE_SERIAL.println("[HTTP] GET...\n");
                
    // lancer la requete
    int httpCode = http.GET();
          
    // httpCode will be negative on error
      if (httpCode > 0) {
           // HTTP header has been send and Server response header has been handled
           USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);
           USE_SERIAL.println();
                   
           DynamicJsonBuffer jsonBuffer(5000);
                 
              if (httpCode == HTTP_CODE_OK ) {
                 String payload = http.getString();
                     if (DEBUG) USE_SERIAL.println(payload);
                             
                 JsonObject& json = jsonBuffer.parseObject(payload);
                     if (DEBUG) USE_SERIAL.println(json["nhits"].as<int>());
                             
              USE_SERIAL.print("found records: "); USE_SERIAL.println(json["records"].size());
                            
              http.end(); // libere les ressources
                             
              int rec_id;
              int Ligne = 45;
                  for ( rec_id=0;rec_id<json["records"].size();rec_id++) {
                      JsonObject& rec0 = json["records"][rec_id];
                                   
                      // munites resstantes avant bus
                      const char* depart    = rec0["fields"]["depart"]; // "2018-09-22T01:13:00+02:00"
                      int dep = ((((depart[11]-48)*10) + (depart[12]-48))*60) + ((depart[14]-48)*10) + (depart[15]-48); // -48 pour passage code ascii en chiffre
                      int tstamp = (timeClient.getHours()*60)+ timeClient.getMinutes();
                      int mins = dep - tstamp;
                           
                          
                      // Affichage des infos sur afficheur
                             
                      if (mins <= 0 )
                          mins = 0;
                      USE_SERIAL.println(mins);
                      display.setCursor(3,Ligne);            
                      display.print(" "); display.print(mins); display.println(" mins");
                      Ligne = Ligne + 8;
                      display.display();
                      }
                      if (!rec_id) {
                         display.setCursor(0,45);            
                         display.println("Pas de bus\nmaintenant");
                         display.display();
                         }
                      delay(500); //
                }
         }
}
                 
void get_bus3() {
   display.setCursor(70,35);            
   display.println(F("le 207"));
   
   HTTPClient http;
                 
   USE_SERIAL.println("[HTTP] begin...\n");
                             
   timeClient.update();
               
   USE_SERIAL.println(timeClient.getFormattedTime());
           
   // definir l'url
   http.begin(
       "http://data.explore.star.fr/api/records/1.0/search/"
       "?dataset=tco-bus-circulation-passages-tr&rows=2"
       "&sort=-depart&facet=precision&refine.idligne=" IDLIGNE3
       "&refine.idarret=" IDARRET3
       "&timezone=Europe%2FParis&apikey=" APIKEY
               );
                 
   USE_SERIAL.println("[HTTP] GET...\n");
                
   // lancer la requete
   int httpCode = http.GET();
           
   // httpCode will be negative on error
   if (httpCode > 0) {
        USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);
        USE_SERIAL.println();
                   
        DynamicJsonBuffer jsonBuffer(5000);
                 
        if (httpCode == HTTP_CODE_OK ) {
          String payload = http.getString();
             if (DEBUG) USE_SERIAL.println(payload);
                             
          JsonObject& json = jsonBuffer.parseObject(payload);
             if (DEBUG) USE_SERIAL.println(json["nhits"].as<int>());
                             
          USE_SERIAL.print("found records: "); USE_SERIAL.println(json["records"].size());
                             
          http.end(); // libere les ressources
                             
          int rec_id;
          int Ligne = 45;
          for ( rec_id=0;rec_id<json["records"].size();rec_id++)
              {
               JsonObject& rec0 = json["records"][rec_id];
                                       
              // munites resstantes avant bus
              const char* depart    = rec0["fields"]["depart"]; // "2018-09-22T01:13:00+02:00"
              int dep = ((((depart[11]-48)*10) + (depart[12]-48))*60) + ((depart[14]-48)*10) + (depart[15]-48); // -48 pour passage code ascii en chiffre
              int tstamp = (timeClient.getHours()*60)+ timeClient.getMinutes();
              int mins = dep - tstamp;
                            
              // Affichage des infos sur afficheur
                             
             if (mins <= 0 )
                 mins = 0;
             USE_SERIAL.println(mins);
             display.setCursor(64,Ligne);            
             display.print(" "); display.print(mins); display.println(" mins");
             Ligne = Ligne + 8;
             display.display();
             }
             if (!rec_id) {
                 display.setCursor(64,45);            
                 display.println("Pas de bus");
                 display.setCursor(64,53);            
                 display.println("maintenant");
                 display.display();
                 }
         delay(500); //
       }
  }
}

void get_bus4() {
  display.setCursor(0,16);            
  display.println(F(" le C6"));
  HTTPClient http;
                 
  USE_SERIAL.println("[HTTP] begin...\n");
                            
  timeClient.update();
                 
          
  // definir l'url
  http.begin(
      "http://data.explore.star.fr/api/records/1.0/search/"
      "?dataset=tco-bus-circulation-passages-tr&rows=" ROW
      "&sort=-depart&facet=precision&refine.idligne=" IDLIGNE4
      "&refine.idarret=" IDARRET4
      "&timezone=Europe%2FParis&apikey=" APIKEY
             );
                 
  USE_SERIAL.println("[HTTP] GET...\n");
                
  // lancer la requete
  int httpCode = http.GET();
           
  // httpCode will be negative on error
  if (httpCode > 0) {
     // HTTP header has been send and Server response header has been handled
     USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);
     USE_SERIAL.println();
                   
     DynamicJsonBuffer jsonBuffer(5000);
                 
             if (httpCode == HTTP_CODE_OK ) {
                  String payload = http.getString();
                       if (DEBUG) USE_SERIAL.println(payload);
                             
                  JsonObject& json = jsonBuffer.parseObject(payload);
                       if (DEBUG) USE_SERIAL.println(json["nhits"].as<int>());
                           
                  USE_SERIAL.print("found records: "); USE_SERIAL.println(json["records"].size());
                             
                  http.end(); // libere les ressources
                             
                  int rec_id;
                  int Ligne = 25;
                  for ( rec_id=0;rec_id<json["records"].size();rec_id++)
                      {
                       JsonObject& rec0 = json["records"][rec_id];
                                      
                       // munites resstantes avant bus
                       const char* depart    = rec0["fields"]["depart"]; // "2018-09-22T01:13:00+02:00"
                       int dep = ((((depart[11]-48)*10) + (depart[12]-48))*60) + ((depart[14]-48)*10) + (depart[15]-48); // -48 pour passage code ascii en chiffre
                       int tstamp = (timeClient.getHours()*60)+ timeClient.getMinutes();
                       int mins = dep - tstamp;
                            
                            
                       // Affichage des infos sur afficheur
                             
                       if (mins <= 0 )
                           mins = 0;
                       USE_SERIAL.println(mins);
                      display.setCursor(3,Ligne);            
                      display.print(" "); display.print(mins); display.println(" mins");
                      Ligne = Ligne + 8;
                      display.display();
                       }
                       if (!rec_id) {
                          display.setCursor(0,25);            
                          display.println("Pas de bus\nmaintenant");
                          display.display();
                       }
                  delay(500); //
                  }
   }
}
void get_bus5() {
   display.setCursor(64,16);            
   display.println(F(" le 67"));  
   HTTPClient http;
                 
   USE_SERIAL.println("[HTTP] begin...\n");
                             
   timeClient.update();
             
      
   // definir l'url
   http.begin(
      "http://data.explore.star.fr/api/records/1.0/search/"
      "?dataset=tco-bus-circulation-passages-tr&rows=" ROW
      "&sort=-depart&facet=precision&refine.idligne=" IDLIGNE5
      "&refine.idarret=" IDARRET5
      "&timezone=Europe%2FParis&apikey=" APIKEY
             );
                 
   USE_SERIAL.println("[HTTP] GET...\n");
                
   // lancer la requete
   int httpCode = http.GET();
          
   // httpCode will be negative on error
   if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);
      USE_SERIAL.println();
                   
      DynamicJsonBuffer jsonBuffer(5000);
                 
      if (httpCode == HTTP_CODE_OK ) {
         String payload = http.getString();
               if (DEBUG) USE_SERIAL.println(payload);
                             
        JsonObject& json = jsonBuffer.parseObject(payload);
               if (DEBUG) USE_SERIAL.println(json["nhits"].as<int>());
                             
        USE_SERIAL.print("found records: "); USE_SERIAL.println(json["records"].size());
                             
        http.end(); // libere les ressources
                             
        int rec_id;
        int Ligne = 25;
        for ( rec_id=0;rec_id<json["records"].size();rec_id++)
            {
            JsonObject& rec0 = json["records"][rec_id];
                                      
            // munites resstantes avant bus
            const char* depart    = rec0["fields"]["depart"]; // "2018-09-22T01:13:00+02:00"
            int dep = ((((depart[11]-48)*10) + (depart[12]-48))*60) + ((depart[14]-48)*10) + (depart[15]-48); // -48 pour passage code ascii en chiffre
            int tstamp = (timeClient.getHours()*60)+ timeClient.getMinutes();
            int mins = dep - tstamp;
                            
                             
            // Affichage des infos sur afficheur
                
            if (mins <= 0 )
               mins = 0;
            USE_SERIAL.println(mins);
            display.setCursor(64,Ligne);            
            display.print(" "); display.print(mins); display.println(" mins");
            Ligne = Ligne + 8;
            display.display();
            }
            if (!rec_id) {
                display.setCursor(64,25);            
                display.print("Pas de bus");
                display.setCursor(64,33);            
                display.print("maintenant");
                display.display();
                }
        delay(500); //
      }
   }
}


//*******************************
void setup() {
  
    USE_SERIAL.begin(115200);
    USE_SERIAL.println();
    USE_SERIAL.println();

    USE_SERIAL.printf("[SETUP] WAIT ...\n");
    USE_SERIAL.flush();
   
    pinMode(LED_BUILTIN, OUTPUT);

    init_display();
    connect_wifi();  
    disp_intro(-40,-20);
    wait_for_wifi();
  
// recuperation de l'heure NTP
    timeClient.begin();
 
    digitalWrite(LED_BUILTIN, HIGH); // on eteint la LED
}
//**********************************
void loop() {


 if ((WiFiMulti.run() == WL_CONNECTED)) {    // Quand la connection wifi est etablie
                      
     // Affichage premiere page
     display.clearDisplay();
     get_bus("prochain bus C6 dans",3,IDLIGNE1,IDARRET1);
     get_bus2();
     get_bus3();
     delay(10000);
   
    // Affichage page 2
     display.clearDisplay();
     display.setCursor(0,0);            
     display.println(F("   Vers Republique "));  
     get_bus4();
     get_bus5();
     display.display();
     delay(10000);
    
   }  else delay(100); // wait cnx ...
  
/* else {
      USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        display.println(F("Problème de connexion au serveur"));
        display.display();
      }  */
        
//  display.clearDisplay();
  diplay_type (7);
  diplay_type (8);

          
  // on eteint tout
  USE_SERIAL.println("Dodo ! Please reset now.");
  display.ssd1306_command(SSD1306_DISPLAYOFF); // display Off
  ESP.deepSleep(0);
}


 
  
  
  
