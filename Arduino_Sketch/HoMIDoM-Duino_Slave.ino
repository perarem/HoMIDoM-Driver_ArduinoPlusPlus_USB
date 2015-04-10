/*#####################################################
#                                                     #
#          Sketch HoMIDoM-DUINO_Slave V1.0            #
#                                                     #
#   Compatible avec le driver Arduino++_USB V1.2.0.0  #
#                                                     #
#####################################################*/

#include <VirtualWire.h>
#include <Wire.h>

// Zone definition RTC (Optionnel) //
#include <RTClib.h>
RTC_DS1307 RTC;

// Zone definition BMP180 (Optionnel) //
#include <SFE_BMP180.h>
SFE_BMP180 pressure;
#define ALTITUDE 25.0 // Altitude du capteur.

// Zone definition One-Wire (Optionnel) //
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 9
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress Sonde_1 = { 0x28, 0xFF, 0x5B, 0x25, 0x62, 0x14, 0x03, 0x83 };
//DeviceAddress Sonde_2 = { 0x__, 0x__, 0x__, 0x__, 0x__, 0x__, 0x__, 0x__ };

// Zone definition DHTxx //
#include <DHT.h>
#define DHTPIN 5         // Pin connectée à la sonde DHTxx
#define DHTTYPE DHT22    // Type de sonde : DHT11, DHT 22 (AM2302),DHT 21 (AM2301)
DHT dht(DHTPIN, DHTTYPE);

// Zone définotion des constantes (a adapter en fonction de la configuration)
const int ID_ARDUINO = 2;       // Adresse du module Arduino peripherique (1 - 99)
const int led_pin = 13;         // Pin de la LED de test (carte Arduino) - NE PAS MODIFIER !
const int transmit_pin = 12;    // Pin connectée au module transmeteur RF
const int receive_pin = 11;     // Pin connectée au module emeteur RF
const int transmit_en_pin = 10; // Pin connectée au module transmeteur RF (Enable) (Optionnel)

// Zone définition des variables
String AckMsgTmp;

void setup()
{
    delay(1000);
    Serial.begin(9600);
    Serial.println("Initialisation...");

  Wire.begin();
  RTC.begin();
  if (! RTC.isrunning()) {
    //Serial.println("L'horloge temps reelle n'a pas demarree!");
    // following line sets the RTC to the date & time this sketch was compiled
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }
  
    vw_set_tx_pin(transmit_pin);
    vw_set_rx_pin(receive_pin);
    vw_set_ptt_pin(transmit_en_pin);
    vw_set_ptt_inverted(true); // Required for DR3100
    vw_setup(2000);	       // Bits per sec
    vw_rx_start();             // Start the receiver PLL running

// Activation des modules One-Wire //
    sensors.begin();
    sensors.setResolution(Sonde_1, ONE_WIRE_BUS);

// Activation des modules DHTxx //
    dht.begin();

    pinMode(led_pin, OUTPUT); // Programation de la pin 13 en mode "Digital Output" 
    Serial.println("Initialisation terminee !");
}

