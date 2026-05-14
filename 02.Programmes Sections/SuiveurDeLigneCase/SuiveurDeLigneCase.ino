#include <Arduino.h>
#include <Wire.h>
#include <MeOrion.h>
#include "MeRGBLineFollower.h"

// -------- Capteur --------
MeRGBLineFollower LightSensorRGB_1(PORT_3);

// -------- Moteurs --------
// A = DROITE
// B = GAUCHE
#define MOTEUR_A 0x66
#define MOTEUR_B 0x68

#define ARRET   0x00
#define AVANT   0x01
#define ARRIERE 0x02

// -------- Pilotage --------
void piloterMoteur(byte adresse, byte direction, byte vitesse) {
  if (vitesse > 63) vitesse = 63;

  byte commande = (vitesse << 2) | direction;

  Wire.beginTransmission(adresse);
  Wire.write(0x00);
  Wire.write(commande);
  Wire.endTransmission();
}

// -------- Mouvements --------
void toutDroit() {
  piloterMoteur(MOTEUR_A, ARRIERE, 36); // droite
  piloterMoteur(MOTEUR_B, AVANT, 36);   // gauche
}

void tournerDroite() {
  // on ralentit la roue droite (A)
  piloterMoteur(MOTEUR_A, ARRIERE, 18);
  piloterMoteur(MOTEUR_B, AVANT, 40);
}

void tournerGauche() {
  // on ralentit la roue gauche (B)
  piloterMoteur(MOTEUR_A, ARRIERE, 40);
  piloterMoteur(MOTEUR_B, AVANT, 18);
}

void arreter() {
  piloterMoteur(MOTEUR_A, ARRET, 0);
  piloterMoteur(MOTEUR_B, ARRET, 0);
}

// -------- Setup --------
void setup() {
  Wire.begin();
  Serial.begin(115200);
  LightSensorRGB_1.begin();
  delay(1000);
}

// -------- Loop --------
void loop() {

  LightSensorRGB_1.updataAllSensorValue();
  uint8_t pos = LightSensorRGB_1.getPositionState();

  Serial.println(pos, BIN);

  switch (pos) {

    case 0b1001:
      toutDroit();
      break;

    case 0b1100:
    case 0b1000:
      tournerDroite();
      break;

    case 0b0011:
    case 0b0001:
      tournerGauche();
      break;

    case 0b1111:
      arreter();
      break;

    default:
      toutDroit();
      break;
  }

  delay(20);
}
