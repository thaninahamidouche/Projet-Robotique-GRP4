#include <Arduino.h>
#include <Wire.h>
#include <MeOrion.h>
#include "MeRGBLineFollower.h"

// -------- Capteur --------
MeRGBLineFollower LightSensorRGB_1(PORT_3);

// -------- Moteurs --------
// A = roue DROITE
// B = roue GAUCHE
#define MOTEUR_A 0x66
#define MOTEUR_B 0x68

#define ARRET   0x00
#define AVANT   0x01
#define ARRIERE 0x02

// -------- Pilotage bas niveau --------
void piloterMoteur(byte adresse, byte direction, byte vitesse) {
    if (vitesse > 63) vitesse = 63;

    byte commande = (vitesse << 2) | direction;

    Wire.beginTransmission(adresse);
    Wire.write(0x00);
    Wire.write(commande);
    Wire.endTransmission();
}

// -------- Déplacements --------
void toutDroit() {
    // A (droite)
    piloterMoteur(MOTEUR_A, AVANT, 36);

    // B (gauche)
    piloterMoteur(MOTEUR_B, ARRIERE, 36);
    Serial.println("TOUT DROIT.");
    //delay(5000);
}

void tournerDroite() {
    // ralentir roue droite (A)
    piloterMoteur(MOTEUR_A, AVANT, 5);

    // roue gauche plus rapide (B)
    piloterMoteur(MOTEUR_B, ARRIERE, 40);
    Serial.println(" DROITE.");
    //delay(5000);
}

void tournerGauche() {
    // roue droite plus rapide (A)
    piloterMoteur(MOTEUR_A, AVANT, 40);

    // ralentir roue gauche (B)
    piloterMoteur(MOTEUR_B, ARRIERE, 5);
    Serial.println(" GAUCHE.");
    //delay(5000);
}

void arreter() {
    piloterMoteur(MOTEUR_A, ARRET, 0);
    piloterMoteur(MOTEUR_B, ARRET, 0);
    Serial.println(" ARRET.");
    //delay(5000);
}

// -------- Setup --------
void setup() {
    Wire.begin();
    Serial.begin(9600);
    LightSensorRGB_1.begin();
    delay(1000);
}

// -------- Boucle principale --------
void loop() {
    LightSensorRGB_1.updataAllSensorValue();
    uint8_t pos = LightSensorRGB_1.getPositionState();

    Serial.println(pos, BIN);

    if (pos == 0b1001) {
        toutDroit();
    }
    else if (pos == 0b1011 || pos == 0b0111 || pos == 0b1000 || pos == 0b1100) {
        // ligne à droite
        tournerDroite();
    }
    else if (pos == 0b1101 || pos == 0b1110 || pos == 0b0001 || pos == 0b0011) {
        // ligne à gauche
        tournerGauche();
    }
    else if (pos == 0b1111) {
        arreter();
    }
    else {
        toutDroit();
    }

    delay(20);
}
// test de connexion github