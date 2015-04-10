#include <SerialCommand.h>
#include <VirtualWire.h>

void argument_error();

SerialCommand sCmd;

const int transmit_pin    = 12;
const int receive_pin     = 11;
const int transmit_en_pin = 13;

String Argument;
char* arg;

void digital_read_command();
void digital_write_command();
void analog_read_command();
void analog_write_command();
void reset_command();
void pin_mode_command();
void ping_command();
void unrecognized(const char *command);

void setup() {
  Serial.begin(9600);     
//  Serial.println("Initialisation...");

  vw_set_tx_pin(transmit_pin);
  vw_set_rx_pin(receive_pin);
  vw_set_ptt_pin(transmit_en_pin);
  vw_setup(2000); // Bits par seconde
  vw_rx_start();  // Demarre le recepteur

  sCmd.addCommand("ADR", transmit_command);
  sCmd.addCommand("DR", digital_read_command);
  sCmd.addCommand("DW", digital_write_command);
  sCmd.addCommand("AR", analog_read_command);
  sCmd.addCommand("AW", analog_write_command);
  sCmd.addCommand("PM", pin_mode_command);
  sCmd.addCommand("RF", rf_command);    
  sCmd.addCommand("PING", ping_command);
//  sCmd.addCommand("DHT", dht_command);
//  sCmd.addCommand("ONE-WIRE", dht_command);
//  sCmd.addCommand("RTC", dht_command);
//  sCmd.addCommand("ATMO", dht_command);
  sCmd.addCommand("RESET", reset_command);
  Serial.write("INIT ARDUINO 0 OK");
}

void loop() {
//  Serial.println("Lecture port com");
  sCmd.readSerial();
//  delay(1000);
//  Serial.println("Lecture RF");
  RF_Read();
//  delay(1000);
}

void rf_command() {
  Argument = "";
  do {
    arg = sCmd.next();
//    Serial.print("arg : ");
//    Serial.println(arg);
    if(arg == NULL && Argument == "") {
      argument_error();
      return;
    }
    if(arg == NULL && Argument != "") {
      break;
    }
    for(int i = 0; arg[i] != NULL; i++) {
      Argument += arg[i];
//      Serial.print(arg[i]);
    }
    Argument += " ";
//    Serial.print("Argument : ");
//    Serial.println(Argument);
  } 
  while(arg != NULL);
  
  char ArgumentChar[Argument.length()+1]; // tableau de char de la taille du String param+1 (caractère de fin de ligne) 
  Argument.toCharArray(ArgumentChar,Argument.length()+1); // récupère le param dans le tableau de char  
//  Serial.print("RF commande  : ");
//  Serial.println(ArgumentChar);
  delay(200);
  vw_send((uint8_t *)ArgumentChar, strlen(ArgumentChar));
  vw_wait_tx(); // Wait until the whole message is gone
}

// Action : Transmission d'un résultat
void transmit_command() {
  char* arg = sCmd.next();
  if(arg == NULL) {
    argument_error();
    return;
  }
  int pin = atoi(arg);
  //int val = digitalRead(pin);
  Serial.print("ACK transmit");
  Serial.println(arg);
}

// Action : Lecture d'une broche numérique.
void digital_read_command() {
  char* arg = sCmd.next();
  if(arg == NULL) {
    argument_error();
    return;
  }
  int pin = atoi(arg);
  int val = digitalRead(pin);
  Serial.print("ACK ");
  Serial.print("0");
  Serial.println(val);
}

// Action : Lecture d'une broche analogique.
void analog_read_command() {
  char* arg = sCmd.next();
  if(arg == NULL) {
    argument_error();
    return;
  }
  int pin = atoi(arg);
  int val = analogRead(pin);
  Serial.print("ACK ");
  Serial.println(val);
}

// Action : Ecriture sur une broche numerique.
void digital_write_command() {
  char* arg = sCmd.next();
  if(arg == NULL) {
    argument_error();
    return;
  }
  int pin = atoi(arg);
  arg = sCmd.next();
  if(arg == NULL) {
    argument_error();
    return;
  }
  int val = atoi(arg);
  digitalWrite(pin, val);
  Serial.println("ACK");
}

// Action : Ecriture d'une broche analogique (PWM).
void analog_write_command() {
  char* arg = sCmd.next();
  if(arg == NULL) {
    argument_error();
    return;
  }
  int pin = atoi(arg);
  arg = sCmd.next();
  if(arg == NULL) {
    argument_error();
    return;
  }
  int val = atoi(arg);
  analogWrite(pin, val);
  Serial.println("ACK");
}

// Action : Paramétrage d'une broche.
void pin_mode_command() {
  char* arg = sCmd.next();
  if(arg == NULL) {
    argument_error();
    return;
  }
  int pin = atoi(arg);
  arg = sCmd.next();
  if(arg == NULL) {
    argument_error();
    return;
  }
  // INPUT 0x0
  // OUTPUT 0x1
  int mode = atoi(arg);
  pinMode(pin, mode);
  Serial.println("ACK");	
}


// Action : Simulation, renvoi la commande sur la liaison serie.
void ping_command() {
  char *arg;
  Serial.print("PING");
  arg = sCmd.next();
  if (arg != NULL) {
    Serial.print(" ");
    Serial.print(arg);
  }
  Serial.println("");
}


// Action : Reset interfaces.
void reset_command() {
//  RFControl::stopReceiving();
  Serial.println("ready");
}

// Action : Retour Message d'erreur (Nombre d'argument insuffisant).
void argument_error() {
  Serial.println("ERR argument_error");
}
// This gets set as the default handler, and gets called when no other command matches.
// Action : Retour Message d'erreur (commande inconnue).
void unrecognized(const char *command) {
  Serial.println("ERR unknown_command");
}

void RF_Read() {
    uint8_t buf[VW_MAX_MESSAGE_LEN]="";
    uint8_t buflen = VW_MAX_MESSAGE_LEN;

    if (vw_get_message(buf, &buflen)) {
      char *AckMsg = (char*)buf;
      //Serial.print("RF reception : ");
      Serial.println(AckMsg);
      //delay(200);
    }
}

