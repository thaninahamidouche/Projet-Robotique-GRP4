#include <Arduino.h>
#include <Wire.h>
#include <MeOrion.h>
#include <MeRGBLineFollower.h>

MeRGBLineFollower ligne(PORT_3);
Servo radar;

const int PIN_SERVO = A0;
const int PIN_US = 7;

#define MD 0x66
#define MG 0x68
#define STOP 0x00
#define AVANT 0x01
#define ARRIERE 0x02

enum Mode { ATTENTE, LIGNE, TUNNEL, RECUP };
Mode mode = ATTENTE;

const int ANGLE_CENTRE = 60;
const int ANGLE_GAUCHE = 150;

const int V_LIGNE = 34;
const int V_TUNNEL = 23;

const int CIBLE_GAUCHE = 28;
const int MARGE = 3;

float corrD = 0.78;
float corrG = 1.00;

int perdu = 0;
unsigned long debutTunnel = 0;

void moteur(byte adr, byte dir, int v) {
  v = constrain(v, 0, 63);
  Wire.beginTransmission(adr);
  Wire.write(0x00);
  Wire.write((v << 2) | dir);
  Wire.endTransmission();
}

void avancer(int vd, int vg) {
  moteur(MD, AVANT, vd * corrD);
  moteur(MG, ARRIERE, vg * corrG);
}

void arret() {
  moteur(MD, STOP, 0);
  moteur(MG, STOP, 0);
}

long distanceCM() {
  pinMode(PIN_US, OUTPUT);
  digitalWrite(PIN_US, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_US, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_US, LOW);

  pinMode(PIN_US, INPUT);
  long t = pulseIn(PIN_US, HIGH, 22000);

  if (t == 0) return 999;
  return t / 58;
}

long distanceFiltree() {
  long somme = 0;
  int n = 0;

  for (int i = 0; i < 3; i++) {
    long d = distanceCM();
    if (d != 999) {
      somme += d;
      n++;
    }
    delay(5);
  }

  if (n == 0) return 999;
  return somme / n;
}

uint8_t lireLigne() {
  ligne.updataAllSensorValue();
  return ligne.getPositionState();
}

// Plus permissif : si ce n'est pas 1111, il voit du noir
bool ligneVue(uint8_t p) {
  return p != 0b1111;
}

void suivreLigne(uint8_t p) {
  if (p == 0b1001 || p == 0b0000) {
    avancer(V_LIGNE, V_LIGNE);
  }

  else if (p == 0b1011 || p == 0b0111 || p == 0b1000 || p == 0b1100) {
    avancer(22, 42);   // tourner à droite
  }

  else if (p == 0b1101 || p == 0b1110 || p == 0b0001 || p == 0b0011) {
    avancer(42, 22);   // tourner à gauche
  }

  else {
    avancer(V_LIGNE, V_LIGNE);
  }
}

void entrerTunnel() {
  arret();
  delay(200);

  radar.write(ANGLE_GAUCHE);
  delay(300);

  debutTunnel = millis();
  mode = TUNNEL;
}

void suivreMurGauche() {
  long d = distanceFiltree();

  if (millis() - debutTunnel < 500) {
    avancer(V_TUNNEL, V_TUNNEL);
    return;
  }

  if (d == 999 || d > 100) {
    avancer(V_TUNNEL, V_TUNNEL);
    return;
  }

  if (d < CIBLE_GAUCHE - MARGE) {
    // TROP PROCHE du mur gauche -> il faut s'éloigner vers la DROITE
    // On donne plus de puissance au moteur gauche (deuxième paramètre)
    avancer(22, 34);   
  }

  else if (d > CIBLE_GAUCHE + MARGE) {
    // TROP LOIN du mur gauche -> il faut se rapprocher vers la GAUCHE
    // On donne plus de puissance au moteur droit (premier paramètre)
    avancer(34, 22);   
  }

  else {
    avancer(V_TUNNEL, V_TUNNEL);
  }
}

void setup() {
  Wire.begin();
  Serial.begin(9600);

  ligne.begin();

  radar.attach(PIN_SERVO);
  radar.write(ANGLE_CENTRE);

  arret();
  delay(500);
}

void loop() {
  uint8_t p = lireLigne();

  if (mode == ATTENTE) {
    arret();

    if (ligneVue(p)) {
      perdu = 0;
      mode = LIGNE;
      delay(300);
    }
  }

  else if (mode == LIGNE) {
    if (ligneVue(p)) {
      perdu = 0;
      suivreLigne(p);
    }

    else {
      arret();
      perdu++;

      if (perdu >= 5) {
        entrerTunnel();
      }
    }
  }

  else if (mode == TUNNEL) {
    if (millis() - debutTunnel > 1000 && ligneVue(p)) {
      radar.write(ANGLE_CENTRE);
      delay(200);
      mode = RECUP;
    }

    else {
      suivreMurGauche();
    }
  }

  else if (mode == RECUP) {
    if (ligneVue(p)) {
      suivreLigne(p);

      if (p == 0b1001 || p == 0b0000) {
        perdu = 0;
        mode = LIGNE;
      }
    }

    else {
      avancer(18, 18);
    }
  }

  delay(25);
}