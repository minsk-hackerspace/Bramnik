
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

#define DEBUG

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

int pin_led_green = A0;
int pin_led_red = A1;
int pin_sound = 2;

int noteNum = 0;

const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns


// consts/variables to throttle same NFC reporting
uint8_t last_read_uid[32] = { 0, 0, 0, 0, 0, 0, 0 }; // uid of last read NFC label
unsigned long    last_read_time  = -1; // Time of last successful NFC read
const   unsigned long read_debounce = 1000000 * 1; // 1 second
const   unsigned long NFC_data_timeout = 1000000 * 10; // 10 seconds


/*Master to slave:*/

/** 0x10 disable NFC reader + disable keypad*/
/** 0x11 enable NFC reader + disable keypad*/
/** 0x12 disable NFC reader + enable keypad*/
/** 0x13 enable NFC reader + enable keypad*/
/** 0x20 play "access denied"*/
/** 0x21 play "access granted"*/
/** 0x30 ask for status*/
/** 0x31 ask for 32 bytes from NFC*/
/** 0x32 ask for 32 bytes from keypad*/

/*slave to master:*/

/** 0 bytes when no ask before request*/
/** 1 byte of status when 0x30 asked (0 = nothing happens, 1 = has NCF data , 2 = has keypad data)*/
/** 32 bytes from NFC when 0x31 asked*/
/** 32 bytes from Keypad when 0x32 asked*/

const uint8_t CMD_ENABLE = 0x10;
const uint8_t KEYPAD_MASK=0x02;
const uint8_t NFC_MASK=0x01;

const uint8_t CMD_PLAY = 0x20;
const uint8_t GRANTED=0x00;
const uint8_t DENIED=0x01;

const uint8_t CMD_ASK = 0x30;
const uint8_t STATUS=0x00;
const uint8_t NFC_DATA=0x01;
const uint8_t KEYPAD_DATA=0x02;

// Wire cmd type and value
volatile unsigned int cmd_type;
volatile unsigned int cmd;



//some reader features
bool enable_nfc; 
bool enable_keypad;

char numberKeys[ROWS][COLS] = {
    { '7','8','9' },
    { '4','5','6' },
    { '1','2','3' },
    { 'C','0','.' }
};

int toneNfc = NOTE_A4;
int toneKey = NOTE_D6;
int toneEnter = NOTE_E7;
int toneClear = NOTE_C3;


byte rowPins[ROWS] = {3, 4, 5, 6}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {7, 8, 9}; //connect to the column pinouts of the keypad

//initialize an instance of class NewKeypad
Keypad keypad = Keypad( makeKeymap(numberKeys), rowPins, colPins, ROWS, COLS); 

#define KEYBUF_LEN  32
char keypadBuffer[KEYBUF_LEN] = {};
int keypadPos = 0;
bool keypadEntered = false;

//function declaration

void keypadEvent(KeypadEvent key);
void requestEvent();
void receiveEvent(int howMany);
void mus_ok();
void mus_no();
void test();

//implementation
void keyAppend(char key) {
  //append keypadBuffer  
  if(keypadEntered){
    keyClearMuted();
  }
  if(keypadPos<KEYBUF_LEN){
    keypadBuffer[keypadPos++] = key;
  } else {
    keyClear();  
  }
}

void keyClearMuted() {
  keypadPos = 0;
  memset(keypadBuffer, 0, KEYBUF_LEN);
  keypadEntered = false;
}
void keyClear() {
  //clear keypadBuffer
  keyClearMuted();
  //beep(toneClear,100);
  mus_ok();
}

void keyEnter() {
  //send keypadBuffer to host
  keypadEntered = true;
  dbgln(keypadBuffer);
  //beep(toneEnter,50);
  mus_no();
}


void setup(void) {

  pinMode(pin_led_green, OUTPUT);
  pinMode(pin_led_red, OUTPUT);
  pinMode(pin_sound, OUTPUT);

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

  Serial.println("Waiting for an ISO14443A Card ...");
  
}



