#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <WiFiClient.h>
//#include "pitches.h"
#include <Phant.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN 14

Adafruit_NeoPixel strip = Adafruit_NeoPixel(6, PIN, NEO_GRB + NEO_KHZ800);

WiFiClient client;

#define B3 2024
#define C4 1915
#define D4 1700
#define E4 1519
#define F4 1432
#define FS4 1352
#define G4 1275
#define GS4 415
#define A4 1136
#define AS4 1073
#define B4 1014
#define C5 956
#define D5 852

int tonepin = 4;
int buttpin = 12;

int song = 1;


const char PhantHost[] = "data.sparkfun.com";     //Needed for everyone to connect to the same server. 
const char PublicKey[] = "g6RKzndYWATp0WxXdyNz";
const char PrivateKey[] = "xxxxxxxxxxxxxxxxxxxxx"; //You can request this from me.

//const char http_site[] = "data.sparkfun.com/output/g6RKzndYWATp0WxXdyNz/latest.json";
const int http_port = 80;

// https://data.sparkfun.com/output/g6RKzndYWATp0WxXdyNz/latest.json christmas cheer last post.

const unsigned long postRate = 30000;
unsigned long lastPost = 0;

int CheerLine = 0;
int buttpushed = 0;

unsigned long currentCheer;
unsigned long newCheer;
unsigned long savedCheer;

int sleepTimeS = 120;

/*******************************************************
 * Songs
 ********************************************************/

int tempo = 100;

int  Frostylength = 31;
int  Frostynotes[] = {G4, E4, F4, G4, C5, B4, C5, D5, C5, B4, A4, G4,0, B4, C5, D5, C5, B4, A4, A4, G4, C5, E4, G4, A4, G4, F4, E4, D4, C4, 0};
int  Frostybeats[] = {4, 4, 1, 2, 4, 1, 1, 2, 2, 2, 2, 4, 2, 1, 1, 2, 2, 2, 1, 1, 2, 2, 2, 1, 1, 2, 2, 2, 2, 4, 1};

int  Merrylength = 31;
int  Merrynotes[] = {D4, G4, G4, A4, G4, FS4, E4, E4, E4, A4, A4, B4, A4, G4, FS4, D4, D4, B4, B4, C5, B4, A4, G4, E4, D4, D4, E4, A4, FS4, G4, 0};
int  Merrybeats[] = {2, 2, 1, 1, 1, 1, 2, 2, 2, 2, 1, 1, 1, 1, 2, 2, 2, 2, 1, 1, 1, 1, 2, 2, 1, 1, 2, 2, 2, 4, 1};

int  Jinglelength = 27;
int  Jinglenotes[] = {B4, B4, B4, B4, B4, B4, B4, D5, G4, A4, B4,0, C5, C5, C5, C5, C5, B4, B4, B4, B4, A4, A4, B4, A4, D5, 0};
int  Jinglebeats[] = {2, 2, 4, 2, 2, 4, 2, 2, 2, 2, 4,4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 4, 1};

int Decklength = 18;
int Decknotes[] = {G4, F4, E4, D4, C4, D4, E4, C4, D4, E4, F4, D4, E4, D4, C4, B3, C4, 0};
int Deckbeats[] = {4, 1, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 4, 1, 2, 2, 4, 1};


void setup() {




  EEPROM.begin(24);
  pinMode(tonepin, OUTPUT);
  pinMode(buttpin, INPUT_PULLUP);
  randomSeed(analogRead(A0));

  strip.begin();
  colorWipe(strip.Color(10, 0, 0), 50);
  //strip.setPixelColor(0, 0,50,0);
  //strip.show(); // Initialize all pixels to 'off'
  
  sleepTimeS = random(120, 240);
  int buttstate = 1;
  buttstate = digitalRead(buttpin);
  if (buttstate == 0) {
    Serial.println("Button Pushed - Make Noise, Lights");
    addCheer();
    buttpushed = 1;
    while (buttstate == 0) {
      buttstate = digitalRead(buttpin);
      delay(100);
    }
  }

  Serial.begin(115200);

  //Set Saved EEPROM BIT
  //EEPROM.write(0, 1);
  int saveBit = EEPROM.read(0);
  Serial.print("Savebit = ");
  Serial.println(saveBit);
  if (saveBit != 1) {
    Serial.println("Initializing EEPROM, Only happens first time programming");
    EEPROMWritelong(1, 0);
    EEPROM.write(0, 1);
  }
  EEPROM.commit();
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset saved settings
  //wifiManager.resetSettings();

  //set custom ip for portal
  //wifiManager.setAPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  //fetches ssid and pass from eeprom and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect("Tree Ornament");
  //or use this for auto generated name ESP + ChipID
  //wifiManager.autoConnect();

  Serial.println("Connected");


}

