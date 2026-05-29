#include <Arduino.h>
#include <Wire.h>
#include <MeOrion.h>
#include "MeRGBLineFollower.h"

// -------- Capteur --------
MeRGBLineFollower LightSensorRGB_1(PORT_3);

// -------- Moteurs --------
#define MOTEUR_A 0x66
#define MOTEUR_B 0x68

#define ARRET   0x00
#define AVANT   0x01
#define ARRIERE 0x02

// -------- Variable d'état --------
int etat = 0;  // 0 = en attente, 1 = en route

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
    piloterMoteur(MOTEUR_A, AVANT, 50);
    piloterMoteur(MOTEUR_B, ARRIERE, 50);
    Serial.println("TOUT DROIT.");
}

void tournerDroite() {
    piloterMoteur(MOTEUR_A, AVANT, 17);
    piloterMoteur(MOTEUR_B, ARRIERE, 50);
    Serial.println("DROITE FORT.");
}

void tournerDroiteL() {
    piloterMoteur(MOTEUR_A, AVANT, 45);
    piloterMoteur(MOTEUR_B, ARRIERE, 50);
    Serial.println("DROITE LEGER.");
}

void tournerGauche() {
    piloterMoteur(MOTEUR_A, AVANT, 50);
    piloterMoteur(MOTEUR_B, ARRIERE, 17);
    Serial.println("GAUCHE FORT.");
}

void tournerGaucheL() {
    piloterMoteur(MOTEUR_A, AVANT, 50);
    piloterMoteur(MOTEUR_B, ARRIERE, 45);
    Serial.println("GAUCHE LEGER.");
}

void arreter() {
    piloterMoteur(MOTEUR_A, ARRET, 0);
    piloterMoteur(MOTEUR_B, ARRET, 0);
    Serial.println("ARRET.");
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

    // -------- Gestion ligne de départ/arrivée --------
    if (pos == 0b0000) {
        if (etat == 0) {
            Serial.println("DEPART !");
            etat = 1;
            toutDroit();  // on passe la ligne et on continue
        }
        else if (etat == 1) {
            Serial.println("ARRIVEE !");
            arreter();
        }
    }

    // -------- Suivi de ligne (seulement si en route) --------
    else if (etat == 1) {
        if (pos == 0b1001) {
            toutDroit();
        }
        else if (pos == 0b0111 || pos == 0b1000) {
            tournerDroite();
        }
        else if (pos == 0b1011 || pos == 0b1100) {
            tournerDroiteL();
        }
        else if (pos == 0b1110 || pos == 0b0001) {
            tournerGauche();
        }
        else if (pos == 0b1101 || pos == 0b0011) {
            tournerGaucheL();
        }
        else {
            toutDroit();
        }
    }

    delay(20);
}