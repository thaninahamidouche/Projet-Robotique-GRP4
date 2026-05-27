// ============================================================
//  ROBOT COMPLET - Machine d'états
//  ATTENTE_DEPART -> SUIVI_L1 -> TUNNEL -> RECUP_L2
//  -> SUIVI_L2 -> EVITEMENT_O1 -> SUIVI_L3
//  -> EVITEMENT_O2 -> SUIVI_RAMPE
//  -> DETECTION_COULEUR -> AFFICHAGE_LEDS
//  -> DEMI_TOUR -> SUIVI_RETOUR -> FIN
// ============================================================

#include <Arduino.h>
#include <Wire.h>
#include <MeOrion.h>
#include "MeRGBLineFollower.h"
#include <Adafruit_NeoPixel.h>
#include "utility/Servo.h"

// ============================================================
// MACHINE D'ETATS
// ============================================================
enum EtatRobot {
  ATTENTE_DEPART,
  SUIVI_L1,
  TUNNEL,
  RECUP_L2,
  SUIVI_L2,
  EVITEMENT_O1,
  SUIVI_L3,
  EVITEMENT_O2,
  SUIVI_RAMPE,
  DETECTION_COULEUR,
  AFFICHAGE_LEDS,
  DEMI_TOUR,
  SUIVI_RETOUR,
  FIN_PARCOURS
};
EtatRobot etat = ATTENTE_DEPART;

// ============================================================
// CAPTEURS
// ============================================================
MeRGBLineFollower LightSensorRGB_1(PORT_3);

// SERVO
Servo radar;
const int PIN_SERVO        = A0;
const int ANGLE_CENTRE     = 95;
const int ANGLE_GAUCHE_TUN = 180;
const int ANGLE_DROITE_O1  = 5;
const int ANGLE_GAUCHE_O2  = 180;

// ULTRASON
const int sigPin = 7;