void loop(void) {

  if (enable_keypad) {
    char customKey = keypad.getKey();
  
    if (customKey) {
      
      if (customKey == '.') {
        //keyEnter();
      } else if (customKey == 'C') {
        test();
        //keyClear();
      } else {
        keyAppend(customKey);
      }
    }
  }
  
  if (enable_nfc) {
  
    uint8_t success;
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
    uint8_t uidLength;   
  
  
    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 20);
    if (success) {
      int compare = memcmp(last_read_uid, uid, 7);
      unsigned long time_passed = micros()-last_read_time;
      if (0 == compare && time_passed < read_debounce ){ //same NFC device
  
          dbgln(time_passed);
          dbgln(micros()-last_read_time);
          
      } else {
        
          beep(toneNfc, 30);
          Serial.println("Found an ISO14443A card");
          Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
          Serial.print("  UID Value: ");
          nfc.PrintHex(uid, uidLength);
          Serial.println("");
          long tm = fillLastNFC(uid);
          Serial.println(tm);
      }
      // Display some basic information about the card
    }
  }



}

long fillLastNFC(uint8_t *newNFC){
    memcpy(last_read_uid, newNFC, 7);
    last_read_time = micros();
    return last_read_time;
}


void requestEvent() {

    dbgln("requestEvent");
    uint8_t status;
    unsigned long time_since_last_read;

    dbg("cmd_type: ");
    dbgln(cmd_type);

    dbg("cmd: ");
    dbgln(cmd);

    switch(cmd_type) {
        case CMD_ENABLE:
            break;
        case CMD_PLAY:
            break;
        case CMD_ASK:
            dbgln("CMD_ASK");
            switch(cmd){
                case STATUS:
                    dbgln("ask: STATUS ");
                    time_since_last_read = micros() - last_read_time;
                    status = time_since_last_read < NFC_data_timeout ? 1 : 0;
                    
                    status |= keypadEntered ? 0x02 : 0;
                    
                    dbg("responce: ");
                    dbgln(status);
                    Wire.write(status);

                break;
                case KEYPAD_DATA:
                    dbgln("ask: KEYPAD_DATA ");
                    Wire.write(keypadBuffer, keypadPos);
                    keyClearMuted();
                break;

                case NFC_DATA:
                    dbgln("ask: NFC_DATA ");
                    Wire.write(last_read_uid, 7);
                    last_read_time = 0;
                break;
            };
            break;
    };

  
}

void receiveEvent(int howMany) {

    if (howMany == 0) {
      dbgln("requestEvent coming ");  
      return;   
    }
    
    dbg("receiveEvent ");
    dbgln(howMany);

    unsigned int x = Wire.read();    // receive byte as an integer
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
            
            if (cmd == 0) {
                dbgln("ACCESS DENIED");
                mus_no();
            }
            if (cmd == 1) {
                dbgln("ACCESS GRANTED !!!");
                mus_ok();
            }            
            break;
        case CMD_ASK:
            dbg("CMD ASK received: ");
            dbgln(cmd);
            break;
    };
}

