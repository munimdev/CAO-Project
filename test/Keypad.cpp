#include <Keypad.h>

#define ROW_NUM     4 // four rows
#define COLUMN_NUM  4 // three columns

#define pin1 17
#define pin2 16
#define pin3 4
#define pin4 2
#define pin5 5
#define pin6 18
#define pin7 19
#define pin8 21

char keys[ROW_NUM][COLUMN_NUM] = {
  {'1', '2', '3','='},
  {'4', '5', '6','>'},
  {'7', '8', '9','<'},
  {'*', '0', '#','+'},

};

byte pin_rows[ROW_NUM] = {pin5,pin6,pin7,pin8}; 
byte pin_column[COLUMN_NUM] = {pin1,pin2,pin3,pin4}; 

Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );

const String password = "7890"; // password for mailbox
String input_password;

void setup() {
  Serial.begin(115200);
  input_password.reserve(32); //
}

void loop() {
  char key = keypad.getKey();

  if (key) {
    Serial.println(key);

    if (key == '*') {
      input_password = ""; // clear input password
    } else if (key == '#') {
      if (password == input_password) {
        Serial.println("The password is correct, ACCESS GRANTED!");
        //to do work

      } else {
        Serial.println("The password is incorrect, ACCESS DENIED!");
      }

      input_password = ""; // clear input password
    } else {
      input_password += key; // append new character to input password string
    }
  }
}