/*
 * Example on how to use the Wiegand reader library with interruptions.
 */

#include "Wiegand.h" //https://github.com/paulo-raca/YetAnotherArduinoWiegandLibrary
#include <Wire.h>

// These are the pins connected to the Wiegand D0 and D1 signals.
// Ensure your board supports external Interruptions on these pins
#define PIN_D0 2
#define PIN_D1 3

#define PIN_INT 10
#define PIN_GREENLED 4
#define PIN_BEEP 5
#define PIN_LOCK 9
#define PIN_ONBOARDLED 13

#define CMD_BEEP       (1<<1)
#define CMD_GREENLED   (1<<2)
#define CMD_READER_EN  (1<<3)
#define CMD_OPEN       (1<<4)
#define CMD_DENY       (1<<5)

uint8_t hostCommandToExecute = 0;


// The object that handles the wiegand protocol
Wiegand wiegand;

enum reader_event_type {
  EVENT_CARD = 0,
  EVENT_ERROR = 1,
  EVENT_STATE = 2,
  EVENT_CODE = 3,
  EVENT_DOOR = 4,
  EVENT_NODATA = 0xfe,
};
struct reader_event {
  uint8_t event;
  uint8_t param;
  uint8_t data[4];
};

#define BUFFER_SIZE 11
static struct reader_event reader_buffer[BUFFER_SIZE];
static uint8_t buf_head = 0;
static uint8_t buf_tail = 0;

#define EVENTS_COUNT() ((buf_head - buf_tail + BUFFER_SIZE) % BUFFER_SIZE)

bool readerEnabled = false;

uint8_t keypadData[4] = {0xFF, 0xFF, 0xFF, 0xFF};
uint8_t keypadData_HARDCODED_PASS[4] = {0x00, 0x00, 0x00, 0x00};

bool keypadData_HARDCODED_PASS_CORRECT = false;

uint8_t keypadDataIdx = 0;
uint8_t keypadDataTiks = 0;
#define MAX_KEY_WAIT_TICKS 20



void keypadAddKey(uint8_t key) {
  
  Serial.print(" > key : ");
  Serial.print(key);
  Serial.println();
  if (key == 10) {
    keypadClear();
  } else if (key == 11) {
    keypadFlush();
  } else if (keypadDataIdx < 8) {

    uint8_t k = keypadData[keypadDataIdx/2];

    if (keypadDataIdx % 2 == 0) {
      k = ( k & 0x0f ) | ((key & 0x0F) << 4);
    } else {
      k = ( k & 0xf0 ) | (key & 0x0F);
    }
    keypadData[keypadDataIdx/2] = k;
    keypadDataIdx ++;
    keypadDataTiks = 0;
    
  } else if (keypadDataIdx > 32) {
    keypadClear();
  }

}

void keypadClear() {
  for (int i=0; i<4; i++) {
    keypadData[i] = 0xFF;
  }
  keypadDataIdx = 0;
  Serial.println(" > key clear ");
}

void keypadFlush() {
  Serial.print(" > key buff : ");
  for (int i=0; i<4; i++) {
    Serial.print(keypadData[i] >> 4, 16);
    Serial.print(keypadData[i] & 0xF, 16);
  }
  Serial.println();

  if (memcmp(keypadData, keypadData_HARDCODED_PASS, 4) == 0) {
    keypadData_HARDCODED_PASS_CORRECT = true;
  }

  struct reader_event ev;
  ev.event = EVENT_CODE;
  ev.param = 4;
  ev.data[0] = keypadData[0];
  ev.data[1] = keypadData[1];
  ev.data[2] = keypadData[2];
  ev.data[3] = keypadData[3];  
  event_push(&ev);    

  keypadClear();
  
}

bool event_push(struct reader_event *reader_event)
{
  if (EVENTS_COUNT() == (BUFFER_SIZE - 1))
    return false;

  memcpy(&reader_buffer[buf_head], reader_event, sizeof(*reader_event));
  
  buf_head++;
  buf_head %= BUFFER_SIZE;

  digitalWrite(PIN_INT, 0);
  return true;
}

