#include <ESP8266WiFi.h>         // Bibliotecă pentru conectarea la rețea Wi-Fi
#include <PubSubClient.h>        // Bibliotecă pentru protocolul MQTT
#include <SoftwareSerial.h>      // Bibliotecă pentru comunicația serială suplimentară

SoftwareSerial mySerial(D6, D5);  // D6=RX și D5=TX ESP

// Configurare Wi-Fi și MQTT
const char* ssid = "HUAWEI P smart 2021";       
const char* password = "21dragoni";             
const char* mqtt_server = "192.168.43.179";     
const int mqtt_port = 1884;                     

// Definire topicuri MQTT
const char* topic_recv_dist   = "rezervor2/distance";   // Primește distanța de la ESP2
const char* topic_recv_cmd    = "arduino/comanda";      // Primește comenzi către Arduino
const char* topic_send_data   = "arduino/data";         // Trimite date de la Arduino spre Node-RED
const char* topic_stare_pompe = "arduino/stare";        // Trimite starea pompelor (manual) spre Node-RED

WiFiClient espClient;
PubSubClient client(espClient);

// Funcție callback – executată la primirea mesajelor MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];  // Construiește mesajul caracter cu caracter
  }

  Serial.println("MQTT <- " + String(topic) + ": " + msg);

  if (String(topic) == topic_recv_dist) {
    // Trimite la Arduino distanța cu prefix "DIST:"
    mySerial.print("DIST:");
    mySerial.println(msg);
  }
  else if (String(topic) == topic_recv_cmd) {
    // Trimite comenzile direct către Arduino (ex: MOD:manual, POMPA1:1)
    mySerial.println(msg);
  }
}

// Funcție de reconectare la brokerul MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.print("MQTT...");
    if (client.connect("ESP1Bridge")) {  // ID-ul clientului MQTT
      Serial.println(" conectat.");
      client.subscribe(topic_recv_dist);   // Subscriere la topicul cu distanța
      client.subscribe(topic_recv_cmd);    // Subscriere la comenzile pentru Arduino
    } else {
      Serial.print("eșec, rc=");
      Serial.print(client.state());
      Serial.println(" → reîncerc în 2s");
      delay(2000);  // Așteaptă 2 secunde înainte de a încerca din nou
    }
  }
}

// Setup – inițializare module
void setup() {
  Serial.begin(115200);         // Serial pentru debugging
  mySerial.begin(9600);         // Serial software pentru comunicare cu Arduino

  WiFi.begin(ssid, password);   // Conectare la rețea
  Serial.print("Conectare WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectat. IP: " + WiFi.localIP().toString());

  client.setServer(mqtt_server, mqtt_port);  // Configurare broker MQTT
  client.setCallback(callback);              // Setare funcție callback
}


void loop() {
  if (!client.connected()) reconnect();  // Reautentificare dacă s-a pierdut conexiunea MQTT
  client.loop();                         // Menține conexiunea activă

  // Verifică dacă sunt date venite de la Arduino
  if (mySerial.available()) {
    String linie = mySerial.readStringUntil('\n');  // Citește până la newline
    linie.trim();                                   // Elimină spații și caractere inutile

    if (linie.startsWith("STARE:")) {
      // Dacă e mesaj cu starea pompelor, publică pe topicul corespunzător
      client.publish(topic_stare_pompe, linie.c_str());
      Serial.println("MQTT -> STARE trimis: " + linie);
    }
    else if (linie.length() > 0 && linie.indexOf(',') > 0) {
      // Dacă e un mesaj valid cu date (ex: 12.3,17.8,7.2,56.4)
      client.publish(topic_send_data, linie.c_str());
      Serial.println("MQTT -> DATA trimis: " + linie);
    }
    else {
      // Mesaj invalid – doar pentru afișare/debug
      Serial.println("⚠️ Linie invalidă: " + linie);
    }
  }
}
