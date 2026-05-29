# 🤖 Robot Autonome — Lanceur & Plan d'expérience

> Projet de robotique L3 — Université Évry Paris Saclay  
> Groupe 4 : Thanina Hamidouche, Jeevithan Jeyakumar, Salma El Battahi, Lina El Hachemi, Anas Baouche, Nathan Pino, Sarah Rabia

---

## 📋 Table des matières

- [Présentation](#présentation)
- [Conception du lanceur](#conception-du-lanceur)
- [Plan d'expérience](#plan-dexpérience)
  - [Modèle mathématique](#modèle-mathématique)
  - [Essais](#essais)
  - [Résultats et formule finale](#résultats-et-formule-finale)
  - [Utilisation lors du lancer](#utilisation-lors-du-lancer)
- [Structure du dépôt](#structure-du-dépôt)

---

## Présentation

Le lanceur est le module responsable de projeter une balle dans un panier situé à une distance variable. Le robot mesure cette distance via son capteur ultrason, calcule l'angle de tir optimal, puis déclenche le lancement. L'objectif est de couvrir une portée comprise entre **1,5 m et 3 m**.

---

## Conception du lanceur

### Principe de fonctionnement

Le lanceur utilise un **système à élastiques** : les élastiques sont tendus manuellement, puis bloqués. Au moment du tir, un **système de goupille** les libère, propulsant la balle posée sur un support incliné.

> ⚠️ L'armement automatique par moteur a été abandonné : le moteur disponible n'était pas suffisamment puissant pour tendre les élastiques. L'armement est donc **manuel**, le déclenchement reste **automatique** via la goupille.

### Réglage de l'angle

- L'angle de tir est **ajustable mécaniquement** pour adapter la portée à la distance mesurée.
- La mesure de l'angle se fait grâce à un **encodeur optique** couplé à une **roue codeuse imprimée en 3D**.

### Positionnement sur le robot

Le lanceur est fixé **au centre du robot** afin de préserver l'équilibre général, notamment lors des montées de rampe et du passage sur la passerelle.

---

## Plan d'expérience

L'objectif du plan d'expérience est de modéliser mathématiquement la **portée** (distance d'atterrissage de la balle) en fonction des paramètres de tir, afin de calculer automatiquement l'angle optimal pour une distance donnée.

### Variables

| Variable | Description | Plage | Variable centrée réduite |
|----------|-------------|-------|--------------------------|
| `α` (X1) | Angle de tir | 40° → 70° | -1 → 1 |
| X2 | Cran d'armement (tension des élastiques) | Cran 1 → Cran 3 | -1 → 1 |

**Correspondance des niveaux :**

| α | Cran d'armement | Variable centrée réduite |
|---|-----------------|--------------------------|
| 70° | Cran 3 | 1 |
| 55° | Cran 2 | 0 |
| 40° | Cran 1 | -1 |

### Modèle mathématique

Un **modèle du second ordre** a été retenu pour sa précision :

$$f(X_1, X_2) = a_0 + a_1 X_1 + a_2 X_2 + a_{12} X_1 X_2 + a_{11} X_1^2 + a_{22} X_2^2 + \epsilon$$

Les coefficients sont déterminés par la méthode des moindres carrés :

$$a = (C^T C)^{-1} C^T Y$$

**Vecteur de coefficients obtenu :**

| Coefficient | Valeur |
|-------------|--------|
| a0 | 1.3281 |
| a1 | -0.6156 |
| a2 | 1.1655 |
| a12 | -0.0642 |
| a11 | -0.3322 |
| a22 | 0.1579 |

**Formule complète (deux variables) :**

$$f(\alpha, X_2) = 1.3281 - 0.6156\,\alpha + 1.1655\,X_2 - 0.0642\,\alpha X_2 - 0.3322\,\alpha^2 + 0.1579\,X_2^2$$

### Essais

- **9 combinaisons** testées (toutes les combinaisons de 3 niveaux × 3 niveaux).
- Chaque combinaison répétée **3 fois**, la **moyenne** des distances est retenue.
- Angles testés : **40°, 55°, 70°**.
- Les matrices de calcul (C, Cᵀ, CᵀC, CᵀY) sont disponibles dans le dossier [`/annexes`](./annexes/).

### Résultats et formule finale

L'analyse des courbes de portée montre que pour couvrir la plage cible de **1,5 m à 3 m**, il faut utiliser le **cran 3** (X2 = 1).

En fixant X2 = 1, la formule se réduit à une **équation à une variable** :

$$f(\alpha) = -0.3322\,\alpha^2 - 0.6798\,\alpha + 2.6515$$

### Utilisation lors du lancer

Pour un panier situé à une distance `d`, le robot résout :

$$f(\alpha) - d = 0$$

**Calcul du discriminant :**

$$\Delta = b^2 - 4ac = 0.6788^2 + 4 \times 0.3322 \times (2.6515 - d)$$

**Racines :**

$$\alpha = \frac{-b \pm \sqrt{\Delta}}{2a} = \frac{-0.6788 \pm \sqrt{\Delta}}{2 \times 0.3322}$$

On retient la racine dans le domaine valide `[-1, 1]`.

**Reconversion en angle réel :**  
La variable centrée réduite est reconvertie avec une règle de 3 :  
`1` en variable centrée réduite correspond à `15°` réels (passage de 55° à 70°).

---

## Structure du dépôt

```
robot-autonome/
├── README.md
├── src/
│   └── lanceur/
│       ├── calcul_angle.ino      # Calcul de l'angle optimal (Arduino)
│       └── declenchement.ino     # Gestion de la goupille
├── cad/
│   └── roue_codeuse.stl          # Roue codeuse imprimée en 3D
└── annexes/
    ├── plan_experience_excel.xlsx # Matrices de calcul et résultats des essais
    └── photos/
        ├── lanceur_v1.jpg
        ├── lanceur_final.jpg
        └── systeme_goupille.jpg
```

---

> Université Évry Paris Saclay — Concours de Robotique L3 — Semestre 2, 2026