void loop() {
  uint8_t buf[VW_MAX_MESSAGE_LEN]="";
  uint8_t buflen = VW_MAX_MESSAGE_LEN;
  char *str;
  char *p=(char*)buf;
  String AckMsgTmp="";

// Recuperation d'une trame RF //
  if (vw_get_message(buf, &buflen)) { // Non-blocking
    //int i;
    Serial.println(p);
    digitalWrite(led_pin, HIGH);
    str = strtok_r(p, " ", &p);

// Detection du debut de trame //
    if (strcmp(str,"ADR") == 0) {
      str = strtok_r(p, " ", &p);
/*
      String strtmp = String(str);
      String firstVal, secondVal;
      int i;
      for (i = 0; i < strtmp.length(); i++) {
        if (strtmp.substring(i, i+1) == "_") { break; }
      }
          firstVal = strtmp.substring(0, i);
          secondVal = strtmp.substring(i+1);
      Serial.print(strtmp);
      Serial.print(" - ");
      Serial.print(firstVal);
      Serial.print(" - ");
      Serial.println(secondVal);
      char str2[firstVal.length()+1];
      firstVal.toCharArray(str2,firstVal.length()+1);
 */
        if (atoi(str) == ID_ARDUINO) { // La commande concerne bien cet Arduino...
        str = strtok_r(p, " ", &p);
        
// Traitemant d'une commande DR (Digital Read) //
        if (strcmp(str,"DR") == 0) {
          // Action : Lecture d'une broche numérique.
          str = strtok_r(p, " ", &p);
          if(str == NULL) {
            argument_error();
            return;
          }
          int pin = atoi(str);
          int val = digitalRead(pin);
          AckMsgTmp = "ACK ADR " + String(ID_ARDUINO) + " DR " + String(pin) + " " + String(val);
          char AckMsg[AckMsgTmp.length()+1];
          AckMsgTmp.toCharArray(AckMsg,AckMsgTmp.length()+1);
          RF_Send(AckMsg);
        }
        
// Traitement d'une commande DW (Digital Write) //
        if (strcmp(str,"DW") == 0) {
          // Action : Ecriture sur une broche numerique.
          str = strtok_r(p, " ", &p);
          if (str == NULL) {
            argument_error();
            return;
          }
          int pin = atoi(str);
          str = strtok_r(p, " ", &p);
          if (str == NULL) {
            argument_error();
            return;
          }
          int val = atoi(str);
          digitalWrite(pin, val);
          AckMsgTmp = "ACK ADR " + String(ID_ARDUINO) + " DW " + String(pin) + " " + String(val);
          char AckMsg[AckMsgTmp.length()+1];
          AckMsgTmp.toCharArray(AckMsg,AckMsgTmp.length()+1);
          RF_Send(AckMsg);
        }
        
// Traitement d'une commande AR (Analog Read) //
        if (strcmp(str,"AR") == 0) {
          // Action : Lecture d'une broche analogique.
          str = strtok_r(p, " ", &p);
          if (str == NULL) {
            argument_error();
            return;
          }
          int pin = atoi(str);
          int val = analogRead(pin);
          AckMsgTmp = "ACK ADR " + String(ID_ARDUINO) + " AR " + String(pin) + " " + String(val);
          char AckMsg[AckMsgTmp.length()+1];
          AckMsgTmp.toCharArray(AckMsg,AckMsgTmp.length()+1);
          RF_Send(AckMsg);
        }
        
// Traitement d'une commande AW (Analog Write) //
        if (strcmp(str,"AW") == 0) {
          // Action : Ecriture d'une broche analogique (PWM).
          str = strtok_r(p, " ", &p);
          if (str == NULL) {
            argument_error();
            return;
          }
          int pin = atoi(str);
          str = strtok_r(p, " ", &p);
          if (str == NULL) {
            argument_error();
            return;
          }
          int val = atoi(str);
          analogWrite(pin, val);
          AckMsgTmp = "ACK ADR " + String(ID_ARDUINO) + " AW " + String(pin) + " " + String(val);
          char AckMsg[AckMsgTmp.length()+1];
          AckMsgTmp.toCharArray(AckMsg,AckMsgTmp.length()+1);
          RF_Send(AckMsg);
        }
        
// Traitement d'une commande PM (Pin Mode) //
        if (strcmp(str,"PM") == 0) {
          // Action : Paramétrage d'une broche.
          str = strtok_r(p, " ", &p);
          if (str == NULL) {
            argument_error();
            return;
          }
          int pin = atoi(str);
          str = strtok_r(p, " ", &p);
          if (str == NULL) {
            argument_error();
            return;
          }
          // INPUT 0x0
          // OUTPUT 0x1
          int mode = atoi(str);
          pinMode(pin, mode);
          AckMsgTmp = "ACK ADR " + String(ID_ARDUINO) + " PM " + String(pin) + " " + String(mode);
          char AckMsg[AckMsgTmp.length()+1];
          AckMsgTmp.toCharArray(AckMsg,AckMsgTmp.length()+1);
          RF_Send(AckMsg);
        }
        
// Traitement d'une commande PING //
        if (strcmp(str,"PING") == 0) {
          // Action : Simulation, renvoi la commande sur la liaison serie.
          Serial.print("PING");
          str = strtok_r(p, " ", &p);
          if (str != NULL) {
            Serial.print(" ");
            Serial.print(str);
          }
          Serial.println("");
        }
        if (strcmp(str,"RESET") == 0) reset_command();
        
// Traitement d'une sonde de type DHTxx (Temperature & Hygrometrie) //        
        if (strcmp(str,"DHTXX") == 0) {
          // Action : Lecture d'une sonde DHTXX.
          str = strtok_r(p, " ", &p);
          if (str == NULL) {
            argument_error();
            return;
          }
          String pin = String(str);
          float h = dht.readHumidity();
          float t = dht.readTemperature();
          if (isnan(h) || isnan(t)) { 
            //Serial.println("Failed to read from DHTXX sensor!"); 
          } 
          String AckMsgTmp = "ACK ADR " + String(ID_ARDUINO) + " DHTXX " + pin + " " + String(t) + " " + String(h);
          char AckMsg[AckMsgTmp.length()+1];
          AckMsgTmp.toCharArray(AckMsg,AckMsgTmp.length()+1);
          RF_Send(AckMsg);
        }
        
// Traitement d'une sonde de type BMP180 (Pression atmospherique) //
        if (strcmp(str,"BMP180") == 0) {
          // Action : Lecture d'une sonde barometrique (BMP180).
          str = strtok_r(p, " ", &p);
          if (str == NULL) {
            argument_error();
            return;
          }
          String pin = String(str);
          if (pressure.begin()) {
            char status;
            double T,P,p0;//,a;
            status = pressure.startPressure(3);
            if (status != 0) {
              delay(status);
              status = pressure.getPressure(P,T);
              if (status != 0) {
                //Serial.print("absolute pressure: ");
                //Serial.print(P,2);
                //Serial.print(" mb, ");
                //Serial.print(P*0.0295333727,2);
                //Serial.println(" inHg");
                p0 = pressure.sealevel(P,ALTITUDE);
                //Serial.print("Perssion relative (niveau de la mer) : ");
                //Serial.print(p0,2);
                //Serial.println(" mb");
                //Serial.print(p0*0.0295333727,2);
                //Serial.println(" inHg");
                //a = pressure.altitude(P,p0);
                //Serial.print("computed altitude: ");
                //Serial.print(a,0);
                //Serial.print(" meters, ");
                //Serial.print(a*3.28084,0);
                //Serial.println(" feet");
              }
              //else Serial.println("error retrieving pressure measurement\n");
            }
            //else Serial.println("error starting pressure measurement\n");
            //char AckMsg[32];
            char p1[32];
            dtostrf(p0, 4, 2, p1);
            String AckMsgTmp = "ACK ADR " + String(ID_ARDUINO) + " BMP180 " + pin + " " + String(p1);
            char AckMsg[AckMsgTmp.length()+1]; 
            AckMsgTmp.toCharArray(AckMsg,AckMsgTmp.length()+1);
            //sprintf(AckMsg,"ACK ADR 2 ATMO %4.7s",p1);
            RF_Send(AckMsg);
          }
        }
      }
    }
  }
}

