/*#####################################################
#                                                     #
#          Sketch HoMIDoM-DUINO_Slave V1.0            #
#                                                     #
#   Compatible avec le driver Arduino++_USB V1.2.0.0  #
#                                                     #
#####################################################*/

#include <VirtualWire.h>
#include <Wire.h>
//#include "DHTxx.h"

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

// Zone définition LCD //
#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 7, 14, 15, 16, 17);           // select the pins used on the LCD panel
int lcd_key     = 0;
int adc_key_in  = 0;
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

int read_LCD_buttons(){               // read the buttons
    adc_key_in = analogRead(0);       // read the value from the sensor 

    // my buttons when read are centered at these valies: 0, 144, 329, 504, 741
    // we add approx 50 to those values and check to see if we are close
    // We make this the 1st option for speed reasons since it will be the most likely result

    if (adc_key_in > 1000) return btnNONE; 
/*
    // For V1.1 us this threshold
    if (adc_key_in < 50)   return btnRIGHT;  
    if (adc_key_in < 250)  return btnUP; 
    if (adc_key_in < 450)  return btnDOWN; 
    if (adc_key_in < 650)  return btnLEFT; 
    if (adc_key_in < 850)  return btnSELECT;  
*/

   // For V1.0 comment the other threshold and use the one below:
     if (adc_key_in < 50)   return btnRIGHT;  
     if (adc_key_in < 195)  return btnUP; 
     if (adc_key_in < 380)  return btnDOWN; 
     if (adc_key_in < 555)  return btnLEFT; 
     if (adc_key_in < 790)  return btnSELECT;   

    return btnNONE;                // when all others fail, return this.
}

// Zone définition des constantes (a adapter en fonction de la configuration)
const int ID_ARDUINO = 2;       // Adresse du module Arduino peripherique (1 - 99)
const int led_pin = 13;         // Pin de la LED de test (carte Arduino) - NE PAS MODIFIER !
const int transmit_pin = 12;    // Pin connectée au module transmeteur RF
const int receive_pin = 11;     // Pin connectée au module emeteur RF
const int transmit_en_pin = NULL; // Pin connectée au module transmeteur RF (Enable) (Optionnel)

// Zone définition des variables
String AckMsgTmp;

void(* resetFunc) (void) = 0; //declare reset function @ address 0

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

// Activation du module LCD
    lcd.begin(16, 2);               // start the library
    lcd.setCursor(0,0);             // set the LCD cursor   position 
    lcd.print("Initialisation...");  // print a simple message on the LCD

    pinMode(led_pin, OUTPUT); // Programation de la pin 13 en mode "Digital Output" 
    Serial.println("INIT ADR " + String(ID_ARDUINO) + " OK");
    
    String AckMsgTmp = "INIT ADR " + String(ID_ARDUINO) + " OK";
    char AckMsg[AckMsgTmp.length()+1];
    AckMsgTmp.toCharArray(AckMsg,AckMsgTmp.length()+1);
    RF_Send(AckMsg);
}

void loop() {
  RF_Rec(); // Reception d'une trame RF
  LCD_Key(); // Lecture du clavier LCD
}

// ####################################  ####################################
void LCD_Key(){
}

// #################################### Reception d'une trame RF ####################################
void RF_Rec(){
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
    str = strtok_r(p, " ", &p);

// Detection du debut de trame //
    if (strcmp(str,"ADR") == 0) {
      str = strtok_r(p, " ", &p);
      if (atoi(str) == ID_ARDUINO) { // La commande concerne bien cet Arduino...
        str = strtok_r(p, " ", &p);
        
// Traitemant d'une commande VW ) //
        if (strcmp(str,"VW")== 0) {
          AckMsgTmp = "ACK ADR " + String(ID_ARDUINO) + " VW ";
          char AckMsg[AckMsgTmp.length()+1];
          AckMsgTmp.toCharArray(AckMsg,AckMsgTmp.length()+1);
          RF_Send(AckMsg);
        }

// Traitemant d'une commande VR ) //
        if (strcmp(str,"VR")== 0) {
          AckMsgTmp = "ACK ADR " + String(ID_ARDUINO) + " VR ";
          char AckMsg[AckMsgTmp.length()+1];
          AckMsgTmp.toCharArray(AckMsg,AckMsgTmp.length()+1);
          RF_Send(AckMsg);
        }

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
          String AckMsgTmp = "ACK ADR " + String(ID_ARDUINO) + " DHTXX " + pin + " " + String(h) + " " + String(t);
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
// Traitement d'un ecran LCD //
        if (strcmp(str,"LCD") == 0) {
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
          AckMsgTmp = "ACK ADR " + String(ID_ARDUINO) + " LCD " + String(pin) + " " + String(val);
          char AckMsg[AckMsgTmp.length()+1];
          AckMsgTmp.toCharArray(AckMsg,AckMsgTmp.length()+1);
          RF_Send(AckMsg);
          }
        if (strcmp(str,"SETLCD") == 0) {
          str = strtok_r(p, " ", &p);
          if (str == NULL) {
            argument_error();
            return;
          }
          String param1 = str;
          int CurCol, CurLgn;
          for (int i = 0; i < param1.length(); i++) {
            if (param1.substring(i, i+1) == ",") {
              CurCol = param1.substring(0, i).toInt();
              CurLgn = param1.substring(i+1).toInt();
              break;
            }
          }
          str = strtok_r(p, " ", &p);
          if (str == NULL) {
            argument_error();
            return;
          }
          String param2 = str;
          param2.replace("/#"," ");
          lcd.setCursor(CurCol,CurLgn);
          lcd.print(param2);
          AckMsgTmp = "ACK ADR " + String(ID_ARDUINO) + " SETLCD " + String(param1) + " " + String(param2);
          char AckMsg[AckMsgTmp.length()+1];
          AckMsgTmp.toCharArray(AckMsg,AckMsgTmp.length()+1);
          RF_Send(AckMsg);
          }
      }
    }
  }
}

// #################################### Emission d'une trame RF. ####################################
void RF_Send(char* AckMsg) {
  Serial.println(AckMsg);
  //delay(300);
  vw_send((uint8_t *)AckMsg, strlen(AckMsg));
  vw_wait_tx(); // Attente fin emission du message
}

// ################################# Réinitialisation de l'Arduino. #################################
void reset_command() {
  AckMsgTmp = "ACK ADR " + String(ID_ARDUINO) + " RESET";
  char AckMsg[AckMsgTmp.length()+1]; 
  AckMsgTmp.toCharArray(AckMsg,AckMsgTmp.length()+1);
  RF_Send(AckMsg);
  delay(500);
  resetFunc();  //call reset
}

// #################### Retour Message d'erreur (Nombre d'argument insuffisant). ####################
void argument_error() {
  AckMsgTmp = "ERR argument_error ADR " + String(ID_ARDUINO);
  char AckMsg[AckMsgTmp.length()+1]; 
  AckMsgTmp.toCharArray(AckMsg,AckMsgTmp.length()+1);
  RF_Send(AckMsg);
}

// ########################## Retour Message d'erreur (commande inconnue). ##########################
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

