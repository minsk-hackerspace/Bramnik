
/**************************************************************************/
/*! 
    This example of arduino sketch for Bramnik outdoor reader
*/
/**************************************************************************/

#include <Keypad.h>
#include <ctype.h>

#include "pitches.h"

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

// If using the breakout with SPI, define the pins for SPI communication.
#define PN532_SCK  (13)
#define PN532_MOSI (11)
#define PN532_SS   (10)
#define PN532_MISO (12)

Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);


int pin_led_green = A0;
int pin_led_red = A1;
int pin_sound = 2;

int noteNum = 0;

const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns

char numberKeys[ROWS][COLS] = {
    { '7','8','9' },
    { '4','5','6' },
    { '1','2','3' },    
    { '*','0','#' }
};

byte rowPins[ROWS] = {3, 4, 5, 6}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {7, 8, 9}; //connect to the column pinouts of the keypad

//initialize an instance of class NewKeypad
Keypad keypad = Keypad( makeKeymap(numberKeys), rowPins, colPins, ROWS, COLS); 

void setup(void) {

  pinMode(pin_led_green, OUTPUT);
  pinMode(pin_led_red, OUTPUT);
  pinMode(pin_sound, OUTPUT);

  digitalWrite(pin_led_green, LOW);
  digitalWrite(pin_led_red, LOW);
  digitalWrite(pin_sound, LOW);
  
  keypad.addEventListener(keypadEvent);
  
  Serial.begin(115200);
  Serial.println("Hello!");

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  
  // configure board to read RFID tags
  nfc.SAMConfig();
  
  Serial.println("Waiting for an ISO14443A Card ...");
}

void loop(void) {

  
  
  char customKey = keypad.getKey();
  
  if (customKey){
    //Serial.println(customKey);
    if (customKey == '*') {
      firstSection();
    }
    if (customKey == '#') {
      mus1();
    }
  }
  

  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;   

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength,20);
  
  if (success) {
    tone(pin_sound, NOTE_A4, 20);      
    
    // Display some basic information about the card

    Serial.println("Found an ISO14443A card");
    Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    Serial.println("");
  }     
  


}


void keypadEvent(KeypadEvent key){
  switch (keypad.getState()){
    case PRESSED: {
      Serial.print("press ");
      int note = toneByKey(key);
      if (note) {
        tone(pin_sound, note, 2000);
      }
    }
    break;
    case RELEASED: {
      Serial.print("release ");
      noTone(pin_sound);
      }
      
    break;
    case HOLD:
      Serial.print("hold ");
    break;
  }
  Serial.println(key);  
}

int toneByKey(char key) {
  switch (key){
    case '7':
    return NOTE_C3;
    case '8':
    return NOTE_D3;
    case '9':
    return NOTE_E3;
    case '4':
    return NOTE_C4;
    case '5':
    return NOTE_D4;
    case '6':
    return NOTE_E4;
    
  }
  return 0;
}


void firstSection()
{
  beep(a, 500);
  beep(a, 500);    
  beep(a, 500);
  beep(f, 350);
  beep(cH, 150);  
  beep(a, 500);
  beep(f, 350);
  beep(cH, 150);
  beep(a, 650);
 
  delay(500);
 
  beep(eH, 500);
  beep(eH, 500);
  beep(eH, 500);  
  beep(fH, 350);
  beep(cH, 150);
  beep(gS, 500);
  beep(f, 350);
  beep(cH, 150);
  beep(a, 650);
 
  delay(500);
}

void beep(int note, int duration) {

  if (note == NO_SOUND) {
    noTone(pin_sound);
  } else {
    tone(pin_sound, note, duration);
  }
  if(noteNum % 2 == 0) {
    digitalWrite(pin_led_green, HIGH);
    digitalWrite(pin_led_red, LOW);
  } else {
    digitalWrite(pin_led_green, LOW);
    digitalWrite(pin_led_red, HIGH);
  }

  delay(duration*1.1);
  
  noteNum++;
  
}




int not1[] = {
  /*NOTE_G4,NOTE_G4,NO_SOUND,NOTE_G4,NOTE_G4,NO_SOUND,NOTE_G4,NOTE_G4,NOTE_G4,NOTE_G4,NOTE_G4,
   NOTE_B3,NOTE_G3,NOTE_C4,NOTE_G3,NOTE_CS4,NOTE_G3,NOTE_C4,NOTE_G3,NOTE_B3,NOTE_G3,NOTE_C4,NOTE_G3,NOTE_CS4,NOTE_G3,NOTE_C4,NOTE_G3,
   NOTE_E4,NOTE_F4,NOTE_F4,NOTE_F4,NOTE_F4,NOTE_E4,NOTE_E4,NOTE_E4,
   NOTE_E4,NOTE_G4,NOTE_G4,NOTE_G4,NOTE_G4,NOTE_E4,NOTE_E4,NOTE_E4,*/
   //Introduction
  NOTE_E4,NOTE_F4,NOTE_F4,NOTE_F4,NOTE_F4,NOTE_E4,NOTE_E4,NOTE_E4,
  NOTE_E4,NOTE_G4,NOTE_G4,NOTE_G4,NOTE_G4,NOTE_E4,NOTE_E4,NOTE_E4,
  NOTE_E4,NOTE_F4,NOTE_F4,NOTE_F4,NOTE_F4,NOTE_E4,NOTE_E4,NOTE_E4,
  NOTE_E4,NOTE_G4,NOTE_G4,NOTE_G4,NOTE_G4,NOTE_E4,NOTE_E4,NOTE_E4,
  NOTE_DS5,NOTE_D5,NOTE_B4,NOTE_A4,NOTE_B4,
  NOTE_E4,NOTE_G4,NOTE_DS5,NOTE_D5,NOTE_G4,NOTE_B4,
  NOTE_B4,NOTE_FS5,NOTE_F5,NOTE_B4,NOTE_D5,NOTE_AS5,
  NOTE_A5,NOTE_F5,NOTE_A5,NOTE_DS6,NOTE_D6,NO_SOUND
};

// note duration: 1 = whole note, 2 = half note, 4 = quarter note, 8 = eighth note, etc.
int dur1[] = {
  /*8,8,2,8,8,2,16,8,16,8,8,
   2,4,2,4,2,4,2,4,2,4,2,4,2,4,2,4,
   8,16,16,8,4,8,8,8,
   8,16,16,8,4,8,8,8,*/
  8,16,16,8,4,8,8,8,
  8,16,16,8,4,8,8,8,
  8,16,16,8,4,8,8,8,
  8,16,16,8,4,8,8,8,
  8,2,8,8,1,
  8,4,8,4,8,8,
  8,8,4,8,4,8,
  4,8,4,8,3
};

void music(int nots[], int durs[], int count) {
  for (int Note = 0; Note < count; Note++) {
    int duration = 1450/durs[Note];
    int cur_note = nots[Note];
    beep(cur_note,duration);
  }  
  
  digitalWrite(pin_led_green, LOW);
  digitalWrite(pin_led_red, LOW);
  
} 

void mus1() {
   music(not1,dur1,54);
}
void mus2() {
    
}