// Action : Réponse du périphérique.
void RF_Send(char* AckMsg) {
  Serial.println(AckMsg);
  delay(300);
  vw_send((uint8_t *)AckMsg, strlen(AckMsg));
  vw_wait_tx(); // Wait until the whole message is gone
}

// Action : Reset interfaces.
void reset_command() {
  AckMsgTmp = "ACK ADR " + String(ID_ARDUINO) + " RESET";
  char AckMsg[AckMsgTmp.length()+1]; 
  AckMsgTmp.toCharArray(AckMsg,AckMsgTmp.length()+1);
  RF_Send(AckMsg);
}

// Action : Retour Message d'erreur (Nombre d'argument insuffisant).
void argument_error() {
  AckMsgTmp = "ERR argument_error ADR " + String(ID_ARDUINO);
  char AckMsg[AckMsgTmp.length()+1]; 
  AckMsgTmp.toCharArray(AckMsg,AckMsgTmp.length()+1);
  RF_Send(AckMsg);
}

// Action : Retour Message d'erreur (commande inconnue).
void unrecognized(const char *command) {
  AckMsgTmp = "ERR unknown_command ADR " + String(ID_ARDUINO);
  char AckMsg[AckMsgTmp.length()+1]; 
  AckMsgTmp.toCharArray(AckMsg,AckMsgTmp.length()+1);
  RF_Send(AckMsg);
}

void rtc() {
  DateTime now = RTC.now();
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(' ');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
    
  Serial.print(" since midnight 1/1/1970 = ");
  Serial.print(now.unixtime());
  Serial.print("s = ");
  Serial.print(now.unixtime() / 86400L);
  Serial.println("d");
   
  // calculate a date which is 7 days and 30 seconds into the future
  DateTime future (now.unixtime() + 7 * 86400L + 30);
    
  Serial.print(" now + 7d + 30s: ");
  Serial.print(future.year(), DEC);
  Serial.print('/');
  Serial.print(future.month(), DEC);
  Serial.print('/');
  Serial.print(future.day(), DEC);
  Serial.print(' ');
  Serial.print(future.hour(), DEC);
  Serial.print(':');
  Serial.print(future.minute(), DEC);
  Serial.print(':');
  Serial.print(future.second(), DEC);
  Serial.println();
    
  Serial.println();
}

void Temperature()
{
  sensors.requestTemperatures();
  float tempC = sensors.getTempC(Sonde_1);
  if (tempC == -127.00) {
    Serial.print("Error getting temperature");
  } else {
    Serial.print("C: ");
    Serial.println(tempC);
  }
}

