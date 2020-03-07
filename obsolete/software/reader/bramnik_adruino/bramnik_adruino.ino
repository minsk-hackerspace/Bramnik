
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

//#define DEBUG

#ifdef DEBUG
    #define dbg(a) Serial.print(a)
    #define dbgln(a)    Serial.println(a)
#else
    #define dbg(a) 
    #define dbgln(a)
#endif

Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

//#define NOKEYSOUND
#define SAMEKEYSOUND


int pin_door = A1;

int pin_led_green = A2;
int pin_led_red = A3;
int pin_sound = 2;

int noteNum = 0;

const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns

byte rowPins[ROWS] = {4, 9, 8, 6}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {5, 3, 7}; //connect to the column pinouts of the keypad

char numberKeys[ROWS][COLS] = {
    { '1','2','3' },
    { '4','5','6' },
    { '7','8','9' },
    { '*','0','#' }
};



#define KEYBUF_LEN  8
char keypadBuffer[KEYBUF_LEN] = {0,0,0,0,0,0,0,0};
int keypadPos = 0;

// consts/variables to throttle same NFC reporting
uint8_t last_read_uid[8] = {0,0,0,0,0,0,0,0}; // uid of last read NFC label



//some reader features
volatile bool enable_nfc; 
volatile bool enable_keypad;

volatile bool hasNFCData = false;
volatile bool hasKeypadData = false;

volatile bool doorOpen = false;


volatile int resetNFCDataLoops = 0;
volatile int resetKeypadDataLoops = 0;

const int loopsTooReset = 20;

//commands for i2c communication protocol
const uint8_t CMD_ENABLE = 0x10;
const uint8_t KEYPAD_MASK=0x02;
const uint8_t NFC_MASK=0x01;

const uint8_t CMD_PLAY = 0x20;
const uint8_t GRANTED=0x00;
const uint8_t DENIED=0x01;
const uint8_t WARNING=0x02;

const uint8_t CMD_ASK = 0x30;
const uint8_t STATUS=0x00;
const uint8_t NFC_DATA=0x01;
const uint8_t KEYPAD_DATA=0x02;

// Wire cmd type and value
volatile unsigned int cmd_type;
volatile unsigned int cmd;

volatile byte r_status;

/*Master to slave:*/

/** 0x10 disable NFC reader + disable keypad*/
/** 0x11 enable NFC reader + disable keypad*/
/** 0x12 disable NFC reader + enable keypad*/
/** 0x13 enable NFC reader + enable keypad*/

/** 0x20 32dec play "access granted"*/
/** 0x21 33dec play "access denied"*/
/** 0x22 34dec play "access warn"*/

/** 0x30 48dec ask for status*/
/** 0x31 49dec ask for 32 bytes from NFC*/
/** 0x32 50dec ask for 32 bytes from keypad*/

/*slave to master:*/

/** 0 bytes when no ask before request*/
/** 1 byte of status when 0x30 asked (0 = nothing happens, 1 = has NCF data , 2 = has keypad data, 4 = door open)*/
/** 32 bytes from NFC when 0x31 asked*/
/** 32 bytes from Keypad when 0x32 asked*/



int toneNfc = NOTE_A4;
int toneKey = NOTE_D6;
int toneEnter = NOTE_E7;
int toneClear = NOTE_C3;



//initialize an instance of class NewKeypad
Keypad keypad = Keypad( makeKeymap(numberKeys), rowPins, colPins, ROWS, COLS); 


//function declaration
void updateStatus();

void resetNFCData();
void resetKeypadData();


void keypadEvent(KeypadEvent key);
void requestEvent();
void receiveEvent(int howMany);

void access_granted();
void access_denied();
void access_warning();

void mus_ok();
void mus_no();
void test();

//implementation
void keyAppend(char key) {
  //append keypadBuffer  
  if(hasKeypadData){
    resetKeypadData();
  }
  if(keypadPos<KEYBUF_LEN){
    keypadBuffer[keypadPos++] = key;
  } else {
    keyClear();  
  }
}

