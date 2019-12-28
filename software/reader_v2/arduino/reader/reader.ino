/*
 * Example on how to use the Wiegand reader library with interruptions.
 */

#include <Wiegand.h>
#include <Wire.h>

// These are the pins connected to the Wiegand D0 and D1 signals.
// Ensure your board supports external Interruptions on these pins
#define PIN_D0 2
#define PIN_D1 3

#define PIN_INT 10
#define PIN_GREENLED 5
#define PIN_BEEP 4

#define CMD_BEEP      (1<<0)
#define CMD_GREENLED  (1<<1)

// The object that handles the wiegand protocol
Wiegand wiegand;

enum reader_event_type {
  EVENT_ID = 0,
  EVENT_ERROR = 1,
  EVENT_STATE = 2,
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

bool event_push(struct reader_event *reader_event)
{
  if (EVENTS_COUNT() == (BUFFER_SIZE - 1))
    return false;

  memcpy(&reader_buffer[buf_head], reader_event, sizeof(*reader_event));
  
  buf_head++;
  buf_head %= BUFFER_SIZE;
    
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

// Initialize Wiegand reader
void setup() {
  Serial.begin(9600);

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

  Wire.begin(0x06);
  Wire.onRequest(i2c_request_cb);
  Wire.onReceive(i2c_receive_cb);

  digitalWrite(PIN_INT, HIGH);
  pinMode(PIN_INT, OUTPUT);

  digitalWrite(PIN_BEEP, HIGH);
  pinMode(PIN_BEEP, OUTPUT);
  
  digitalWrite(PIN_GREENLED, HIGH);
  pinMode(PIN_GREENLED, OUTPUT);
  
  //Sends the initial pin state to the Wiegand library
  pinStateChanged();
}

// Every few milliseconds, check for pending messages on the wiegand reader
// This executes with interruptions disabled, since the Wiegand library is not thread-safe
void loop() {
  noInterrupts();
  wiegand.flush();
  interrupts();
  //Sleep a little -- this doesn't have to run very often.
  delay(100);
}

void i2c_receive_cb(int)
{
  uint8_t cmd;

  while (Wire.available()) {
    cmd = Wire.read();
    digitalWrite(PIN_BEEP, cmd & CMD_BEEP);
    digitalWrite(PIN_GREENLED, cmd & CMD_GREENLED);
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
  if (EVENTS_COUNT() == 0)
    digitalWrite(PIN_INT, 1);
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
    struct reader_event ev;

    ev.event = EVENT_ID;
    ev.param = bits;
    uint8_t bytes = (bits+7)/8;
    for (int i = 0; i < bytes; i++) {
      ev.data[i] = data[i];
    }

    event_push(&ev);
    digitalWrite(PIN_INT, 0);
    
    Serial.print(message);
    Serial.print(bits);
    Serial.print("bits / ");
    //Print value in HEX
    
    for (int i=0; i<bytes; i++) {
        Serial.print(data[i] >> 4, 16);
        Serial.print(data[i] & 0xF, 16);
    }
    Serial.println();
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
