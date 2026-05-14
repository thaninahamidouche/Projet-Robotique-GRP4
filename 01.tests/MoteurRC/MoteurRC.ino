#include <Wire.h>

// Adresses trouvées par ton scanner
#define MOTEUR_A 0x66 
#define MOTEUR_B 0x68 

// Commandes de direction pour le DRV8830
#define ARRET     0x00
#define AVANT     0x01
#define ARRIERE   0x02
#define FREIN     0x03

void setup() {
  Wire.begin();
  Serial.begin(115200);
  Serial.println("Test Mini I2C Motor Driver (DRV8830)");
}

// Fonction pour piloter un moteur
void piloterMoteur(byte adresse, byte direction, byte vitesse) {
  // La vitesse sur ce module va de 0 à 63 (6 bits)
  if (vitesse > 63) vitesse = 63;
  
  // Le registre de contrôle est 0x00
  // On combine la vitesse et la direction dans un seul octet
  byte commande = (vitesse << 2) | direction;
  
  Wire.beginTransmission(adresse);
  Wire.write(0x00);      // Registre de contrôle
  Wire.write(commande);  // Valeur vitesse + direction
  Wire.endTransmission();
}

void loop() {
  Serial.println("En avant !");
  piloterMoteur(MOTEUR_A, AVANT, 40); // Vitesse 40/63
  piloterMoteur(MOTEUR_B, ARRIERE, 40);
  delay(2000);

  Serial.println("Stop");
  piloterMoteur(MOTEUR_A, ARRET, 0);
  piloterMoteur(MOTEUR_B, ARRET, 0);
  delay(1000);

  Serial.println("En arriere !");
  piloterMoteur(MOTEUR_A, ARRIERE, 40);
  piloterMoteur(MOTEUR_B, AVANT, 40);
  delay(2000);

  Serial.println("Freinage");
  piloterMoteur(MOTEUR_A, FREIN, 0);
  piloterMoteur(MOTEUR_B, FREIN, 0);
  delay(2000);
}
