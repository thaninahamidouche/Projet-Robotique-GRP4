// ============================================================
//  Robot défi 2026 — Sections 3 & 4
//  Détection obstacle à ~20 cm → contournement IMMÉDIAT sans recul
//  O1 (section 3) : contournement par la GAUCHE
//  O2 (section 4) : contournement par la DROITE
// ============================================================

// ---- PINS MOTEURS ------------------------------------------
const int ENA = 2;   // PWM moteur gauche
const int IN1 = 22;
const int IN2 = 24;

const int ENB = 3;   // PWM moteur droit
const int IN3 = 26;
const int IN4 = 28;

// ---- CAPTEUR ULTRASON --------------------------------------
const int TRIG = 4;
const int ECHO = 5;

// ---- SUIVEUR DE LIGNE --------------------------------------
const int LINE_PIN = A0;

// ---- PARAMÈTRES RÉGLABLES ----------------------------------
// Vitesse réduite pendant le suivi de ligne pour avoir le temps
// de réagir à 20 cm sans toucher l'obstacle
const int VITESSE_BASE   = 50;
const int VITESSE_VIRAGE = 80;

// Détection déclenchée à 15 cm (modifiable entre 10 et 20)
const int DIST_DETECTION = 15;  // cm

// Durées à calibrer sur le terrain (ms)
// L'obstacle fait 44cm x 33cm donc :
const int T_VIRAGE_90    = 550;   // temps pour pivoter ~90° sur place
const int T_AVANCE_LARGE = 800;   // dépasser la largeur 33cm + marge
const int T_AVANCE_LONG  = 1300;  // longer la longueur 44cm + marge
const int T_RECENTER     = 500;   // avancer après dernier virage

// Seuils capteur ligne (à calibrer)
const int LIGNE_GAUCHE_MAX = 200;
const int LIGNE_DROITE_MIN = 800;

// ---- ÉTAT GLOBAL -------------------------------------------
bool o1Contourne = false;
bool o2Contourne = false;

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(9600);

  pinMode(ENA, OUTPUT); pinMode(ENB, OUTPUT);
  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(LINE_PIN, INPUT);

  stopMoteurs();
  delay(1000);
  Serial.println("=== Robot prêt — vitesse 50, détection 15 cm ===");
}

// ============================================================
//  BOUCLE PRINCIPALE
// ============================================================
void loop() {

  long dist = mesurerDistance();

  // Détection O1 → contournement GAUCHE immédiat
  if (!o1Contourne && dist <= DIST_DETECTION) {
    Serial.print("O1 à "); Serial.print(dist); Serial.println(" cm → contournement GAUCHE");
    stopMoteurs();
    delay(100); // micro-pause pour stabiliser avant de virer
    contournerGauche();
    o1Contourne = true;
    return;
  }

  // Détection O2 → contournement DROITE immédiat
  if (o1Contourne && !o2Contourne && dist <= DIST_DETECTION) {
    Serial.print("O2 à "); Serial.print(dist); Serial.println(" cm → contournement DROITE");
    stopMoteurs();
    delay(100);
    contournerDroite();
    o2Contourne = true;
    return;
  }

  // Suivi de ligne normal
  suivreLigne();
}

// ============================================================
//  SUIVI DE LIGNE
// ============================================================
void suivreLigne() {
  int val = analogRead(LINE_PIN);

  if (val < LIGNE_GAUCHE_MAX) {
    // Ligne détectée à gauche → corriger gauche
    motorGauche(VITESSE_BASE - 50);
    motorDroit(VITESSE_BASE);
  } else if (val > LIGNE_DROITE_MIN) {
    // Ligne détectée à droite → corriger droite
    motorGauche(VITESSE_BASE);
    motorDroit(VITESSE_BASE - 50);
  } else {
    // Centré
    avancer(VITESSE_BASE);
  }
}