void keyClear() {
  dbgln("KEY CLEAR");
  //clear keypadBuffer
  resetKeypadData();
  beep(toneClear,100);
}

void keyEnter() {
  dbgln("KEY ENTER");
  hasKeypadData = true;
  dbgln(keypadBuffer);
  beep(toneEnter,50);
}


void setup(void) {

  pinMode(pin_led_green, OUTPUT);
  pinMode(pin_led_red, OUTPUT);
  pinMode(pin_sound, OUTPUT);
  pinMode(pin_door, INPUT_PULLUP);
  
  digitalWrite(pin_led_green, LOW);
  digitalWrite(pin_led_red, LOW);
  digitalWrite(pin_sound, LOW);

  keypad.addEventListener(keypadEvent);

  Serial.begin(115200);
  dbgln("Hello!");

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  } else {
    // Got ok data, print it out!
    Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
    Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
    Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  }

  enable_keypad = true;
  enable_nfc = true;
  
  // configure board to read RFID tags
  nfc.SAMConfig();

  Wire.begin(0x68);

  Wire.onRequest(requestEvent);
  Wire.onReceive(receiveEvent); 

  Serial.println("Run");

  test();
}



void loop(void) {

  if (enable_keypad) {
    char customKey = keypad.getKey();
  
    if (customKey) {
      dbg("KEY: ");
      dbgln(customKey);
      resetKeypadDataLoops = 0;
      if (customKey == '#') {
        keyEnter();
      } else if (customKey == '*') {
        keyClear();
      } else {
        keyAppend(customKey);
      }
    }
  }
  
  if (enable_nfc) {

    //timeout for NFC read
    //timeout for NFC clear

    uint8_t success;
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
    uint8_t uidLength;   
  
  
    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 20);
    if (success) {
      
      int compare = memcmp(last_read_uid, uid, 7);
      
      if (0 == compare){ 
          //same NFC device
          dbgln("SAME card");
          // nothing
          
      } else {
        
          dbgln("Found card");
          dbg("  UID Value: ");
          dbgln(uidLength);
          
          nfc.PrintHex(uid, uidLength);
          fillLastNFC(uid);
          
      }
      // Display some basic information about the card
    }
  }

  //react on play command
  switch(cmd_type) {
      case CMD_ENABLE:
          break;
      case CMD_PLAY:
          if (cmd == GRANTED) {
              access_granted();
          } else if (cmd == DENIED) {
              access_denied();
          } else if (cmd == WARNING) {
              access_warning();
          }
          cmd = 0;
          cmd_type = 0;
          break;
      case CMD_ASK:
          break;
  };

  
  if (hasKeypadData) {
    resetKeypadDataLoops ++;
    //dbgln(resetKeypadDataLoops);
    if (resetKeypadDataLoops > loopsTooReset) {
        resetKeypadData();
        resetKeypadDataLoops = 0;
    }
  }

  if (hasNFCData) {
    resetNFCDataLoops ++;
    //dbgln(resetNFCDataLoops);
    if (resetNFCDataLoops > loopsTooReset ) {
      resetNFCData();
      resetNFCDataLoops = 0;
    }
  }

  doorOpen = digitalRead(pin_door);
  
  updateStatus();
  dbgln(r_status);
  delay(10);

}


void updateStatus() {
  //update reader status
  r_status = 0;
  r_status = r_status | hasNFCData << 0;
  r_status = r_status | hasKeypadData << 1;
  r_status = r_status | doorOpen << 2;  
}

void fillLastNFC(uint8_t *newNFC){
    hasNFCData = true;
    memcpy(last_read_uid, newNFC, 7);
    resetNFCDataLoops = 0;
    beep(toneNfc, 30);
}

void resetNFCData() {
    dbgln("resetNFCData");
    memset(last_read_uid, 0, 7);
    resetNFCDataLoops = 0;
    hasNFCData = false;    
    updateStatus();
}


