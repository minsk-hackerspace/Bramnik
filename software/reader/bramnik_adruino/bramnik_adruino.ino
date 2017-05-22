
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
  
    if (customKey){
      
      if (customKey == '*') {
        firstSection();
      }
      if (customKey == '#') {
        mus1();
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
  
          Serial.println(time_passed);
          Serial.println(micros()-last_read_time);
      }
      else{
          tone(pin_sound, NOTE_A4, 20);
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

    Serial.println("requestEvent");
    uint8_t status;
    unsigned long time_since_last_read;

    Serial.print("cmd_type: ");
    Serial.println(cmd_type);

    Serial.print("cmd: ");
    Serial.println(cmd);

    switch(cmd_type) {
        case CMD_ENABLE:
            Serial.print("CMD_ASK");
            Wire.write("hello there!");
            //cmd
            //KEYPAD_MASK=0x02;
            //NFC_MASK=0x01;
            break;
        case CMD_PLAY:
            Serial.print("CMD_PLAY ");
            Wire.write("hello there!");
            break;
        case CMD_ASK:
            Serial.print("CMD_ASK");
            switch(cmd){
                case STATUS:
                    Serial.print("ask: STATUS ");
                    time_since_last_read = micros() - last_read_time;
                    status = time_since_last_read < NFC_data_timeout ? 1 : 0;
                    Serial.println(status);
                    Wire.write(status);

                break;
                case NFC_DATA:
                    Serial.print("ask: NFC_DATA ");
                    Wire.write(last_read_uid, 7);  
                break;
            };
            break;
    };

  
}

void receiveEvent(int howMany) {

    if (howMany == 0) {
      Serial.println("requestEvent coming ");  
      return;   
    }
    
    Serial.print("receiveEvent ");
    Serial.println(howMany);

    unsigned int x = Wire.read();    // receive byte as an integer
    cmd_type = x & 0xf0;
    cmd = x & 0x0f;

    Serial.print("cmd_type = ");
    Serial.println(cmd_type);

    Serial.print("cmd = ");
    Serial.println(cmd);

    switch(cmd_type){
        case CMD_ENABLE:
            Serial.print("CMD Enable received: ");
            Serial.println(cmd);
            break;
        case CMD_PLAY:
            Serial.print("CMD Play received: ");
            Serial.println(cmd);
            break;
        case CMD_ASK:
            Serial.print("CMD ASK received: ");
            Serial.println(cmd);
            break;
    };
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

//beep only for plaing music
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
  NOTE_G4,NOTE_G4,NO_SOUND,NOTE_G4,NOTE_G4,NO_SOUND,NOTE_G4,NOTE_G4,NOTE_G4,NOTE_G4,NOTE_G4,
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