// ============================================================
//  CONTOURNEMENT PAR LA GAUCHE (O1)
//
//  Vue de dessus :
//
//      ←←← virage gauche
//      ↑              ↑
//      ↑   [OBSTACLE] ↑
//      ↑              ↑
//      [ROBOT] →→→→→→→
//
//  Séquence :
//  1. Virer 90° à gauche
//  2. Avancer (dépasser la largeur de l'obstacle : 33 cm)
//  3. Virer 90° à droite
//  4. Avancer (longer la longueur de l'obstacle : 44 cm)
//  5. Virer 90° à droite
//  6. Avancer pour revenir à hauteur de la ligne
//  7. Virer 90° à gauche pour reprendre le cap
//  8. Rechercher la ligne
// ============================================================
void contournerGauche() {
  Serial.println("--- Contournement gauche ---");

  // 1. Virer 90° gauche
  tournerGauche();
  delay(T_VIRAGE_90);
  stopMoteurs(); delay(150);

  // 2. Dépasser la largeur de l'obstacle
  avancer(VITESSE_VIRAGE);
  delay(T_AVANCE_LARGE);
  stopMoteurs(); delay(150);

  // 3. Virer 90° droite pour longer l'obstacle
  tournerDroite();
  delay(T_VIRAGE_90);
  stopMoteurs(); delay(150);

  // 4. Longer la longueur de l'obstacle
  avancer(VITESSE_VIRAGE);
  delay(T_AVANCE_LONG);
  stopMoteurs(); delay(150);

  // 5. Virer 90° droite pour revenir vers la ligne
  tournerDroite();
  delay(T_VIRAGE_90);
  stopMoteurs(); delay(150);

  // 6. Avancer pour se rapprocher de la ligne
  avancer(VITESSE_VIRAGE);
  delay(T_RECENTER);
  stopMoteurs(); delay(150);

  // 7. Virer 90° gauche pour reprendre le cap initial
  tournerGauche();
  delay(T_VIRAGE_90);
  stopMoteurs(); delay(150);

  // 8. Rechercher et reprendre la ligne
  rechercherLigne();
  Serial.println("O1 contourné ✓");
}

// ============================================================
//  CONTOURNEMENT PAR LA DROITE (O2) — miroir du gauche
//
//  Séquence :
//  1. Virer 90° à droite
//  2. Avancer (dépasser la largeur : 33 cm)
//  3. Virer 90° à gauche
//  4. Avancer (longer la longueur : 44 cm)
//  5. Virer 90° à gauche
//  6. Avancer pour revenir à hauteur de la ligne
//  7. Virer 90° à droite pour reprendre le cap
//  8. Rechercher la ligne
// ============================================================
void contournerDroite() {
  Serial.println("--- Contournement droite ---");

  // 1. Virer 90° droite
  tournerDroite();
  delay(T_VIRAGE_90);
  stopMoteurs(); delay(150);

  // 2. Dépasser la largeur de l'obstacle
  avancer(VITESSE_VIRAGE);
  delay(T_AVANCE_LARGE);
  stopMoteurs(); delay(150);

  // 3. Virer 90° gauche pour longer l'obstacle
  tournerGauche();
  delay(T_VIRAGE_90);
  stopMoteurs(); delay(150);

  // 4. Longer la longueur de l'obstacle
  avancer(VITESSE_VIRAGE);
  delay(T_AVANCE_LONG);
  stopMoteurs(); delay(150);

  // 5. Virer 90° gauche pour revenir vers la ligne
  tournerGauche();
  delay(T_VIRAGE_90);
  stopMoteurs(); delay(150);

  // 6. Avancer pour se rapprocher de la ligne
  avancer(VITESSE_VIRAGE);
  delay(T_RECENTER);
  stopMoteurs(); delay(150);

  // 7. Virer 90° droite pour reprendre le cap initial
  tournerDroite();
  delay(T_VIRAGE_90);
  stopMoteurs(); delay(150);

  // 8. Rechercher et reprendre la ligne
  rechercherLigne();
  Serial.println("O2 contourné ✓");
}

// ============================================================
//  RECHERCHE DE LIGNE après contournement
//  Avance doucement jusqu'à retrouver la ligne, timeout 3s
// ============================================================
void rechercherLigne() {
  Serial.println("Recherche ligne...");
  unsigned long debut = millis();

  while (millis() - debut < 3000) {
    int val = analogRead(LINE_PIN);
    if (val >= LIGNE_GAUCHE_MAX && val <= LIGNE_DROITE_MIN) {
      Serial.println("Ligne retrouvée !");
      stopMoteurs();
      return;
    }
    avancer(VITESSE_VIRAGE - 30);
    delay(20);
  }

  stopMoteurs();
  Serial.println("ATTENTION : ligne non retrouvée !");
}

// ============================================================
//  MESURE DISTANCE
// ============================================================
long mesurerDistance() {
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  long duree = pulseIn(ECHO, HIGH, 30000);
  if (duree == 0) return 500;
  return (duree * 343L) / 20000;
}

// ============================================================
//  COMMANDES MOTEURS
// ============================================================
void motorGauche(int pwm) {
  analogWrite(ENA, constrain(pwm, 0, 255));
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
}

void motorDroit(int pwm) {
  analogWrite(ENB, constrain(pwm, 0, 255));
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void avancer(int vitesse) {
  analogWrite(ENA, vitesse);
  analogWrite(ENB, vitesse);
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

// Pivot gauche : moteur G recule, moteur D avance
void tournerGauche() {
  analogWrite(ENA, VITESSE_VIRAGE);
  analogWrite(ENB, VITESSE_VIRAGE);
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

// Pivot droite : moteur G avance, moteur D recule
void tournerDroite() {
  analogWrite(ENA, VITESSE_VIRAGE);
  analogWrite(ENB, VITESSE_VIRAGE);
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
}

void stopMoteurs() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}
