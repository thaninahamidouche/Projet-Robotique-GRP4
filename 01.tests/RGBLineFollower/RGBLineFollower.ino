#include <Arduino.h>
#include <MeOrion.h>
#include "MeRGBLineFollower.h"

MeRGBLineFollower LightSensorRGB_1(PORT_3);

void setup() {
  Serial.begin(9600); 
  LightSensorRGB_1.begin();
  Serial.println("Initialisation terminee");
}

void loop() {
  LightSensorRGB_1.updataAllSensorValue(); 

  uint8_t pos = LightSensorRGB_1.getPositionState();

  Serial.print("Position : ");
  Serial.print(pos);
  Serial.print("  |  Binaire : [");
  Serial.print((pos >> 3) & 1);
  Serial.print(" ");
  Serial.print((pos >> 2) & 1);
  Serial.print(" ");
  Serial.print((pos >> 1) & 1);
  Serial.print(" ");
  Serial.print((pos >> 0) & 1);
  Serial.println("]");
  
  delay(200);
}
