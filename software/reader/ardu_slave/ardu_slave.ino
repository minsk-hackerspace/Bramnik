// Wire Slave Sender
// by Nicholas Zambetti <http://www.zambetti.com>

// Demonstrates use of the Wire library
// Sends data as an I2C/TWI slave device
// Refer to the "Wire Master Reader" example for use with this

// Created 29 March 2006

// This example code is in the public domain.


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
  Serial.begin(9600);          
  
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



