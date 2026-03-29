#include <Stepper.h>
#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>

LiquidCrystal lcd(7, 8, A0, A1, A2, A3);

const int STEPS = 2048;
Stepper stepper(STEPS, 2, 4, 3, 5);

#define SS_PIN 10
#define RST_PIN 9
MFRC522 rfid(SS_PIN, RST_PIN);

const int BUZZER_PIN = 6;
const int DEFAULT_COMPARTMENT = 1; // carton

int currentCompartment = 1;

struct Badge {
  byte uid[4];
  int compartment;
};

Badge badges[] = {
  {{0x35, 0xC2, 0xEB, 0x76}, 2},  // Compartiment 2
  {{0x34, 0x45, 0x16, 0x06}, 3},  // Compartiment 3
  {{0xA4, 0x70, 0xB3, 0x85}, 4},  // Compartiment 4
};

byte refusedUID[4] = {0xF1, 0x2B, 0x6A, 0x01};

void bipSuccess() {
  tone(BUZZER_PIN, 1000, 200); delay(250);
  tone(BUZZER_PIN, 1500, 200); delay(250);
}

void bipFail() {
  tone(BUZZER_PIN, 400, 1000); delay(1000);
}

int getCompartment(byte* uid) {
  for (int i = 0; i < 4; i++) {
    bool match = true;
    for (byte j = 0; j < 4; j++) {
      if (uid[j] != badges[i].uid[j]) {
        match = false;
        break;
      }
    }
    if (match) return badges[i].compartment;
  }
  return -1;
}

void goToCompartment(int target) {
  int diff = target - currentCompartment;
  if (diff < 0) diff += 4;
  int steps = diff * 518;
  if (steps > 0) stepper.step(steps);
  currentCompartment = target;
}

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);

  Serial.begin(9600);
  stepper.setSpeed(10);

  SPI.begin();
  rfid.PCD_Init();

  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Scannez un badge");
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  byte* uid = rfid.uid.uidByte;

  Serial.print("UID: ");
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (uid[i] < 0x10) Serial.print("0");
    Serial.print(uid[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  int target = getCompartment(uid);

  if (target != -1) {
    Serial.print("ACCES AUTORISE - Compartiment ");
    Serial.println(target);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Compartiment ");
    lcd.print(target);

    bipSuccess();

    // Tourne vers le bon compartiment
    goToCompartment(target);

    // ouvert 5 secondes
    delay(5000);

    // Revient au carton (compartiment 1)
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Fermeture...");
    goToCompartment(DEFAULT_COMPARTMENT);

  } else {
    Serial.println("ACCES REFUSE");

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ACCES REFUSE");

    bipFail();
  }

  delay(1000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scannez un badge");

  rfid.PICC_HaltA();
}