#include <Arduino.h>
#include <Wire.h>
#include <MeOrion.h>

// ==================================================
// BRANCHEMENTS
// LED       -> D6
// Ultrason  -> D7
// Couleur   -> I2C
// ==================================================

const int sigPin = 7;

// Moteurs
#define MOTEUR_A 0x66
#define MOTEUR_B 0x68

#define ARRET   0x00
#define AVANT   0x01
#define ARRIERE 0x02

const byte VITESSE = 30;
const int DIST_STOP = 2;

// Capteur couleur I2C
#define COLOR_ADDR 0x29

void setup() {

  Serial.begin(9600);
  Wire.begin();

  initColorSensor();

  arreter();

  Serial.println("Robot pret");
}

// ==================================================
void loop() {

  float distance = mesurerDistance();

  Serial.print("Distance : ");
  Serial.print(distance);
  Serial.println(" cm");

  // STOP immédiat à 2 cm
  if (distance <= 2.0 && distance > 0) {

    arreter();      // arrêt moteurs immédiat
    delay(50);

    // sécurité : on coupe une deuxième fois
    arreter();

    Serial.println("STOP - obstacle à 2 cm");

    delay(500);

    afficherCouleur();

    // rester bloqué à l'arrêt
    while (true) {
      arreter();
    }
  }

  // sinon avance
  avancer();

  delay(20);
}

// ==================================================
// ULTRASON
// ==================================================

float mesurerDistance() {

  pinMode(sigPin, OUTPUT);

  digitalWrite(sigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(sigPin, HIGH);
  delayMicroseconds(10);

  digitalWrite(sigPin, LOW);

  pinMode(sigPin, INPUT);

  long duree = pulseIn(sigPin,HIGH,30000);

  if(duree==0)
    return 400;

  float distance=duree*0.034/2.0;

  return distance;
}

// ==================================================
// MOTEURS
// ==================================================

void piloterMoteur(byte adresse, byte direction, byte vitesse){

    if(vitesse>63)
      vitesse=63;

    byte commande=(vitesse<<2)|direction;

    Wire.beginTransmission(adresse);
    Wire.write(0x00);
    Wire.write(commande);
    Wire.endTransmission();
}

void avancer(){

    piloterMoteur(MOTEUR_A,AVANT,VITESSE);
    piloterMoteur(MOTEUR_B,ARRIERE,VITESSE);
}

void arreter(){

    piloterMoteur(MOTEUR_A,ARRET,0);
    piloterMoteur(MOTEUR_B,ARRET,0);
}

// ==================================================
// CAPTEUR COULEUR
// ==================================================

void writeColorRegister(byte reg,byte value){

  Wire.beginTransmission(COLOR_ADDR);
  Wire.write(0x80|reg);
  Wire.write(value);
  Wire.endTransmission();
}

uint16_t readColorRegister16(byte reg){

  Wire.beginTransmission(COLOR_ADDR);
  Wire.write(0x80|reg);
  Wire.endTransmission();

  Wire.requestFrom(COLOR_ADDR,2);

  if(Wire.available()<2)
    return 0;

  uint16_t low=Wire.read();
  uint16_t high=Wire.read();

  return (high<<8)|low;
}

void initColorSensor(){

  writeColorRegister(0x00,0x03);
  writeColorRegister(0x01,0xEB);
  writeColorRegister(0x0F,0x01);

  delay(100);
}

void afficherCouleur(){

  uint16_t red=readColorRegister16(0x16);
  uint16_t green=readColorRegister16(0x18);
  uint16_t blue=readColorRegister16(0x1A);

  Serial.println("===== COULEUR =====");

  if(red>green && red>blue){

      Serial.println("ROUGE");
  }

  else if(green>red && green>blue){

      Serial.println("VERT");
  }

  else if(blue>red && blue>green){

      Serial.println("BLEU");
  }

  else{

      Serial.println("INCONNUE");
  }
}