#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <DHT.h>

// D7 = Relay
// D6 = DHT11

/////////////////// CHANGE THESE VALUES //////////////////////
const char* ssid = "SSID"; // Name of your network
const char* password = "PASSWORD"; // Password for your network
double Setpoint = 22.0; // Initial heating setpoint
double margin = 0.5; // Temperature hysteresis
double stopFreeze = 10.0; // Antifreeze trigger temperature
bool targetHeatingCoolingState = false; // Initial heating state
const String relay = "HIGH"; // Relay type (`HIGH` or `LOW`)
const char* mdns = "boiler"; // mDNS name
//////////////////////////////////////////////////////////////

const int relayPin = 13;
const int dhtPin = 12;

DHT dht(dhtPin, DHT11);

double upperLim, lowerLim, Input;
float relativeHumidity;
bool currentHeatingCoolingState = targetHeatingCoolingState;
int relayOn, relayOff;
unsigned long ts = 0;

ESP8266WebServer server(80);

void setup() {
  upperLim = Setpoint + margin;
  lowerLim = Setpoint - margin;

  if (relay.equals("LOW")) {
    relayOn = 0;
    relayOff = 1;
  } else {
    relayOn = 1;
    relayOff = 0;
  }

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, relayOff);

  Serial.begin(115200);
  delay(10);

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.println("Connecting to \"" + String(ssid) + "\"");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(String(++i) + " ");
  }
  Serial.println();
  Serial.println("Connected successfully");

  // Print the IP address
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (!MDNS.begin(mdns)) {
    Serial.println("Error setting up MDNS responder!");
  }
  Serial.println("mDNS address: " + String(mdns) + ".local");

  server.on("/targetHeatingCoolingState", []() {
    targetHeatingCoolingState = server.arg("value").toInt();
    server.send(200);
  });

  server.on("/targetTemperature", []() {
    Setpoint = server.arg("value").toFloat();
    upperLim = Setpoint + margin;
    lowerLim = Setpoint - margin;
    server.send(200);
  });

  server.on("/status", []() {
   size_t capacity = JSON_OBJECT_SIZE(7);
   DynamicJsonDocument doc(capacity);

   doc["targetHeatingCoolingState"] = targetHeatingCoolingState;
   doc["currentHeatingCoolingState"] = currentHeatingCoolingState;
   doc["targetTemperature"] = Setpoint;
   doc["currentTemperature"] = Input;
   doc["upperLim"] = upperLim;
   doc["lowerLim"] = lowerLim;
   doc["currentRelativeHumidity"] = relativeHumidity;

   String json;
   serializeJson(doc, json);
   server.send(200, "application/json", json);
 });

  // Start the server
  server.begin();

  // Initialise temperature sensor
  dht.begin();
  Input = dht.readTemperature();
}

//Main loop
void loop() {
  if (millis() - ts > 300000) {
    ts = millis();
    Input = dht.readTemperature();
  }
  relativeHumidity = dht.readHumidity();

  server.handleClient();
  MDNS.update();

  if (targetHeatingCoolingState) {
    if (Input <= lowerLim){
      digitalWrite(relayPin, relayOn);
      currentHeatingCoolingState = 1;
    }
    if (Input >= upperLim) {
      digitalWrite(relayPin, relayOff);
      currentHeatingCoolingState = 0;
    }
  } else if (Input < stopFreeze) {
    digitalWrite(relayPin, relayOn);
    currentHeatingCoolingState = 1;
  } else {
    digitalWrite(relayPin, relayOff);
    currentHeatingCoolingState = 0;
  }
}
