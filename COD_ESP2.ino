#include <ESP8266WiFi.h>         
#include <PubSubClient.h>        

// Configurare Wi-Fi
const char* ssid = "HUAWEI P smart 2021";  
const char* password = "21dragoni";        

// Configurare MQTT
const char* mqtt_server = "192.168.43.179";  // Adresa IP a brokerului MQTT
const int mqtt_port = 1884;                 // Portul pe care comunică MQTT
const char* mqtt_topic = "rezervor2/distance"; // Topic-ul unde trimitem distanța

// Configurare pinii senzorului ultrasonic HC-SR04
const int trigPin = D5;  // Pinul de trigger – trimite impulsul
const int echoPin = D6;  // Pinul de recepție – primește ecoul

WiFiClient espClient;              // Creează clientul Wi-Fi
PubSubClient client(espClient);    // Creează clientul MQTT folosind conexiunea Wi-Fi

// Funcție pentru conectarea la rețeaua Wi-Fi
void setup_wifi() {
  delay(10);
  Serial.begin(115200);         // Pornim comunicația serială pentru debug
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);   // Începe conectarea la Wi-Fi
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.print("ESP IP: ");
  Serial.println(WiFi.localIP()); // Afișează adresa IP a ESP-ului
}

// Funcție pentru reconectarea la brokerul MQTT dacă s-a deconectat
void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP2DistanceSensor")) { // Numele clientului MQTT
      Serial.println("connected");
    } else {
      Serial.print("failed, code=");
      Serial.print(client.state()); // Codul erorii
      Serial.println(" retrying in 2s...");
      delay(2000);
    }
  }
}

// Funcție care măsoară distanța folosind senzorul ultrasonic
float readDistanceCM() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);       // Trimite un impuls de 10 microsecunde
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 30000); // Măsoară timpul până primește ecoul (timeout 30ms ≈ 5m)
  float distance = duration * 0.0343 / 2.0;       // Convertește timpul în distanță (în cm)
  return distance;
}

void setup() {
  setup_wifi();                        // Conectează ESP-ul la Wi-Fi
  client.setServer(mqtt_server, mqtt_port); // Configurează adresa și portul brokerului MQTT

  pinMode(trigPin, OUTPUT);   // Setează pinul trig ca ieșire
  pinMode(echoPin, INPUT);    // Setează pinul echo ca intrare
}

void loop() {
  if (!client.connected()) {
    reconnect();             // Reconectează la MQTT dacă este deconectat
  }
  client.loop();             // Menține conexiunea MQTT activă

  float distance = readDistanceCM(); // Citește distanța în cm

  if (distance > 2.0 && distance < 400.0) {  // Filtru pentru a evita măsurătorile aberante
    char msg[10];
    dtostrf(distance, 4, 1, msg);           // Conversie float în string cu 1 zecimal

    client.publish(mqtt_topic, msg);       // Trimite distanța pe topicul MQTT
    Serial.print("Distance sent: ");
    Serial.println(msg);
  } else {
    Serial.println("Invalid distance reading"); // În caz că distanța e în afara domeniului
  }

  delay(2000); // Trimite datele la fiecare 2 secunde
}
