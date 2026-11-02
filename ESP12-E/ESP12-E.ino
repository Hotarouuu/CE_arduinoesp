#include <PubSubClient.h>
#include <ArduinoJson.h>

// Variaveis Globais
extern PubSubClient client;       // Tells the compiler that 'client' is defined in another file
extern const char* mqtt_server;   // Tells the compiler that 'mqtt_server' is defined in another file
JsonDocument doc;

// Define the pins for RS485 communication
#define RE 21
#define DE 19
#define RX2 16
#define TX2 17
#define LED 2 // Led para indicar conexão ao wifi

 
// Request frame for the soil sensor
const byte soilSensorRequest[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x07, 0x04, 0x08};
byte soilSensorResponse[19]; // Cria um vetor contendo 9 bytes
// SoftwareSerial mod(12, 13); // Software serial for RS485 communication / Pino 2 para Receiver e Pino 3 para Transmitter
 
 
void setup()
{
  Serial.begin(115200); // Initializ e serial communication for debugging
  // mod.begin(4800);    // Initialize software serial communication at 4800 baud rate

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  
  // Initialize ESP32 hardware Serial2 on pins 12 (RX) and 13 (TX)
  Serial2.begin(4800, SERIAL_8N1, RX2, TX2);
  
  pinMode(RE, OUTPUT); // Set RE pin as output
  pinMode(DE, OUTPUT); // Set RE pin as output

  pinMode(LED, OUTPUT);

}
 
 
void loop()
{

  reconnect();
  client.loop();
  
  // Clear incoming buffer before sending a new request to prevent old data interference
  while(Serial2.available() > 0) { Serial2.read(); }

  // Start the transmission mode for RS485
  digitalWrite(RE, HIGH);
  digitalWrite(DE, HIGH);

  delay(10);
 
  // Send the request frame to the soil sensor
  Serial2.write(soilSensorRequest, sizeof(soilSensorRequest));
  
  // Wait until all bytes are physically transmitted before flipping RE/DE pins
  Serial2.flush();
 
  // End the transmission mode and set to receive mode for RS485
  digitalWrite(RE, LOW);
  digitalWrite(DE, LOW);

  delay(10);
 
  // Wait for the response from the sensor or timeout after 1 second
  unsigned long startTime = millis();
  while (Serial2.available() < 19 && millis() - startTime < 1000)
  {
    delay(1);
  }
 
  if (Serial2.available() >= 19) // If valid response received
  {
    // Read the response from the sensor
    byte index = 0;
    while (Serial2.available() && index < 19)
    {
      soilSensorResponse[index] = Serial2.read();
     // Serial.print(soilSensorResponse[index], HEX); // Print the received byte in HEX format
     // Serial.print(" ");
      index++;
    }
    Serial.println();
 
    // Parse and calculate the Moisture value
    int Moisture_Int = (int)(soilSensorResponse[3] << 8 | soilSensorResponse[4]);
    float Moisture_Percent = Moisture_Int / 10.0;
 
    Serial.print("Moisture: ");
    Serial.print(Moisture_Percent);
    Serial.println(" %RH");
 
    // Parse and calculate the Temperature value
    int Temperature_Int = (int)(soilSensorResponse[5] << 8 | soilSensorResponse[6]);
    float Temperature_Celsius = Temperature_Int / 10.0;
 
    // Check if temperature is negative and convert accordingly
    if (Temperature_Int > 0x7FFF)
    {
      Temperature_Celsius = 0x10000 - Temperature_Int;
      Temperature_Celsius = -Temperature_Celsius / 10.0;
    }
 
    Serial.print("Temperature: ");
    Serial.print(Temperature_Celsius);
    Serial.println(" °C");

    // Parse and calculate the Conductivity value
    int Conductivity = (int)(soilSensorResponse[7] << 8 | soilSensorResponse[8]);
 
    Serial.print("Conductivity: ");
    Serial.print(Conductivity);
    Serial.println(" us/cm");

    // Parse and calculate the Conductivity value
    int nitrogen = (int)(soilSensorResponse[11] << 8 | soilSensorResponse[12]);
 
    Serial.print("Nitrogen: ");
    Serial.print(nitrogen);
    Serial.println(" mg/kg");

    // Parse and calculate the Conductivity value
    int phosphorus = (int)(soilSensorResponse[13] << 8 | soilSensorResponse[14]);
 
    Serial.print("phosphorus: ");
    Serial.print(phosphorus);
    Serial.println(" mg/kg");

    // Parse and calculate the Conductivity value
    int potassium = (int)(soilSensorResponse[15] << 8 | soilSensorResponse[16]);
 
    Serial.print("Potassium: ");
    Serial.print(potassium);
    Serial.println(" mg/kg");

    // Criação do Payload para enviar ao broker

    doc["moisture"].add(Moisture_Percent);
    doc["temperature"].add(Temperature_Celsius);
    doc["conductivity"].add(Conductivity);
    doc["nitrogen"].add(nitrogen);
    doc["phosphorus"].add(phosphorus);
    doc["potassium"].add(potassium);

    String jsonString;

    serializeJson(doc, jsonString);

    // Envia ao broker
    client.publish("ce_project/sensor/payload", jsonString.c_str());


  }
  else
  {
    // Print error message if no valid response received
    Serial.println("Sensor timeout or incomplete frame");
  }
 
  delay(1000); // Wait for a second before the next loop iteration
}
