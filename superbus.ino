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
#include "gfx.c" // image, include directly as C

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
    SCREEN_WIDTH, SCREEN_HEIGHT, // screen size
    OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS
);

const int DEBUG=0;
unsigned int card_id=0; // card identifier based on MAC NIC 

int rec_id;
int mins;

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

int wait_for_wifi() // returns card_id from mac
{
    int i = 0;
    int y = -30;
    int z = -20;
    while (WiFi.status() != WL_CONNECTED) { // En attente de connection
        z = z + 10;
        y = y + 10;
        USE_SERIAL.print(++i);
        USE_SERIAL.print('.');
        disp_intro(y,z);
    }

    // get mac, card_id
    unsigned char mac[6];
    WiFi.macAddress(mac); // fill in mac address
    card_id = mac[3]<<16|mac[4]<<8|mac[5];  // mac address is 3 bytes OUI, 3 bytes NIC.

    USE_SERIAL.print("Connected to ");
    USE_SERIAL.println(WiFi.SSID());
    USE_SERIAL.print("IP address:\t");
    USE_SERIAL.println(WiFi.localIP()); // utile pour recuperer l'IP et la rendre fixe sur notre Livebox
    USE_SERIAL.print("Adresse MAC :");
    for (int i=0;i<5;i++) 
        USE_SERIAL.printf("%X:",mac[i]);
    USE_SERIAL.printf("%X (id: 0x%X)\n",mac[5], card_id);
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

    for (int i=j; i<k; i+=2) {
        display.clearDisplay();
        display.drawXBitmap(i, 0,superbus_bits, superbus_width, superbus_height,WHITE);
        display.display();
        delay(100);
    }

    delay(500);

    display.clearDisplay();
    display.setTextSize(1);             // Normal 1:1 pixel scale
    display.setTextColor(WHITE);        // Draw white text
    display.setCursor(0,0);             // Start at top-left corner
}

#define BUFSIZE 512
char url_buffer[BUFSIZE];

void get_bus(int x, int y, int rows,char *idligne,char *idarret ) {

    display.setCursor(x,y);
    display.setTextSize(1);
    // display.println(title);
    USE_SERIAL.println(idligne);
    HTTPClient http;

    USE_SERIAL.println("[HTTP] begin...\n");

    timeClient.update();

    // definir l'url

    snprintf(url_buffer,BUFSIZE, "http://data.explore.star.fr/api/records/1.0/search/"
             "?dataset=tco-bus-circulation-passages-tr&rows=%d"
             "&sort=-depart&facet=precision&refine.idligne=%s"
             "&refine.idarret=%s"
             "&timezone=Europe%%2FParis&apikey=%s",rows,idligne,idarret,APIKEY );

    http.begin(url_buffer);


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

            USE_SERIAL.print("found records: ");
            USE_SERIAL.println(json["records"].size());

            http.end(); // libere les ressources

            int rec_id;
            for ( rec_id=0; rec_id<json["records"].size(); rec_id++)
            {
                JsonObject& rec0 = json["records"][rec_id];

                // minutes restantes avant bus
                const char* depart    = rec0["fields"]["depart"]; // "2018-09-22T01:13:00+02:00"
                int dep = ((((depart[11]-48)*10) + (depart[12]-48))*60) + ((depart[14]-48)*10) + (depart[15]-48); // -48 pour passage code ascii en chiffre
                int tstamp = (timeClient.getHours()*60)+ timeClient.getMinutes();
                int mins = dep - tstamp;

                // Affichage des infos sur afficheur

                if (mins <= 0 )
                    mins = 0;
                USE_SERIAL.println(mins);
                display.setCursor(x,y);
                display.print(mins);
                display.println(" min.");
                y += 8;
            }
            if (!rec_id) {
                display.setCursor(x,y);
                display.println("--");
                y += 8;
            }
            // send to display now
            display.display();
        }
    } else {
        USE_SERIAL.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        display.println(F("Problème de connexion au serveur"));
        display.display();
    }
}

void display_time () {
    for (int x = 0; x<10; x++) {
        timeClient.update();
        display.clearDisplay();
        display.setCursor(10,25);
        display.setTextSize(2);
        // display.print(" il est ");
        display.println(timeClient.getFormattedTime());
        if (x>5) {
          display.setTextSize(1);
          display.print(F("Appuyer sur le bouton "));
          display.print(" pour rafraichir");
        }
        display.display();
        delay (1000);
    }
}
void sleep() {
    // on eteint tout maintenant
    USE_SERIAL.println("Dodo ! Please reset now.");
    display.ssd1306_command(SSD1306_DISPLAYOFF); // display Off
    ESP.deepSleep(0);
}

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

void affiche_fred() {
    // Affichage premiere page
    display.clearDisplay();
    display.println(F("prochain bus C6 dans\n\n\n le 31      le 207"));
    get_bus( 0, 8, 3, "0002", "1043");
    
    get_bus( 0,43, 2, "0002", "1043");
    get_bus(64,43, 2, "0002", "1043");
    delay(10000);
  
    // Affichage page 2
    display.clearDisplay();
    display.setCursor(0,0);
    display.println(F("   Vers Republique "));    
    display.println(F("  le C6        le 67"));    
    
    get_bus( 0,16, 3, "0002", "1043");
    get_bus(64,16, 3, "0002", "1043");
    delay(10000);
    
    display_time();
}

void minitime() {
  display.setCursor(80,0);
  display.fillRect(80,0,128,8,0);// clear 
  display.println(timeClient.getFormattedTime());
  display.display();
}

void affiche_xav() {
    display.clearDisplay();
    display.setCursor(0,0);
    display.setTextColor(0,1); // inverse
    display.print(F("HorlogeoBus\n"));     
    display.setTextColor(1,0); // normal
    display.println(F("  vers Repu  StGreg"));
    display.println(F("C2:\n\n\n12:"));
    display.drawLine(72,20,72,48,1);
    
    get_bus(20, 16, 3, "0002", "1043");
    get_bus(20, 42, 3, "0012", "1043");
    get_bus(78, 16, 3, "0002", "1099");
    get_bus(78, 42, 3, "0012", "1099");
    
    for (int i=0;i<15;i++) {
      minitime();
      delay(1000); // update time      
    }
    
    // end splash    
    display.fillRect(16,20,96,38,0);
    display.drawRect(16,20,96,38,1);
    
    display.setCursor(20,24); display.print("Appuyer sur le");
    display.setCursor(20,32); display.print(" Bouton  pour");
    display.setCursor(20,40); display.print("  rafraichir");

    display.display();
    
    for (int i=0;i<4;i++) {
      minitime();
      delay(1000); // update time      
    }

}

void loop() {
    if ((WiFiMulti.run() == WL_CONNECTED)) {    // Quand la connection wifi est etablie
      if ( card_id == 0x4517f3 )
        affiche_xav();
      else 
        affiche_fred();
      sleep();
        
    } else {
      delay(100); // wait cnx ...
    }
}
