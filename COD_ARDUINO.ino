// Include librăriile necesare pentru senzorul INA219 (curent/tensiune) și afișajul OLED
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Dimensiunile afișajului OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1  // Nu se folosește pinul de reset

// Inițializarea obiectului display OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Inițializarea senzorului de curent INA219
Adafruit_INA219 ina219;

// Definirea pinilor pentru senzorul ultrasonic
#define TRIG 11
#define ECHO 12

// Definirea pinilor pentru Pompa1 (motor 1)
const int in3 = 7, in4 = 6, enb = 3;

// Definirea pinilor pentru Pompa2 (motor 2)
const int in1 = 9, in2 = 8, ena = 10;

// Parametrii pentru controlul PID
float Kp = 10.0, Ki = 0.1, Kd = 0.5;
float previous_error = 0.0, integral = 0.0;
unsigned long lastTimePID = 0;
unsigned long timestamp_pompa1_stop = 0;
int viteza_anterioara = 0;

// Variabile pentru mesajul serial, distanțe și stări pompe
String mesaj = "";
float distanta2 = 0.0;
float distanta1 = 0.0;
int pompa1_stare = 0, pompa2_stare = 0;
String mod_funct = "automat";  // Mod implicit automat

// Flag-uri de control pentru stări pompe
bool pompa1_terminata = false;
bool pompa2_activata = false;

// SETUP – se execută o singură dată la pornirea Arduino
void setup() {
  Serial.begin(9600);     // Inițializare comunicare serială
  Wire.begin();           // Inițializare comunicație I2C

  ina219.begin();         // Pornire senzor curent INA219
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // Inițializare OLED pe adresa 0x3C
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);

  // Setare pini ca OUTPUT pentru ambele pompe și senzor ultrasonic
  pinMode(in3, OUTPUT); pinMode(in4, OUTPUT); pinMode(enb, OUTPUT);
  pinMode(in1, OUTPUT); pinMode(in2, OUTPUT); pinMode(ena, OUTPUT);
  pinMode(TRIG, OUTPUT); pinMode(ECHO, INPUT);

  // Direcția inițială a motoarelor (pompele)
  digitalWrite(in3, HIGH); digitalWrite(in4, LOW);
  digitalWrite(in1, HIGH); digitalWrite(in2, LOW);

  // Ambele pompe oprite la start
  analogWrite(enb, 0);
  analogWrite(ena, 0);
}

