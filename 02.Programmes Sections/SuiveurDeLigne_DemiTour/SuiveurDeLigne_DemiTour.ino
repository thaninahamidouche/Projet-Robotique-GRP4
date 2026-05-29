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
    piloterMoteur(MOTEUR_A, AVANT, 40);
    piloterMoteur(MOTEUR_B, ARRIERE, 40);
    Serial.println("TOUT DROIT.");
}

void tournerDroite() {
    piloterMoteur(MOTEUR_A, AVANT, 5);
    piloterMoteur(MOTEUR_B, ARRIERE, 40);
    Serial.println("DROITE.");
}

void tournerGauche() {
    piloterMoteur(MOTEUR_A, AVANT, 40);
    piloterMoteur(MOTEUR_B, ARRIERE, 5);
    Serial.println("GAUCHE.");
}

void arreter() {
    piloterMoteur(MOTEUR_A, ARRET, 0);
    piloterMoteur(MOTEUR_B, ARRET, 0);
    Serial.println("ARRET.");
}

// -------- Rotation sur place (demi-tour) --------
// Roue droite recule, roue gauche avance → tourne vers la GAUCHE sur place
void rotationGauche() {
    piloterMoteur(MOTEUR_A, ARRIERE, 35);
    piloterMoteur(MOTEUR_B, ARRIERE, 35);
}

// -------- Détection de ligne --------
bool ligneDetectee(uint8_t pos) {
    // La ligne est considérée trouvée si au moins un capteur central la voit
    // pos 0b1001 = ligne bien centrée
    // tout autre valeur != 0b0000 signifie qu'au moins un capteur voit la ligne
    return (pos != 0b0000);
}

// -------- Setup --------
void setup() {
    Wire.begin();
    Serial.begin(9600);
    LightSensorRGB_1.begin();
    delay(1000);
    Serial.println("Démarrage...");
}

// -------- Boucle principale --------
void loop() {
    LightSensorRGB_1.updataAllSensorValue();
    uint8_t pos = LightSensorRGB_1.getPositionState();

    Serial.print("pos = ");
    Serial.println(pos, BIN);

    // --- CAS 1 : Ligne perdue (aucun capteur ne voit la ligne) ---
    if (pos == 0b0000) {
        Serial.println("LIGNE PERDUE → DEMI-TOUR");

        // On tourne sur place jusqu'à retrouver la ligne
        while (true) {
            rotationGauche();
            delay(30);

            LightSensorRGB_1.updataAllSensorValue();
            pos = LightSensorRGB_1.getPositionState();

            Serial.print("Recherche... pos = ");
            Serial.println(pos, BIN);

            if (ligneDetectee(pos)) {
                Serial.println("LIGNE RETROUVÉE !");
                arreter();
                delay(200); // petite pause avant de reprendre
                break;      // on sort du demi-tour, on reprend le suivi
            }
        }
        return; // on repart depuis le début du loop()
    }

    // --- CAS 2 : Ligne trouvée → suivi normal ---
    if (pos == 0b1001) {
        toutDroit();
    }
    else if (pos == 0b1011 || pos == 0b0111) {
        tournerDroite();
    }
    else if (pos == 0b1101 || pos == 0b1110) {
        tournerGauche();
    }
    else if (pos == 0b1111) {
        // Ligne de fin de section (tous capteurs sur ligne)
        arreter();
    }
    else {
        // Cas intermédiaire inconnu → on continue tout droit
        toutDroit();
    }

    delay(20);
}