void resetKeypadData() {
    dbgln("resetKeypadData");
    memset(keypadBuffer, 0, KEYBUF_LEN);
    keypadPos = 0;
    resetKeypadDataLoops = 0;
    hasKeypadData = false;
    updateStatus();
}


void requestEventD() {
    dbgln("requestEventD");
    Wire.write("lol kek 123456");
}

void requestEvent() {

    dbgln("requestEvent");
    
    switch(cmd_type) {
        case CMD_ENABLE:
            break;
        case CMD_PLAY:
            break;
        case CMD_ASK:
            switch(cmd){
                case STATUS:
                    dbgln("ask: STATUS ");
                    Wire.write(r_status);
                    dbgln(r_status);
                break;
                
                case KEYPAD_DATA:
                    Wire.write(keypadBuffer, keypadPos);
                    resetKeypadData();
                    dbgln("ask: KEYPAD_DATA ");
                break;

                case NFC_DATA:
                    Wire.write(last_read_uid, 7);
                    resetNFCData();
                    dbgln("ask: NFC_DATA ");
                break;
            };
            break;
    };

  
}

void receiveEvent(int howMany) {
    
    dbg("receiveEvent ");
    
    if (howMany == 0) {
      dbgln(" requestEvent coming ...");  
      return;   
    }

    dbg(" howMany = ");
    dbgln(howMany);

    unsigned int x = 0;
    while (Wire.available()) { // loop through all but the last
      x = Wire.read(); // receive last byte as int
    }

    dbgln(x);

    cmd_type = x & 0xf0;
    cmd = x & 0x0f;

    dbg("cmd_type = ");
    dbgln(cmd_type);

    dbg("cmd = ");
    dbgln(cmd);

    switch(cmd_type){
        case CMD_ENABLE:
            dbg("CMD Enable received: ");
            dbgln(cmd);

            enable_keypad = cmd & KEYPAD_MASK;
            enable_nfc = cmd & NFC_MASK;
            
            dbg("enable_keypad: ");
            dbgln(enable_keypad);     
      
            dbg("enable_nfc: ");
            dbgln(enable_nfc);      
            
            break;
        case CMD_PLAY:
            dbg("CMD Play received: ");
            dbgln(cmd);
            break;
        case CMD_ASK:
            dbg("CMD ASK received: ");
            dbgln(cmd);

            if (cmd == 0) {
                dbgln("Request STATUS");
            }

            if (cmd == 1) {
                dbgln("Request NFC");
            }
            
            if (cmd == 2) {
                dbgln("Request KEYPAD");
            }
            break;
    };
}

void keypadEvent(KeypadEvent key) {
  switch (keypad.getState()){
    case PRESSED: {
      dbg("press ");
      int note = toneByKey(key);
      int duration = 2000;
#ifdef SAMEKEYSOUND
      duration = 30;
#endif  
      beep(note, duration);
    }
    break;
    case RELEASED: {
      dbg("release ");
        beep(0,0);
      }
      
    break;
    case HOLD:
      dbg("hold ");
    break;
  }
  dbgln(key);  
}

int toneByKey(char key) {

#ifdef NOKEYSOUND
  return 0;
#endif  
    
  switch (key){
    case '*':
    case '#':
    return 0;
  }
    
#ifdef SAMEKEYSOUND
  return NOTE_D6;
#endif  

  switch (key){
    case '1':
    return NOTE_C3;
    case '2':
    return NOTE_D3;
    case '3':
    return NOTE_E3;
    case '4':
    return NOTE_C4;
    case '5':
    return NOTE_D4;
    case '6':
    return NOTE_E4;
    case '7':
    return NOTE_C5;
    case '8':
    return NOTE_D5;
    case '9':
    return NOTE_E5;
    case '0':
    return NOTE_D6;
  }
  return 0;
}


void access_granted() {
  dbgln("ACCESS GRANTED !!!");

  for ( int i = 0 ; i<4 ; i++) {
      digitalWrite(pin_led_green, HIGH);
      note(NOTE_C4|DUR_16);      
      delay(200);      
      digitalWrite(pin_led_green, LOW);
      note(NOTE_E4|DUR_16);      
      delay(200);  
    }
  
}

