Conținut repository:
- COD_ESP1.ino – codul pentru senzorul ultrasonic și transmiterea datelor prin MQTT
- COD_ESP2.ino – cod pentru transmisia datelor către ESP1
- COD_ARDUINO.ino – codul pentru controlul pompelor și citirea senzorilor
- NodeRED_flow.json – exportul flow-ului Node-RED folosit pentru monitorizare și comenzi

Pași de compilare:
- Fișierele se deschid în Arduino IDE 
- Pentru ESP-uri:
  - Se instalează driverul „CP210x USB to UART Bridge VCP”
  - Se selectează placa: NodeMCU 1.0 (ESP-12E Module)
- Pentru Arduino UNO:
  - Se selectează placa „Arduino Uno” și portul corespunzător din meniu

- Se descarcă următoarele biblioteci din Library Manager:
  - `ESP8266WiFi`
  - `PubSubClient`
  - `Adafruit_INA219`
  - `SoftwareSerial` (implicit inclusă)

- Se încarcă fiecare cod corespunzător prin USB:
  - COD_ESP1 pe ESP1
  - COD_ESP2 pe ESP2
  - COD_ARDUINO pe placa Arduino Uno

Pași de instalare și lansare:
- Dispozitivele ESP trebuie conectate la aceeași rețea Wi-Fi
- Brokerul MQTT (ex: Mosquitto) trebuie să fie pornit pe o mașină locală (ex: PC)
- Node-RED trebuie să ruleze și să aibă importat fișierul `NodeRED_flow.json`