// LED
#define LED_PIN  6
#define NB_LEDS  30
Adafruit_NeoPixel strip(NB_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// CAPTEUR COULEUR
#define COLOR_ADDR 0x29

// ============================================================
// MOTEURS
// ============================================================
#define MOTEUR_A 0x66
#define MOTEUR_B 0x68
#define ARRET    0x00
#define AVANT    0x01
#define ARRIERE  0x02

void piloterMoteur(byte adresse, byte direction, byte vitesse) {
  vitesse = constrain(vitesse, 0, 63);
  byte commande = (vitesse << 2) | direction;
  Wire.beginTransmission(adresse);
  Wire.write(0x00);
  Wire.write(commande);
  Wire.endTransmission();
}

// ============================================================
// MOUVEMENTS DE BASE
// ============================================================
void avancer(int v)              { piloterMoteur(MOTEUR_A, AVANT,    v);  piloterMoteur(MOTEUR_B, ARRIERE, v); }
void tournerGauche(int v)        { piloterMoteur(MOTEUR_A, AVANT,    v);  piloterMoteur(MOTEUR_B, ARRIERE, v/3); }
void tournerDroite(int v)        { piloterMoteur(MOTEUR_A, AVANT,    v/3);piloterMoteur(MOTEUR_B, ARRIERE, v); }
void arreter()                   { piloterMoteur(MOTEUR_A, ARRET,    0);  piloterMoteur(MOTEUR_B, ARRET,   0); }
void tournerGaucheSurPlace(int v){ piloterMoteur(MOTEUR_A, AVANT,    v);  piloterMoteur(MOTEUR_B, AVANT,   v); }
void tournerDroiteSurPlace(int v){ piloterMoteur(MOTEUR_A, ARRIERE,  v);  piloterMoteur(MOTEUR_B, ARRIERE, v); }
void rotationGauche()            { piloterMoteur(MOTEUR_A, AVANT,   35);  piloterMoteur(MOTEUR_B, AVANT,  35); }
void chercherLigneGauche()       { piloterMoteur(MOTEUR_A, AVANT,   25);  piloterMoteur(MOTEUR_B, AVANT,  15); }
void avancerTunnel(int vd,int vg){ piloterMoteur(MOTEUR_A, AVANT, constrain(vd,0,63)); piloterMoteur(MOTEUR_B, ARRIERE, constrain(vg,0,63)); }

// Mouvements fins pour suiveurL1 et tunnel
void toutDroit()          { piloterMoteur(MOTEUR_A, AVANT, 30); piloterMoteur(MOTEUR_B, ARRIERE, 30); }
void tournerDroiteLeger() { piloterMoteur(MOTEUR_A, AVANT, 20); piloterMoteur(MOTEUR_B, ARRIERE, 35); }
void tournerDroiteFort()  { piloterMoteur(MOTEUR_A, AVANT, 15); piloterMoteur(MOTEUR_B, ARRIERE, 40); }
void tournerGaucheLeger() { piloterMoteur(MOTEUR_A, AVANT, 35); piloterMoteur(MOTEUR_B, ARRIERE, 20); }
void tournerGaucheFort()  { piloterMoteur(MOTEUR_A, AVANT, 40); piloterMoteur(MOTEUR_B, ARRIERE, 15); }

// ============================================================
// ULTRASON
// ============================================================
float mesurerDistance() {
  pinMode(sigPin, OUTPUT);
  digitalWrite(sigPin, LOW);  delayMicroseconds(2);
  digitalWrite(sigPin, HIGH); delayMicroseconds(10);
  digitalWrite(sigPin, LOW);
  pinMode(sigPin, INPUT);
  long duree = pulseIn(sigPin, HIGH, 30000);
  if (duree == 0) return 400;
  return duree * 0.034 / 2.0;
}

// Version filtrée pour le tunnel
long derniereDistanceTunnel = 999;
bool murDejaVu = false;

long distanceFiltreeTunnel() {
  long somme = 0; int n = 0;
  for (int i = 0; i < 3; i++) {
    long d = (long)mesurerDistance();
    if (d != 999 && d > 2 && d < 120) { somme += d; n++; }
    delay(5);
  }
  if (n == 0) return murDejaVu ? derniereDistanceTunnel : 999;
  derniereDistanceTunnel = somme / n;
  murDejaVu = true;
  return derniereDistanceTunnel;
}

// ============================================================
// CAPTEUR LIGNE
// ============================================================
uint8_t lireLigne() {
  LightSensorRGB_1.updataAllSensorValue();
  return LightSensorRGB_1.getPositionState();
}

bool ligneVue(uint8_t pos) {
  return pos == 0b1001 || pos == 0b1011 || pos == 0b0111 ||
         pos == 0b1101 || pos == 0b1110 || pos == 0b0000;
}

bool ligneCentree(uint8_t pos) {
  return pos == 0b1001;
}

// ============================================================
// VARIABLES COMMUNES
// ============================================================
int           derniereDirection = 0;
unsigned long debutNoir         = 0;
unsigned long debutBlanc        = 0;

void resetNoir()  { debutNoir  = 0; }
void resetBlanc() { debutBlanc = 0; }

void recupererLigne() {
  if      (derniereDirection ==  1) tournerDroiteFort();
  else if (derniereDirection == -1) tournerGaucheFort();
  else avancer(20);
}
void activerMoteurFin() {
  piloterMoteur(0x65, ARRIERE, 63);
}

// ============================================================
// SUIVEUR L1 - CODE TUNNEL (inchangé)
// Uniquement pour SUIVI_L1 et RECUP_L2
// 1111 long -> tunnel détecté
// ============================================================
bool tunnelFait    = false;
bool tunnelDetecte = false;
const unsigned long TEMPS_AVANT_TUNNEL = 700;

void suiveurL1(uint8_t pos) {
  switch (pos) {
    case 0b1001: resetNoir(); resetBlanc(); toutDroit();          derniereDirection =  0; break;
    case 0b1011: resetNoir(); resetBlanc(); tournerDroiteLeger(); derniereDirection =  1; break;
    case 0b0111: resetNoir(); resetBlanc(); tournerDroiteFort();  derniereDirection =  1; break;
    case 0b1101: resetNoir(); resetBlanc(); tournerGaucheLeger(); derniereDirection = -1; break;
    case 0b1110: resetNoir(); resetBlanc(); tournerGaucheFort();  derniereDirection = -1; break;
    case 0b0001:
    case 0b1000: resetNoir(); resetBlanc(); toutDroit(); derniereDirection = 0; break;
    case 0b0011: resetNoir(); resetBlanc(); tournerDroiteFort(); derniereDirection =  1; break;
    case 0b1100: resetNoir(); resetBlanc(); tournerGaucheFort(); derniereDirection = -1; break;
    case 0b0101:
    case 0b1010:
    case 0b0110: resetNoir(); resetBlanc(); toutDroit(); break;
    case 0b0000:
      resetBlanc();
      toutDroit(); derniereDirection = 0;
      if (debutNoir == 0) debutNoir = millis();
      break;
    case 0b1111:
      resetNoir();
      if (debutBlanc == 0) debutBlanc = millis();
      if (millis() - debutBlanc < TEMPS_AVANT_TUNNEL) {
        recupererLigne();
      } else {
        if (!tunnelFait) tunnelDetecte = true;
        else recupererLigne();
      }
      break;
    default: resetNoir(); resetBlanc(); toutDroit(); break;
  }
}

// ============================================================
// SUIVEUR PID
// Utilisé pour SUIVI_L2, SUIVI_L3, SUIVI_RAMPE, SUIVI_RETOUR
// 1111 = récupération simple, 0000 long = arrêt si SUIVI_RETOUR
// ============================================================
bool arreterSur0000 = false;

// Paramètres PID - ajuster sur le terrain
float Kp = 8.0;           // baisser si oscillations (ex: 5.0)
float Kd = 3.0;           // augmenter pour amortir (ex: 5.0)
const int VITESSE_BASE = 28;

float erreurPrecedente = 0;

// Erreur de position : négatif = trop à droite, positif = trop à gauche
float positionErreur(uint8_t pos) {
  switch (pos) {
    case 0b1001: return  0.0;
    case 0b1101: return -1.0;
    case 0b1110: return -2.0;
    case 0b1100: return -3.0;
    case 0b1011: return  1.0;
    case 0b0111: return  2.0;
    case 0b0011: return  3.0;
    case 0b0001: return  3.0;
    case 0b1000: return -3.0;
    case 0b0101:
    case 0b1010:
    case 0b0110: return  0.0;
    default:     return  0.0;
  }
}

void suiveurPID(uint8_t pos) {

  // Ligne pleine noire
  if (pos == 0b0000) {
    if (arreterSur0000) {
      if (debutNoir == 0) debutNoir = millis();
      if (millis() - debutNoir > 100) {
        Serial.println(F("0000 -> FIN"));
        arreter();
        etat = FIN_PARCOURS;
      } else {
        toutDroit();
      }
    } else {
      resetNoir(); toutDroit(); derniereDirection = 0;
    }
    return;
  }

  // Ligne perdue
  if (pos == 0b1111) {
    resetNoir(); recupererLigne(); return;
  }

  resetNoir();

  // Calcul PID
  float erreur     = positionErreur(pos);
  float derivee    = erreur - erreurPrecedente;
  float correction = Kp * erreur + Kd * derivee;
  erreurPrecedente = erreur;

  if      (erreur > 0) derniereDirection =  1;
  else if (erreur < 0) derniereDirection = -1;
  else                 derniereDirection =  0;

  int vitD = constrain((int)(VITESSE_BASE - correction), 0, 63);
  int vitG = constrain((int)(VITESSE_BASE + correction), 0, 63);

  piloterMoteur(MOTEUR_A, AVANT,   vitD);
  piloterMoteur(MOTEUR_B, ARRIERE, vitG);
}

// ============================================================
// TUNNEL (inchangé)
// ============================================================
unsigned long debutTunnel = 0;
unsigned long debutRecup  = 0;

const int V_TUNNEL       = 18;
const int CIBLE_GAUCHE   = 24;
const int MARGE          = 3;
const int DIST_TROP_PRES = 22;
const int DIST_TROP_LOIN = 28;
const unsigned long TEMPS_SECURITE_ENTREE = 500;

void entrerTunnel() {
  Serial.println(F("Tunnel detecte -> entree"));
  arreter(); delay(300);
  radar.write(ANGLE_GAUCHE_TUN); delay(600);
  murDejaVu = false; derniereDistanceTunnel = 999;
  debutTunnel = millis();
  resetNoir(); resetBlanc();
  etat = TUNNEL;
}

void suivreMurGauche() {
  long d = distanceFiltreeTunnel();
  Serial.print(F("Distance gauche : ")); Serial.println(d);

  if (millis() - debutTunnel < TEMPS_SECURITE_ENTREE) {
    if (d == 999)           { avancerTunnel(12, 12); return; }
    if (d < DIST_TROP_PRES) { avancerTunnel(12, 24); return; }
    if (d > DIST_TROP_LOIN) { avancerTunnel(24, 14); return; }
  }
  if (d == 999)             { avancerTunnel(12, 12); return; }
  if (d < DIST_TROP_PRES)   { avancerTunnel(12, 26); return; }
  if (d > DIST_TROP_LOIN)   { avancerTunnel(28, 14); return; }

  int erreur     = d - CIBLE_GAUCHE;
  if (abs(erreur) <= MARGE) { avancerTunnel(V_TUNNEL, V_TUNNEL); return; }
  int correction = constrain((int)(erreur * 0.6), -5, 5);
  avancerTunnel(constrain(V_TUNNEL + correction, 14, 26),
                constrain(V_TUNNEL - correction, 14, 26));
}

void gererTunnel() {
  uint8_t pos = lireLigne();
  if (millis() - debutTunnel > 1800 && ligneVue(pos)) {
    Serial.println(F("Sortie tunnel -> RECUP_L2"));
    arreter(); delay(150);
    radar.write(ANGLE_CENTRE); delay(250);
    tunnelFait = true;
    resetNoir(); resetBlanc();
    debutRecup = millis();
    etat = RECUP_L2;
  } else {
    suivreMurGauche();
  }
}

// ============================================================
// RECUPERATION LIGNE APRES ESQUIVE
// ============================================================
void revenirSurLigne() {
  radar.write(ANGLE_CENTRE); delay(200);
  piloterMoteur(MOTEUR_A, AVANT, 40); piloterMoteur(MOTEUR_B, AVANT, 40); delay(400);
  piloterMoteur(MOTEUR_A, AVANT, 30); piloterMoteur(MOTEUR_B, AVANT, 30);
  unsigned long t = millis();
  while (true) {
    uint8_t pos = lireLigne();
    if (pos != 0b1111) break;
    if (millis() - t > 2000) break;
  }
  t = millis();
  while (true) {
    uint8_t pos = lireLigne();
    if (pos == 0b1001) break;
    else if (pos == 0b1101 || pos == 0b1110) tournerGauche(25);
    else if (pos == 0b1011 || pos == 0b0111) tournerDroite(25);
    else { piloterMoteur(MOTEUR_A, AVANT, 25); piloterMoteur(MOTEUR_B, AVANT, 25); }
    if (millis() - t > 3000) break;
  }
  arreter(); delay(150);
  Serial.println(F(">>> REPRISE SUIVEUR <<<"));
}

// ============================================================
// EVITEMENT O1 : PAR LA GAUCHE (inchangé)
// ============================================================
void esquiveObstacleGauche() {
  Serial.println(F("\n>>> EVITEMENT O1 (GAUCHE) <<<"));
  arreter(); delay(300);

  tournerGaucheSurPlace(45); delay(700); arreter();
  radar.write(ANGLE_DROITE_O1); delay(300);

  while (true) { float d = mesurerDistance(); if (d > 0 && d < 35) break; avancer(25); delay(30); }

  unsigned long timer = 0; bool perdu = false;
  while (!perdu) {
    float d = mesurerDistance();
    if      (d < 20)           { piloterMoteur(MOTEUR_A, AVANT, 30); piloterMoteur(MOTEUR_B, ARRIERE, 20); }
    else if (d > 20 && d < 30) { piloterMoteur(MOTEUR_A, AVANT, 20); piloterMoteur(MOTEUR_B, ARRIERE, 30); }
    else if (d >= 40) { if (!timer) timer = millis(); if (millis()-timer > 400) perdu = true; avancer(25); }
    else { avancer(25); timer = 0; }
    delay(30);
  }
  avancer(25); delay(1300); arreter();

  tournerDroiteSurPlace(45); delay(500); arreter();

  while (true) { float d = mesurerDistance(); if (d > 0 && d < 35) break; avancer(25); delay(30); }

  timer = 0; perdu = false;
  while (!perdu) {
    float d = mesurerDistance();
    if      (d < 16)           { piloterMoteur(MOTEUR_A, AVANT, 30); piloterMoteur(MOTEUR_B, ARRIERE, 20); }
    else if (d > 16 && d < 40) { piloterMoteur(MOTEUR_A, AVANT, 20); piloterMoteur(MOTEUR_B, ARRIERE, 30); }
    else if (d >= 40) { if (!timer) timer = millis(); if (millis()-timer > 400) perdu = true; avancer(25); }
    else { avancer(25); timer = 0; }
    delay(30);
  }
  avancer(25); delay(1200); arreter();

  tournerDroiteSurPlace(45); delay(500); arreter();

  while (true) {
    float d = mesurerDistance(); uint8_t pos = lireLigne();
    if (pos != 0b1111) break;
    if (d > 0 && d < 35) break;
    avancer(25); delay(30);
  }
  bool ligneTrouvee = false;
  while (!ligneTrouvee) {
    float d = mesurerDistance(); uint8_t pos = lireLigne();
    if (pos != 0b1111) { ligneTrouvee = true; break; }
    if      (d < 18)           { piloterMoteur(MOTEUR_A, AVANT, 30); piloterMoteur(MOTEUR_B, ARRIERE, 20); }
    else if (d > 18 && d < 40) { piloterMoteur(MOTEUR_A, AVANT, 20); piloterMoteur(MOTEUR_B, ARRIERE, 40); }
    else                       { piloterMoteur(MOTEUR_A, AVANT, 20); piloterMoteur(MOTEUR_B, ARRIERE, 35); }
    delay(20);
  }
  revenirSurLigne();
}

// ============================================================
// EVITEMENT O2 : PAR LA DROITE (inchangé)
// ============================================================
void esquiveObstacleDroite() {
  Serial.println(F("\n>>> EVITEMENT O2 (DROITE) <<<"));
  arreter(); delay(300);

  tournerDroiteSurPlace(45); delay(700); arreter();
  radar.write(ANGLE_GAUCHE_O2); delay(300);

  while (true) { float d = mesurerDistance(); if (d > 0 && d < 35) break; avancer(25); delay(30); }

  unsigned long timer = 0; bool perdu = false;
  while (!perdu) {
    float d = mesurerDistance();
    if      (d < 20)           { piloterMoteur(MOTEUR_A, AVANT, 20); piloterMoteur(MOTEUR_B, ARRIERE, 30); }
    else if (d > 20 && d < 30) { piloterMoteur(MOTEUR_A, AVANT, 30); piloterMoteur(MOTEUR_B, ARRIERE, 20); }
    else if (d >= 40) { if (!timer) timer = millis(); if (millis()-timer > 400) perdu = true; avancer(25); }
    else { avancer(25); timer = 0; }
    delay(30);
  }
  avancer(25); delay(1200); arreter();

  tournerGaucheSurPlace(45); delay(700); arreter();

  while (true) { float d = mesurerDistance(); if (d > 0 && d < 35) break; avancer(25); delay(30); }

  timer = 0; bool murPerdu = false;
  while (!murPerdu) {
    float d = mesurerDistance();
    if      (d < 20)           { piloterMoteur(MOTEUR_A, AVANT, 20); piloterMoteur(MOTEUR_B, ARRIERE, 30); timer = 0; }
    else if (d > 20 && d < 40) { piloterMoteur(MOTEUR_A, AVANT, 30); piloterMoteur(MOTEUR_B, ARRIERE, 20); timer = 0; }
    else if (d >= 40)          { if (!timer) timer = millis(); if (millis()-timer > 400) murPerdu = true;
                                  piloterMoteur(MOTEUR_A, AVANT, 30); piloterMoteur(MOTEUR_B, ARRIERE, 27); }
    delay(20);
  }

  while (true) {
    piloterMoteur(MOTEUR_A, AVANT, 45); piloterMoteur(MOTEUR_B, ARRIERE, 27);
    uint8_t pos = lireLigne();
    if (pos != 0b1111) break;
    delay(20);
  }
  arreter(); delay(100);
  radar.write(ANGLE_CENTRE); delay(200);
  Serial.println(F(">>> FIN O2 <<<"));
}

// ============================================================
// DEMI-TOUR (inchangé)
// ============================================================
void reculer() {
  Serial.println(F("Recul"));
  unsigned long debut = millis();
  while (millis() - debut < 1200) {
    uint8_t pos = lireLigne();
    if      (pos == 0b1001 || pos == 0b0000) { piloterMoteur(MOTEUR_A, ARRIERE, 30); piloterMoteur(MOTEUR_B, AVANT, 30); }
    else if (pos == 0b1011 || pos == 0b0111) { piloterMoteur(MOTEUR_A, ARRIERE, 20); piloterMoteur(MOTEUR_B, AVANT, 35); }
    else if (pos == 0b1101 || pos == 0b1110) { piloterMoteur(MOTEUR_A, ARRIERE, 35); piloterMoteur(MOTEUR_B, AVANT, 20); }
    else                                      { piloterMoteur(MOTEUR_A, ARRIERE, 25); piloterMoteur(MOTEUR_B, AVANT, 25); }
    delay(30);
  }
  arreter(); delay(150);
}

void faireDemiTour() {
  Serial.println(F("\n>>> DEMI-TOUR <<<"));
  arreter(); delay(300);
  reculer();
  rotationGauche(); delay(800);
  arreter(); delay(100);
  rotationGauche(); delay(600);
  while (true) {
    uint8_t pos = lireLigne();
    if (ligneVue(pos)) { arreter(); delay(100); Serial.println(F(">>> DEMI-TOUR TERMINE <<<")); return; }
    chercherLigneGauche();
    delay(30);
  }
}

// ============================================================
// CAPTEUR COULEUR (inchangé)
// ============================================================
void writeColorRegister(byte reg, byte value) {
  Wire.beginTransmission(COLOR_ADDR);
  Wire.write(0x80 | reg); Wire.write(value);
  Wire.endTransmission();
}

uint16_t readColorRegister16(byte reg) {
  Wire.beginTransmission(COLOR_ADDR);
  Wire.write(0x80 | reg); Wire.endTransmission();
  Wire.requestFrom(COLOR_ADDR, 2);
  if (Wire.available() < 2) return 0;
  uint16_t low = Wire.read(); uint16_t high = Wire.read();
  return (high << 8) | low;
}

void initColorSensor() {
  writeColorRegister(0x00, 0x03);
  writeColorRegister(0x01, 0xEB);
  writeColorRegister(0x0F, 0x01);
  delay(100);
}

uint32_t couleurDetectee = 0;

void lireCouleur() {
  uint16_t red   = readColorRegister16(0x16);
  uint16_t green = readColorRegister16(0x18);
  uint16_t blue  = readColorRegister16(0x1A);
  Serial.print(F("R=")); Serial.print(red);
  Serial.print(F(" G=")); Serial.print(green);
  Serial.print(F(" B=")); Serial.println(blue);
  if      (red > green && red > blue)   { Serial.println(F("ROUGE")); couleurDetectee = strip.Color(255, 0,   0); }
  else if (green > red && green > blue) { Serial.println(F("VERT"));  couleurDetectee = strip.Color(0,   255, 0); }
  else if (blue > red && blue > green)  { Serial.println(F("BLEU"));  couleurDetectee = strip.Color(0,   0, 255); }
  else                                  { Serial.println(F("INCONNUE")); couleurDetectee = strip.Color(255,255,255); }
}

void afficherRuban(uint32_t couleur) {
  for (int cycle = 0; cycle < 3; cycle++) {
    for (int i = 0; i < NB_LEDS; i++) strip.setPixelColor(i, couleur);
    strip.show(); delay(500);
    strip.clear(); strip.show(); delay(500);
  }
}

// ============================================================
// VARIABLES OBSTACLES
// ============================================================
int obstaclesTraites         = 0;
int SEUIL_DETECTION_OBSTACLE = 20;

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(9600);
  Wire.begin();

  LightSensorRGB_1.begin();
  initColorSensor();

  radar.attach(PIN_SERVO);
  radar.write(ANGLE_CENTRE);
  delay(500);

  strip.begin(); strip.clear(); strip.show();

  arreter();
  delay(1000);

  Serial.println(F("====================================="));
  Serial.println(F("  ROBOT PRET - ATTENTE DEPART        "));
  Serial.println(F("====================================="));
}

