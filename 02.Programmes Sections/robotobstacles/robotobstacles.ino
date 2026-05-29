#include <Arduino.h>
#include <Wire.h>
#include <MeOrion.h>
#include "MeRGBLineFollower.h"
#include "Ultrasonic.h"

// -------- Capteurs et Actionneurs --------
MeRGBLineFollower LightSensorRGB_1(PORT_3);
Ultrasonic ultrasonic(7); 
Servo monServo;
#define PIN_SERVO A0 // Configuré sur la broche analogique A0

// -------- Configuration Servo --------
#define SERVO_AVANT 60
#define SERVO_DROITE 0
#define SERVO_GAUCHE 150

// -------- Moteurs --------
#define MOTEUR_A 0x66 // Droite
#define MOTEUR_B 0x68 // Gauche
#define ARRET   0x00
#define AVANT   0x01
#define ARRIERE 0x02

enum EtatRobot {
    SUIVRE_LIGNE_O1,
    MANOEUVRE_EVITEMENT_O1,
    LONGER_LARGEUR_O1,
    CONTOURNER_COIN_O1,
    LONGER_LONGUEUR_O1,
    RETOUR_LIGNE_O1,
    
    SUIVRE_LIGNE_O2,
    MANOEUVRE_EVITEMENT_O2,
    LONGER_LARGEUR_O2,
    CONTOURNER_COIN_O2,
    LONGER_LONGUEUR_O2,
    RETOUR_LIGNE_O2,
    
    FIN_PARCOURS
};
EtatRobot etatActuel = SUIVRE_LIGNE_O1;

// -------- Pilotage Moteurs --------
void piloterMoteur(byte adresse, byte direction, byte vitesse) {
    if (vitesse > 63) vitesse = 63;
    byte commande = (vitesse << 2) | direction;
    Wire.beginTransmission(adresse);
    Wire.write(0x00);
    Wire.write(commande);
    Wire.endTransmission();
}

void toutDroit()     { Serial.println(F("ACTION: Tout Droit")); piloterMoteur(MOTEUR_A, AVANT, 36);   piloterMoteur(MOTEUR_B, ARRIERE, 36); }
void enArriere()     { Serial.println(F("ACTION: Recule Droit")); piloterMoteur(MOTEUR_A, ARRIERE, 36); piloterMoteur(MOTEUR_B, AVANT, 36); }
void tournerDroite() { Serial.println(F("ACTION: Ajustement Droite")); piloterMoteur(MOTEUR_A, AVANT, 5);    piloterMoteur(MOTEUR_B, ARRIERE, 40); }
void tournerGauche() { Serial.println(F("ACTION: Ajustement Gauche")); piloterMoteur(MOTEUR_A, AVANT, 40);   piloterMoteur(MOTEUR_B, ARRIERE, 5); }
void arreter()       { Serial.println(F("ACTION: ARRET")); piloterMoteur(MOTEUR_A, ARRET, 0);    piloterMoteur(MOTEUR_B, ARRET, 0); }

// Marche arrière asservie sur la ligne
void reculerDroite() { Serial.println(F("ACTION: Recule Ajustement Droite")); piloterMoteur(MOTEUR_A, ARRIERE, 5);  piloterMoteur(MOTEUR_B, AVANT, 40); }
void reculerGauche() { Serial.println(F("ACTION: Recule Ajustement Gauche")); piloterMoteur(MOTEUR_A, ARRIERE, 40); piloterMoteur(MOTEUR_B, AVANT, 5); }

void gererSuiviLigne() {
    LightSensorRGB_1.updataAllSensorValue();
    uint8_t pos = LightSensorRGB_1.getPositionState();
    if (pos == 0b1001)                      toutDroit();
    else if (pos == 0b1011 || pos == 0b0111) tournerDroite();
    else if (pos == 0b1101 || pos == 0b1110) tournerGauche();
    else if (pos == 0b1111)                  arreter();
    else                                     toutDroit();
}

// Fonction de suivi de ligne en reculant
void gererSuiviLigneInterne() {
    LightSensorRGB_1.updataAllSensorValue();
    uint8_t pos = LightSensorRGB_1.getPositionState();
    if (pos == 0b1001)                      enArriere();
    else if (pos == 0b1011 || pos == 0b0111) reculerDroite();
    else if (pos == 0b1101 || pos == 0b1110) reculerGauche();
    else if (pos == 0b1111)                  arreter();
    else                                     enArriere();
}

void setup() {
    Wire.begin();
    Serial.begin(9600);
    LightSensorRGB_1.begin();
    monServo.attach(PIN_SERVO);
    
    Serial.println(F("--- DEMARRAGE DU ROBOT ---"));
    Serial.println(F("Initialisation: Servo a 60 degres..."));
    monServo.write(SERVO_AVANT);
    delay(2000); 
    Serial.println(F("Robot Pret !"));
}