void loop() {
  Serial.println("Retrieving EEMPROM Cheer.");
  savedCheer = EEPROMReadlong(1);  //Retrieve cheer value saved in flash
  Serial.print("Saved Cheer: ");
  Serial.println(savedCheer);
  if ( !getPage() ) {             //Get lastest cheer value from Phant
    Serial.println("GET request failed");
  }


  if (buttpushed == 1) {
    newCheer = currentCheer + 1;
    EEPROMWritelong(1, newCheer);
    EEPROM.commit();
    Serial.print("Sending ");
    Serial.print(newCheer);
    Serial.println(" to Phant");
    postToPhant(newCheer);
    Serial.print("Sleeping for ");
    Serial.print(sleepTimeS);
    Serial.println(" second.");
    ESP.deepSleep(sleepTimeS * 1000000);
  }


  Serial.print("Saved Cheer: ");
  Serial.print(savedCheer);
  Serial.print(" Current Cheer: ");
  Serial.println(currentCheer);
  if (savedCheer < currentCheer) {
    Serial.println("Someone has added Christmas Cheer!");
    Serial.println("Playing Song and Lights");
    Serial.println("Updating Internal Cheer Value");
    //EEPROMWritelong(1, currentCheer);
    playSong();

  }
  EEPROMWritelong(1, currentCheer);
  EEPROM.commit();
  colorWipe(strip.Color(0, 0, 0), 50); // White
  Serial.print("Sleeping for ");
  Serial.print(sleepTimeS);
  Serial.println(" second.");
  ESP.deepSleep(sleepTimeS * 1000000);


}

int postToPhant(int cheer)
{
  Serial.println("Posting to Phant");
  // LED turns on when we enter, it'll go off when we
  // successfully post.
  //digitalWrite(LED_PIN, HIGH);

  // Declare an object from the Phant library - phant
  Phant phant(PhantHost, PublicKey, PrivateKey);

  // Do a little work to get a unique-ish name. Append the
  // last two bytes of the MAC (HEX'd) to "Thing-":
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.macAddress(mac);
  String macID = String(mac[WL_MAC_ADDR_LENGTH - 2], HEX) +
                 String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);
  macID.toUpperCase();
  String postedID = "Thing-" + macID;

  // Add the four field/value pairs defined by our stream:
  phant.add("cheer", cheer);

  // Now connect to data.sparkfun.com, and post our data:
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(PhantHost, httpPort))
  {
    // If we fail to connect, return 0.
    return 0;
  }
  // If we successfully connected, print our Phant post:
  client.print(phant.post());

  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    //Serial.print(line); // Trying to avoid using serial
  }

  // Before we exit, turn the LED off.
  //digitalWrite(LED_PIN, LOW);

  return 1; // Return success
}


bool getPage() {

  // Attempt to make a connection to the remote server
  if ( !client.connect(PhantHost, http_port) ) {
    return false;
  }


  // Make an HTTP GET request
  client.println("GET /output/g6RKzndYWATp0WxXdyNz/latest.json HTTP/1.1");
  client.print("Host: ");
  client.println(PhantHost);
  client.println("Connection: close");
  client.println();

  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 7000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return false;
    }
  }
  while (client.available()) {
    String line = client.readStringUntil('\r');
    int DateLine = line.indexOf("Date:");
    CheerLine = line.indexOf("cheer");
    if (DateLine == 1) {
      Serial.println();
      Serial.println("Found Date");
    }
    if (CheerLine > 0) {
      currentCheer = client.parseInt();
      String temp = line;
      temp.remove(0, 12);
      currentCheer = temp.toInt();
      CheerLine = 0;
      Serial.print(temp);
    }
    Serial.print(line);


  }

  Serial.println();
  Serial.println("closing connection");

  return true;
}