// ============================================================
// LOOP - MACHINE D'ETATS
// ============================================================
void loop() {
  uint8_t pos = lireLigne();

  switch (etat) {

    case ATTENTE_DEPART:
      arreter();
      if (pos == 0b0000) {
        Serial.println(F("Depart detecte !"));
        resetNoir(); resetBlanc(); derniereDirection = 0;
        delay(50);
        etat = SUIVI_L1;
      }
      break;

    case SUIVI_L1:
      tunnelDetecte = false;
      suiveurL1(pos);
      if (tunnelDetecte) entrerTunnel();
      delay(20);
      break;

    case TUNNEL:
      gererTunnel();
      delay(20);
      break;

    case RECUP_L2:
      if (ligneVue(pos)) {
        suiveurL1(pos);
        if (ligneCentree(pos)) {
          Serial.println(F("L2 recuperee -> SUIVI_L2"));
          resetNoir(); resetBlanc();
          erreurPrecedente = 0;
          etat = SUIVI_L2;
        }
      } else {
        recupererLigne();
      }
      delay(20);
      break;

    case SUIVI_L2:
    {
      float d = mesurerDistance();
      if (d > 0 && d < SEUIL_DETECTION_OBSTACLE) {
        Serial.println(F("O1 detecte -> EVITEMENT_O1"));
        arreter(); delay(200);
        etat = EVITEMENT_O1;
      } else {
        suiveurPID(pos);
        delay(20);
      }
      break;
    }

    case EVITEMENT_O1:
      esquiveObstacleGauche();
      obstaclesTraites = 1;
      resetNoir();
      erreurPrecedente = 0;
      etat = SUIVI_L3;
      break;

    case SUIVI_L3:
    {
      float d = mesurerDistance();
      if (d > 0 && d < SEUIL_DETECTION_OBSTACLE) {
        Serial.println(F("O2 detecte -> EVITEMENT_O2"));
        arreter(); delay(200);
        etat = EVITEMENT_O2;
      } else {
        suiveurPID(pos);
        delay(20);
      }
      break;
    }

    case EVITEMENT_O2:
      esquiveObstacleDroite();
      obstaclesTraites = 2;
      SEUIL_DETECTION_OBSTACLE = 2;
      resetNoir();
      erreurPrecedente = 0;
      etat = SUIVI_RAMPE;
      break;

    case SUIVI_RAMPE:
    {
      float d = mesurerDistance();
      if (d > 0 && d <= 3.0) {
        Serial.println(F("Panneau couleur a 3cm -> DETECTION_COULEUR"));
        arreter();
        etat = DETECTION_COULEUR;
      } else {
        suiveurPID(pos);
        delay(20);
      }
      break;
    }

    case DETECTION_COULEUR:
      lireCouleur();
      etat = AFFICHAGE_LEDS;
      break;

    case AFFICHAGE_LEDS:
      arreter();
      afficherRuban(couleurDetectee);
      etat = DEMI_TOUR;
      break;

    case DEMI_TOUR:
      faireDemiTour();
      resetNoir();
      erreurPrecedente = 0;
      arreterSur0000 = true;
      etat = SUIVI_RETOUR;
      break;

    case SUIVI_RETOUR:
      suiveurPID(pos);
      delay(20);
      break;

    case FIN_PARCOURS:
      arreter();
      Serial.println(F("=== FIN DU PARCOURS ==="));
      activerMoteurFin();
      while (true) {}
      break;
  }
}