void loop() {
    long distance = ultrasonic.read();
    
    // Affichage de la distance
    Serial.print(F("Dist: "));
    Serial.print(distance);
    Serial.print(F(" cm | "));
    
    switch (etatActuel) {
        
        // ==================== OBSTACLE 1 (Par la gauche) ====================
        case SUIVRE_LIGNE_O1:
            Serial.println(F("ETAT: SUIVRE_LIGNE_O1 (Recherche Obstacle 1)"));
            gererSuiviLigne();
            if (distance > 0 && distance <= 10) { 
                Serial.println(F("-> OBSTACLE O1 DETECTE A 3CM !"));
                arreter();
                delay(200);
                
                Serial.println(F("-> Debut de la marche arriere assistee (500ms)..."));
                unsigned long debutRecul = millis();
                while (millis() - debutRecul < 500) {
                    gererSuiviLigneInterne();
                    delay(20);
                }
                arreter();     delay(200);
                
                Serial.println(F("-> Envoi du Servo a 0 degres (regarde a droite)..."));
                monServo.write(SERVO_DROITE);
                delay(600);                 
                
                Serial.println(F("-> Execution du Pivot Initial a Gauche (Vitesse 45, 850ms)..."));
                piloterMoteur(MOTEUR_A, AVANT, 45); 
                piloterMoteur(MOTEUR_B, AVANT, 45); 
                delay(850);                 
                arreter();     delay(400);
                
                Serial.println(F("-> Passage a l'etat: LONGER_LARGEUR_O1"));
                etatActuel = LONGER_LARGEUR_O1;
            }
            break;
            
        case LONGER_LARGEUR_O1:
            Serial.println(F("ETAT: LONGER_LARGEUR_O1 (Avance sur le cote)"));
            toutDroit();
            if (distance > 10) {
                Serial.println(F("-> Paroi perdue (>10cm) : Coin 1 detecte !"));
                Serial.println(F("-> Petit delai de securite lynx pour depasser (350ms)..."));
                delay(600); 
                arreter();
                Serial.println(F("-> Passage a l'etat: CONTOURNER_COIN_O1"));
                etatActuel = CONTOURNER_COIN_O1;
            }
            break;
            
        case CONTOURNER_COIN_O1:
            Serial.println(F("ETAT: CONTOURNER_COIN_O1 (Pivot a droite face a la longueur)"));
            piloterMoteur(MOTEUR_A, ARRIERE, 45);   
            piloterMoteur(MOTEUR_B, ARRIERE, 45); 
            delay(850); 
            arreter();     delay(200);
            Serial.println(F("-> Passage a l'etat: LONGER_LONGUEUR_O1"));
            etatActuel = LONGER_LONGUEUR_O1;
            break;
            
        case LONGER_LONGUEUR_O1:
            Serial.println(F("ETAT: LONGER_LONGUEUR_O1 (Longe la grande longueur)"));
            toutDroit();
            if (distance > 10) {
                Serial.println(F("-> Paroi perdue (>10cm) : Coin 2 fini ! L'obstacle est depasse."));
                Serial.println(F("-> Petit delai de securite (350ms)..."));
                delay(350); 
                arreter();
                
                Serial.println(F("-> Pivot a droite pour amorcer le retour vers la ligne (600ms)..."));
                piloterMoteur(MOTEUR_A, AVANT, 45);   
                piloterMoteur(MOTEUR_B, AVANT, 45); 
                delay(600);
                arreter();
                Serial.println(F("-> Passage a l'etat: RETOUR_LIGNE_O1"));
                etatActuel = RETOUR_LIGNE_O1;
            }
            break;
            
        case RETOUR_LIGNE_O1:
            Serial.println(F("ETAT: RETOUR_LIGNE_O1 (Recherche de la ligne noire...)"));
            toutDroit(); 
            LightSensorRGB_1.updataAllSensorValue();
            
            if (LightSensorRGB_1.getPositionState() != 0b0000 && LightSensorRGB_1.getPositionState() != 0b1111) {
                Serial.println(F("-> LIGNE NOIRE RETROUVEE !"));
                arreter();
                Serial.println(F("-> Remise du servo en position avant (60 degres)..."));
                monServo.write(SERVO_AVANT); 
                delay(300);
                
                Serial.println(F("--- SESSION DE TEST O1 TERMINEE ---"));
                Serial.println(F("-> Mode suivi classique active. Passage a l'etat: FIN_PARCOURS"));
                etatActuel = FIN_PARCOURS; 
            }
            break;

        case FIN_PARCOURS:
            Serial.println(F("ETAT: FIN_PARCOURS (Suivi de ligne classique suite test)"));
            gererSuiviLigne();
            break;
            
    } // Fin du switch
    
    delay(40); 
} // Fin de la loop