void addCheer() {
  colorWipe(strip.Color(150, 0, 0), 50); // Red
  playNote(C4, 400);
  colorWipe(strip.Color(0, 150, 0), 50); // Green
  playNote(G4, 400);
  colorWipe(strip.Color(150, 150, 150), 50); // White
  delay(500);
  noTone(tonepin);
  colorWipe(strip.Color(0, 0, 0), 50); // White
}


//This function will write a 4 byte (32bit) long to the eeprom at
//the specified address to address + 3.
void EEPROMWritelong(int address, long value)
{
  //Decomposition from a long to 4 bytes by using bitshift.
  //One = Most significant -> Four = Least significant byte
  byte four = (value & 0xFF);
  byte three = ((value >> 8) & 0xFF);
  byte two = ((value >> 16) & 0xFF);
  byte one = ((value >> 24) & 0xFF);

  //Write the 4 bytes into the eeprom memory.
  EEPROM.write(address, four);
  EEPROM.write(address + 1, three);
  EEPROM.write(address + 2, two);
  EEPROM.write(address + 3, one);
}

long EEPROMReadlong(long address)
{
  //Read the 4 bytes from the eeprom memory.
  long four = EEPROM.read(address);
  long three = EEPROM.read(address + 1);
  long two = EEPROM.read(address + 2);
  long one = EEPROM.read(address + 3);

  //Return the recomposed long by using bitshift.
  return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}


void playSong() {
switch (song) {
    case 1:
      Frosty();
      song++;
      break;
    case 2:
      Merry();
      song++;
      break;
    case 3:
      Deck();
      song++;
      break;
      case 4:
      Jingle();
      song = 1;
      break;
  }
}

void Frosty() {
  for (int i = 0; i < Frostylength; i++) {
    if (Frostynotes[i] == 0) {
      delay(Frostybeats[i] * tempo); // rest
      noTone(tonepin);
    } else {
      playNote(Frostynotes[i], Frostybeats[i] * tempo);
    }

    // pause between notes
    delay(tempo / 2);
    if (Frostynotes[i + 1] == Frostynotes[i]) {
      noTone(tonepin);
    }
  }
  noTone(tonepin);
}

void Merry() {
  for (int i = 0; i < Merrylength; i++) {
    if (Merrynotes[i] == 0) {
      delay(Merrybeats[i] * tempo); // rest
      noTone(tonepin);
    } else {
      playNote(Merrynotes[i], Merrybeats[i] * tempo);
    }

    // pause between notes
    delay(tempo / 2);
    if (Merrynotes[i + 1] == Merrynotes[i]) {
      noTone(tonepin);
    }
  }
  noTone(tonepin);
}

void Deck() {
  for (int i = 0; i < Decklength; i++) {
    if (Decknotes[i] == 0) {
      delay(Deckbeats[i] * tempo); // rest
      noTone(tonepin);
    } else {
      
      playNote(Decknotes[i], Deckbeats[i] * tempo);
    }

    // pause between notes
    delay(tempo / 2);
    if (Decknotes[i + 1] == Decknotes[i]) {
      noTone(tonepin);
    }
  }
  noTone(tonepin);
}

void Jingle() {
  for (int i = 0; i < Jinglelength; i++) {
    if (Jinglenotes[i] == 0) {
      delay(Jinglebeats[i] * tempo); // rest
      noTone(tonepin);
    } else {
      
      playNote(Jinglenotes[i], Jinglebeats[i] * tempo);
    }

    // pause between notes
    delay(tempo / 2);
    if (Jinglenotes[i + 1] == Jinglenotes[i]) {
      noTone(tonepin);
    }
  }
  noTone(tonepin);
}

void playNote(int note, int duration) {
  playTone(note, duration);
  int p1 = random(0,6);
  int p2 = random(0,6);
  int p3 = random(0,6);
  int p4 = random(0,6);
  int p5 = random(0,6);
  int p6 = random(0,6);
  strip.setPixelColor(p1, 100,0,0);
  strip.setPixelColor(p2, 0,100,0);
  strip.setPixelColor(p3, 100,100,100);
  strip.setPixelColor(p4, 100,0,0);
  strip.setPixelColor(p5, 0,100,0);
  strip.setPixelColor(p6, 100,100,100);
  strip.show();
}

void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void playTone(int tone, int duration) {
  for (long i = 0; i < duration * 1000L; i += tone * 2) {
    digitalWrite(tonepin, HIGH);
    delayMicroseconds(tone);
    digitalWrite(tonepin, LOW);
    delayMicroseconds(tone);
  }
}