bool event_pop(struct reader_event *reader_event)
{
  if (buf_head == buf_tail)
    return false;

    memcpy(reader_event, &reader_buffer[buf_tail], sizeof(*reader_event));
    buf_tail++;
    buf_tail %= BUFFER_SIZE;

    return true;
}


void unlock() {
  Serial.println("LOCK OPEN");
  digitalWrite(PIN_LOCK, HIGH);
  digitalWrite(PIN_ONBOARDLED, HIGH);

  for (int i=0; i<4; i++) {
    digitalWrite(PIN_GREENLED, LOW);
    digitalWrite(PIN_BEEP, LOW);
    delay(100);
    digitalWrite(PIN_GREENLED, HIGH);
    digitalWrite(PIN_BEEP, HIGH);
    delay(100);
  }

  digitalWrite(PIN_LOCK, LOW);
  digitalWrite(PIN_ONBOARDLED, LOW);
}

void accessDeny() {
  digitalWrite(PIN_BEEP, LOW);
  delay(1000);
  digitalWrite(PIN_BEEP, HIGH);
}

void accessGrant() {
  unlock();
}


void enableReader(bool enable) {

  //TODO: create additional pin for power on-off 
  
  if (readerEnabled == true && enable == false) {
    //disable
    wiegand.flushNow();
    wiegand.end();
    detachInterrupt(digitalPinToInterrupt(PIN_D0));
    detachInterrupt(digitalPinToInterrupt(PIN_D1));
    readerEnabled = false;
    Serial.println("READER DISABLED");
  }
  
  if (readerEnabled == false && enable == true) {
    //enable

    //Install listeners and initialize Wiegand reader
    wiegand.onReceive(receivedData, "Card readed: ");
    wiegand.onReceiveError(receivedDataError, "Card read error: ");
    wiegand.onStateChange(stateChanged, "State changed: ");
    wiegand.begin(Wiegand::LENGTH_ANY, true);
  
    //initialize pins as INPUT and attaches interruptions
    pinMode(PIN_D0, INPUT);
    pinMode(PIN_D1, INPUT);
    attachInterrupt(digitalPinToInterrupt(PIN_D0), pinStateChanged, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_D1), pinStateChanged, CHANGE);

    readerEnabled = true;
    //Sends the initial pin state to the Wiegand library
    pinStateChanged();
    Serial.println("READER ENABLED");
  }
  
}

// Initialize Wiegand reader
void setup() {
  Serial.begin(9600);


  Wire.begin(0x06);
  Wire.onRequest(i2c_request_cb);
  Wire.onReceive(i2c_receive_cb);

  digitalWrite(PIN_INT, HIGH);
  pinMode(PIN_INT, OUTPUT);

  digitalWrite(PIN_BEEP, HIGH);
  pinMode(PIN_BEEP, OUTPUT);
  
  digitalWrite(PIN_GREENLED, HIGH);
  pinMode(PIN_GREENLED, OUTPUT);

  digitalWrite(PIN_LOCK, LOW);
  pinMode(PIN_LOCK, OUTPUT);

  digitalWrite(PIN_ONBOARDLED, LOW);
  pinMode(PIN_ONBOARDLED, OUTPUT);
  
  
//  TODO COMMENT
//  delay(200);
//  accessDeny();
//  delay(200);
//  accessGrant();

  delay(100);
  
  //Initialize Wiegand reader and

  enableReader(true);
  
}