void access_denied() {
  dbgln("ACCESS DENIED !!!");
  digitalWrite(pin_led_red, HIGH);
  note(NOTE_A2|DUR_1);  
  delay(1000);
  digitalWrite(pin_led_red, LOW);  
}

void access_warning() {
  dbgln("ACCESS WARNING !!!");
  note(NOTE_A4|DUR_8);
  digitalWrite(pin_led_red, HIGH);
  delay(100);
  digitalWrite(pin_led_green, HIGH);
  delay(100);
  digitalWrite(pin_led_red, LOW);  
  delay(100);
  digitalWrite(pin_led_green, LOW);  
  delay(100);
  note(NOTE_A4|DUR_8);
  digitalWrite(pin_led_red, HIGH);
  delay(100);
  digitalWrite(pin_led_green, HIGH);
  delay(100);
  digitalWrite(pin_led_red, LOW);  
  delay(100);
  digitalWrite(pin_led_green, LOW);
  delay(100);

}



//beep 
void beep(int note, int duration) {
  if (note == NO_SOUND || duration == 0) {
    noTone(pin_sound);
  } else {
    tone(pin_sound, note, duration);
  }
}

//note only for plaing music
void note(unsigned int note) {

  dbgln( "   -   ");
  dbg("  note  = ");
  dbgln(note);
  
  int note_ton = note & 0x0FFF;
  int note_dur = (note >> 12) & 7;
  int note_long = note >> 15;

  dbg(" note_ton = ");
  dbgln(note_ton);
  
  dbg(" note_dur = ");
  dbgln(note_dur);

  dbg(" note_long = ");
  dbgln(note_long);

  
  int tempo = 1024;
  int duration = 2*tempo;
  
  if (note_dur != 0) {
    duration = duration / (1<<note_dur);
  }
  
  if (note_long) {
      duration = duration + duration/2;
  }

  dbg(" duration = ");
  dbgln(duration);
  
  beep(note_ton, duration);
  delay(duration);
  
}



void test() {

  dbgln("test");      

  note(NOTE_F4|DURL_4);
  note(NOTE_C5|DUR_8);  
  note(NOTE_A4|DUR_2);

}

/*
int not1[] = {
  NOTE_G4,NOTE_G4,NOTE_PAUSE,NOTE_G4,NOTE_G4,NO_SOUND,NOTE_G4,NOTE_G4,NOTE_G4,NOTE_G4,NOTE_G4,
   NOTE_B3,NOTE_G3,NOTE_C4,NOTE_G3,NOTE_CS4,NOTE_G3,NOTE_C4,NOTE_G3,NOTE_B3,NOTE_G3,NOTE_C4,NOTE_G3,NOTE_CS4,NOTE_G3,NOTE_C4,NOTE_G3,
   NOTE_E4,NOTE_F4,NOTE_F4,NOTE_F4,NOTE_F4,NOTE_E4,NOTE_E4,NOTE_E4,
   NOTE_E4,NOTE_G4,NOTE_G4,NOTE_G4,NOTE_G4,NOTE_E4,NOTE_E4,NOTE_E4,
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
  8,8,2,8,8,2,16,8,16,8,8,
  2,4,2,4,2,4,2,4,2,4,2,4,2,4,2,4,
  8,16,16,8,4,8,8,8,
  8,16,16,8,4,8,8,8,
  8,16,16,8,4,8,8,8,
  8,16,16,8,4,8,8,8,
  8,16,16,8,4,8,8,8,
  8,16,16,8,4,8,8,8,
  8,2,8,8,1,
  8,4,8,4,8,8,
  8,8,4,8,4,8,
  4,8,4,8,1
};

*/

void music(int nots[], int durs[], int count) {
  for (int Note = 0; Note < count; Note++) {
    int duration = 1450/durs[Note];
    int cur_note = nots[Note];
    note(cur_note);
  }  
} 


void mus_ok() {
   
}

void mus_no() {
   
}