void keypadEvent(KeypadEvent key){
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
    case '.':
    case 'C':
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

void starwars() {

  note(NOTE_A4|DUR_2);
  note(NOTE_A4|DUR_2); 
  note(NOTE_A4|DUR_2);
  note(NOTE_F4|DURL_4);
  note(NOTE_C5|DUR_8);  
  note(NOTE_A4|DUR_2);
  note(NOTE_F4|DURL_4);
  note(NOTE_C5|DUR_8);
  note(NOTE_A4|DURL_2);

  note(NOTE_PAUSE|DURL_4);
  
  note(NOTE_E5|DUR_2);
  note(NOTE_E5|DUR_2);
  note(NOTE_E5|DUR_2);  
  note(NOTE_F5|DURL_4);
  note(NOTE_C5|DUR_8);
  note(NOTE_GS4|DUR_2);
  note(NOTE_F4|DURL_4);
  note(NOTE_C5|DUR_8);
  note(NOTE_A4|DURL_2);
 
  note(NOTE_PAUSE|DURL_4);
}


void mario(){
  
  note(NOTE_C4|DUR_8);
  
  note(NOTE_PAUSE|DUR_4);
  
  note(NOTE_G3|DUR_8);
  note(NOTE_PAUSE|DUR_4);
  
  note(NOTE_E3|DUR_4);  

  note(NOTE_A3|DURL_8);  
  note(NOTE_B3|DURL_8);
  note(NOTE_A3|DURL_8);

  note(NOTE_GS3|DURL_8);
  note(NOTE_AS3|DURL_8);  
  
  note(NOTE_GS3|DURL_8);
  
  note(NOTE_G3|DUR_8);
  note(NOTE_F3|DUR_8);

  note(NOTE_G3|DUR_2);
    
}

void test() {

  dbgln("test");      
  //starwars();
  mario();
}


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

void music(int nots[], int durs[], int count) {
  for (int Note = 0; Note < count; Note++) {
    int duration = 1450/durs[Note];
    int cur_note = nots[Note];
    note(cur_note);
  }  
  
  
} 

void mus1() {
  
  digitalWrite(pin_led_green, HIGH);
  music(not1,dur1,54);
  digitalWrite(pin_led_green, LOW);
  digitalWrite(pin_led_red, LOW);
  
}
void mus2() {
  
   digitalWrite(pin_led_red, HIGH);  
   
  digitalWrite(pin_led_green, LOW);
  digitalWrite(pin_led_red, LOW);
   
}

void mus_ok() {
   mus1();
}

void mus_no() {
   mus2();
}

/*
#include <Wire.h>

int led = 13;
bool on = LOW;

int rd;
int count;
int num;

byte i2c_arr_9[9] = {9,1,2,3,4,5,6,7,8};    
byte i2c_arr_12[12] = {12,1,2,3,4,5,6,7,8,9,10,11};
byte i2c_arr_32[32] = {32,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92};
byte i2c_arr_33[33] = {33,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,11};

void setup() {
  pinMode(led, OUTPUT); 

  
  rd = 0;
  count = 0;
  num = 0;
  
  Wire.begin(0x68);
  Wire.onRequest(requestEvent);
  Wire.onReceive(receiveEvent); 
  Serial.begin(115200);          
  Serial.println(rd);  
}

void loop() {

  if (count > 0) {
    on = !on;
    if (on == false) {
       count --;
     }
  }
  
  digitalWrite(led, on);
  
  delay(100);
}

void requestEvent() {
  
  Serial.print("rq ");  
  Serial.println(rd);  
  
  if (rd == 1) {
    Wire.write("hello there!");
    count = 1;
  } else if (rd == 2) {
    Wire.write("This is a sample of a very long string. I suppose it is about 122 (one hundred and twenty two) bytes length. Amazing!!!!!");
    count = 2;
  } else if (rd == 9) {
    Wire.write(i2c_arr_9,9);  
    count = 3;
  } else if (rd == 12) {
    Wire.write(i2c_arr_12,12);  
    count = 4;    
  } else if (rd == 32) {
    Wire.write(i2c_arr_32,32);  
    count = 5;    
  } else if (rd == 33) {
    Wire.write(i2c_arr_33,33);  
    count = 6;    
  } else {
    num++;
    Wire.write(num);  
    count = 8;
  }
  
}

void receiveEvent(int howMany) {
  Serial.print("rc ");
  Serial.print(howMany);
  Serial.print(" = ");
  int x = Wire.read();    // receive byte as an integer
  Serial.println(x);         // print the integer

  if (x >= 0) {
    rd = x;
    count = x;
  }
  
}

*/