// Every few milliseconds, check for pending messages on the wiegand reader
// This executes with interruptions disabled, since the Wiegand library is not thread-safe
void loop() {
  noInterrupts();
  wiegand.flush();
  interrupts();
  //Sleep a little -- this doesn't have to run very often.

  if (keypadData_HARDCODED_PASS_CORRECT) {
    keypadData_HARDCODED_PASS_CORRECT = false;
    delay(100);
    Serial.print(" > key HARDCODED YAY !!! ");
    delay(100);
    accessGrant();
    delay(200);
  }

  if (keypadDataIdx != 0 && keypadDataTiks > MAX_KEY_WAIT_TICKS) {
    keypadClear();
  } else {
    keypadDataTiks ++;
  }

  if (hostCommandToExecute) {
    digitalWrite(PIN_BEEP, hostCommandToExecute & CMD_BEEP);
    digitalWrite(PIN_GREENLED, hostCommandToExecute & CMD_GREENLED);

    bool reader_enable = hostCommandToExecute & CMD_READER_EN;
    enableReader(reader_enable);
    
    if (hostCommandToExecute & CMD_OPEN) {
       accessGrant();
    }
   
    if (hostCommandToExecute & CMD_DENY) {
       accessDeny();
    }
       
   hostCommandToExecute = 0; 
  }


  delay(100);
}

void i2c_receive_cb(int)
{
  uint8_t cmd;

  while (Wire.available()) {
    cmd = Wire.read();
    if (cmd) {
      hostCommandToExecute = 0x01 | cmd;
    }
  }
}

void i2c_request_cb()
{
  struct reader_event ev;

//  Serial.println("!");
//  Serial.flush();

  if (!event_pop(&ev)) {
    Wire.write(EVENT_NODATA);
    return;
  }
    
/*
  ev.event = EVENT_ID;
  ev.param = 4;
  ev.data[0] = 0xDE;
  ev.data[1] = 0xAD;
  ev.data[2] = 0xBE;
  ev.data[3] = 0xEF;
*/
  Wire.write((const char *)&ev, sizeof(ev));
  if (EVENTS_COUNT() == 0) {
    digitalWrite(PIN_INT, 1);
  }
}

// When any of the pins have changed, update the state of the wiegand library
void pinStateChanged() {
  wiegand.setPin0State(digitalRead(PIN_D0));
  wiegand.setPin1State(digitalRead(PIN_D1));
}

// Notifies when a reader has been connected or disconnected.
// Instead of a message, the seconds parameter can be anything you want -- Whatever you specify on `wiegand.onStateChange()`
void stateChanged(bool plugged, const char* message) {
    Serial.print(message);
    Serial.println(plugged ? "CONNECTED" : "DISCONNECTED");
}

// Notifies when a card was read.
// Instead of a message, the seconds parameter can be anything you want -- Whatever you specify on `wiegand.onReceive()`
void receivedData(uint8_t* data, uint8_t bits, const char* message) {

    Serial.print(message);
    Serial.print(bits);
    Serial.print(" bits / data: ");
    //Print value in HEX
    uint8_t bytes = (bits+7)/8;
    for (int i=0; i<bytes; i++) {
        Serial.print(data[i] >> 4, 16);
        Serial.print(data[i] & 0xF, 16);
    }
    Serial.println();

    if (bits == 4) {
      //keypad code enter
      keypadAddKey(data[0]);
      return;
    }

    struct reader_event ev;

    ev.event = EVENT_CARD;
    ev.param = bits;
    
    for (int i = 0; i < bytes; i++) {
      ev.data[i] = data[i];
    }

    event_push(&ev);    
    
}

// Notifies when an invalid transmission is detected
void receivedDataError(Wiegand::DataError error, uint8_t* rawData, uint8_t rawBits, const char* message) {
    Serial.print(message);
    Serial.print(Wiegand::DataErrorStr(error));
    Serial.print(" - Raw data: ");
    Serial.print(rawBits);
    Serial.print("bits / ");

    //Print value in HEX
    uint8_t bytes = (rawBits+7)/8;
    for (int i=0; i<bytes; i++) {
        Serial.print(rawData[i] >> 4, 16);
        Serial.print(rawData[i] & 0xF, 16);
    }
    Serial.println();
}