void loop() {
  // Procesare date primite prin serial (ex: DIST:xx, MOD:automat/manual etc.)
  while (Serial.available()) {
    char c = Serial.read();  // Citește un caracter
    if (c == '\n') {         // Dacă e sfârșit de linie, procesează mesajul
      mesaj.trim();

      if (mesaj.startsWith("DIST:")) {
        distanta2 = mesaj.substring(5).toFloat();  // Setează distanța 2
      }

      else if (mesaj.startsWith("MOD:")) {
        mod_funct = mesaj.substring(4);  // Setează modul (automat/manual)

        // RESET complet la schimbarea modului
        pompa1_terminata = false;
        pompa2_activata = false;
        viteza_anterioara = 0;
        previous_error = 0;
        integral = 0;
        lastTimePID = millis();

        analogWrite(enb, 0);
        analogWrite(ena, 0);
      }

      // Comenzi manuale pentru pompe
      else if (mod_funct == "manual") {
        if (mesaj.startsWith("POMPA1:"))
          pompa1_stare = mesaj.substring(7).toInt();  // ON/OFF pentru Pompa1
        else if (mesaj.startsWith("POMPA2:"))
          pompa2_stare = mesaj.substring(7).toInt();  // ON/OFF pentru Pompa2
      }

      mesaj = "";  // Golește mesajul pentru următoarea citire
    } else mesaj += c;  // Adaugă caracterul în mesaj
  }

  distanta1 = citireDistanta();  // Citește distanța de la senzorul ultrasonic
  float tensiune = ina219.getBusVoltage_V();  // Tensiunea citită
  float curent = ina219.getCurrent_mA();      // Curentul citit

  // Oprire de urgență dacă tensiunea este în afara limitelor admise
  if (tensiune < 6.0 || tensiune > 8.4) {
    analogWrite(enb, 0); analogWrite(ena, 0);
    afisareAlertaOLED(tensiune);
    return;
  }

  // MODUL MANUAL – control direct al pompelor
  if (mod_funct == "manual") {
    analogWrite(enb, pompa1_stare ? 100 : 0);
    analogWrite(ena, pompa2_stare ? 100 : 0);
  }

  // MODUL AUTOMAT – control cu logică PID
  else if (mod_funct == "automat") {
    if (distanta1 >= 20.0 || distanta2 <= 8.0) {
      // Condiții de oprire: rezervor plin sau golit
      analogWrite(enb, 0);
      analogWrite(ena, 0);
      viteza_anterioara = 0;
      pompa1_terminata = false;
      pompa2_activata = false;
    }

    else if (!pompa1_terminata && distanta1 < 20 && distanta2 > 8.0) {
      float setpoint = 16.0;
      float error = distanta2 - setpoint;

      if (error <= 0.3) {
        // Dacă s-a atins nivelul dorit, oprește Pompa1
        analogWrite(enb, 0);
        viteza_anterioara = 0;
        pompa1_terminata = true;
        timestamp_pompa1_stop = millis();  // Pornește timer pentru Pompa2
      } else {
        // Aplică algoritmul PID
        unsigned long now = millis();// Timpul curent
        float deltaTime = (now - lastTimePID) / 1000.0; // Timp scurs (sec)
        lastTimePID = now; // Actualizează timpul anterior

        integral += error * deltaTime; // Acumulare eroare (I)
        float derivative = (error - previous_error) / deltaTime; // Variație eroare (D)
        previous_error = error; // Salvează eroarea curentă

        float output = Kp * error + Ki * integral + Kd * derivative; // Calcul PID total
        output = constrain(output, 100, 150);  // Limitează output-ul
        int viteza = map(output, 100, 150, 100, 180);

        // Soft-start: modifică gradual viteza
        if (viteza > viteza_anterioara + 5) viteza = viteza_anterioara + 5;
        if (viteza < viteza_anterioara - 5) viteza = viteza_anterioara - 5;

        analogWrite(enb, viteza);  // Aplică viteza calculată
        viteza_anterioara = viteza;
        delay(200);  // Mică întârziere pentru stabilitate
      }
    }

    else if (pompa1_terminata && !pompa2_activata && (millis() - timestamp_pompa1_stop >= 2000)) {
      // După 2 secunde de la oprirea Pompei1, pornește Pompa2
      analogWrite(ena, 100);
      pompa2_activata = true;
    }

    if (pompa2_activata && (distanta2 <= 8.0 || distanta1 >= 20.0)) {
      // Oprește Pompa2 dacă s-a golit/umplut complet
      analogWrite(ena, 0);
      pompa2_activata = false;
      pompa1_terminata = false;
    }
  }

  // Afișare tensiune și curent pe OLED
  afisareOLED(tensiune, curent);

  // Trimitere date prin serial (pentru monitorizare externă)
  Serial.print(distanta1, 1); Serial.print(",");
  Serial.print(distanta2, 1); Serial.print(",");
  Serial.print(tensiune, 2); Serial.print(",");
  Serial.println(curent, 1);

  // Trimitere starea pompelor
  int stare1 = 0, stare2 = 0;
  if (mod_funct == "automat") {
    stare1 = viteza_anterioara > 0 ? 1 : 0;
    stare2 = pompa2_activata ? 1 : 0;
  } else if (mod_funct == "manual") {
    stare1 = pompa1_stare;
    stare2 = pompa2_stare;
  }

  Serial.print("STARE:");
  Serial.print(stare1); Serial.print(",");
  Serial.println(stare2);

  delay(1000);  // Pauză între cicluri
}

// Funcție pentru citirea distanței cu senzor ultrasonic
float citireDistanta() {
  digitalWrite(TRIG, LOW); delayMicroseconds(2);
  digitalWrite(TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  long durata = pulseIn(ECHO, HIGH, 30000);  // Timeout 30ms
  return durata * 0.034 / 2.0;  // Conversie la centimetri
}

// Afișează tensiunea și curentul pe OLED
void afisareOLED(float u, float i) {
  display.clearDisplay();
  display.setCursor(0, 10);
  display.print("U: "); display.print(u, 2); display.println(" V");
  display.print("I: "); display.print(i, 1); display.println(" mA");
  display.display();
}

// Afișează mesaj de eroare pe OLED dacă tensiunea e anormală
void afisareAlertaOLED(float tensiune) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 20); display.println("Sistem");
  display.setCursor(0, 45); display.println("oprit!");
  display.display();
}
