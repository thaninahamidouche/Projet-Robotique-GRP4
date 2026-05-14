#include <Wire.h>
#include "Servo.h"

int angle = 50;
Servo servo;


void setup() {
 Serial.begin(9600);
 Wire.begin();
 
 servo.attach(A0);
}



void test_servo()
{
  servo.write(angle);
  angle=angle-35;
  delay(500);
  servo.write(angle);
  angle=angle+35;
  delay(500);
}



void loop() {
 test_servo();

}
