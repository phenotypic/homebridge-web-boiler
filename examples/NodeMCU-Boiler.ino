#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <OpenTherm.h>
#include <DHT.h>
#include <PID_v1.h>

// GitHub Page = https://github.com/Tommrodrigues/homebridge-web-boiler

// D2 = INPUT
// D1 = OUTPUT
// D6 = DHT11

/////////////////// CHANGE THESE VALUES //////////////////////
const char* ssid = "SSID"; //Name of your network
const char* password = "PASSWORD"; //Password for your network
const char* mdns = "boiler"; //mDNS name
double Setpoint = 22; //Initial setpoint
int targetHeatingCoolingState = 0; //Initial state
//////////////////////////////////////////////////////////////

// DHT declarations
#define DHTTYPE DHT11
#define DHTPIN 12
DHT dht(DHTPIN, DHTTYPE);

// Pin declarations
const int inPin = 4;
const int outPin = 5;

// Specify the links and initial tuning parameters for PID
double Kp = 10, Ki = 5, Kd = 8;
double MinTemp = 20, MaxTemp = 90;
double Input, Output;
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

// OpenTherm initial declarations
bool enableCentralHeating = false, enableHotWater = true, enableCooling = false;
OpenTherm ot(inPin, outPin);
unsigned long request, response;

// Other declarations
int currentHeatingCoolingState, boilerState;
float boilerCurrentTemp, boilerTargetTemp, ambientTemp, relativeHumidity;
unsigned long ts = 0, new_ts = 0;

WiFiServer server(80);

//OT setup
void ICACHE_RAM_ATTR handleInterrupt() {
  ot.handleInterrupt();
}

void setup() {

  Serial.begin(115200);
  delay(10);

  dht.begin();

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

  // Start the server
  server.begin();

  // Print the IP address
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (!MDNS.begin(mdns)) {
    Serial.println("Error setting up MDNS responder!");
  }
  Serial.println("mDNS address: " + String(mdns) + ".local");

  ts = millis();

  ot.begin(handleInterrupt);

  // Get central heating bounds
  request = ot.buildRequest(
    OpenThermRequestType::READ,
    OpenThermMessageID::MaxTSetUBMaxTSetLB,
    0x0000
  );
  response = ot.sendRequest(request);
  if (ot.isValidResponse(response)) {
    MaxTemp = (response & 0xff00) >> 8;
    MinTemp = response & 0x00ff;
  }

  Serial.println();
  Serial.println("Heating upper bound: " + String(MaxTemp));
  Serial.println("Heating lower bound: " + String(MinTemp));

  myPID.SetMode(AUTOMATIC);
  myPID.SetOutputLimits(MinTemp, MaxTemp);
}

void loop() {

  // Get DHT11 data
  Input = dht.readTemperature();
  relativeHumidity = dht.readHumidity();

  // Get and set boiler parameters
  new_ts = millis();
  if (new_ts - ts > 1000) {
    ts = new_ts;
    if (targetHeatingCoolingState == 0) {
      enableCentralHeating = false;
      boilerTargetTemp = MinTemp;
    } else if (targetHeatingCoolingState == 1) {
      myPID.Compute();
      enableCentralHeating = true;
      boilerTargetTemp = Output;
    } else {
      Serial.println("Invalid targetHeatingCoolingState: " + String(targetHeatingCoolingState));
    }
    response = ot.setBoilerStatus(enableCentralHeating, enableHotWater, enableCooling);
    if (ot.isValidResponse(response)) {
      ot.setBoilerTemperature(boilerTargetTemp);
      boilerCurrentTemp = ot.getBoilerTemperature();
      boilerState = ot.isCentralHeatingActive(response);
    } else {
      Serial.println("Error: Invalid boiler response " + String(response, HEX));
    }
  }

  //Determine state to give to homebridge
  if (boilerState == 1 && boilerCurrentTemp > MinTemp) {
    currentHeatingCoolingState = 1;
  } else {
    currentHeatingCoolingState = 0;
  }

  Serial.println();
  Serial.println();
  Serial.println("===========================");
  Serial.println("Target State : " + String(targetHeatingCoolingState));
  Serial.println("Current State: " + String(currentHeatingCoolingState));
  Serial.println();
  Serial.println("Target Temperature : " + String(Setpoint));
  Serial.println("Current Temperature: " + String(Input));
  Serial.println("Relative Humidity  : " + String(relativeHumidity));
  Serial.println("---------------------------");
  Serial.println("Boiler Status     : " + String(boilerState));
  Serial.println("Boiler Target     : " + String(boilerTargetTemp));
  Serial.println("Boiler Temperature: " + String(boilerCurrentTemp));
  Serial.println();
  Serial.println("Requesting Heating  : " + String(enableCentralHeating));
  Serial.println("Requesting Hot Water: " + String(enableHotWater));
  Serial.println("Requesting Cooling  : " + String(enableCooling));
  Serial.println("===========================");

  MDNS.update();

  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  // Wait until the client sends some data
  Serial.println("New client");
  while (!client.available()) {
    delay(1);
  }

  // Read the first line of the request
  String request = client.readStringUntil('\r');
  Serial.println(request);
  client.flush();

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println();

  // Match the request
  if (request.indexOf("/targetHeatingCoolingState") != -1) {
    targetHeatingCoolingState = request.substring(31, 32).toInt();
  }

  if (request.indexOf("/targetTemperature") != -1) {
    Setpoint = request.substring(23, 25).toFloat();
  }

  if (request.indexOf("/status") != -1) {
    client.println("{\"targetHeatingCoolingState\": " + String(targetHeatingCoolingState) + ",");
    client.println("\"currentHeatingCoolingState\": " + String(currentHeatingCoolingState) + ",");
    client.println("\"targetTemperature\": " + String(Setpoint) + ",");
    client.println("\"currentTemperature\": " + String(Input) + ",");
    client.println("\"currentRelativeHumidity\": " + String(relativeHumidity) + "}");
  }

  if (request.indexOf("/diag") != -1) {
    client.println("===========================");
    client.println("Target State : " + String(targetHeatingCoolingState));
    client.println("Current State: " + String(currentHeatingCoolingState));
    client.println();
    client.println("Target Temperature : " + String(Setpoint));
    client.println("Current Temperature: " + String(Input));
    client.println("Relative Humidity  : " + String(relativeHumidity));
    client.println("---------------------------");
    client.println("Boiler Status     : " + String(boilerState));
    client.println("Boiler Target     : " + String(boilerTargetTemp));
    client.println("Boiler Temperature: " + String(boilerCurrentTemp));
    client.println();
    client.println("Requesting Heating  : " + String(enableCentralHeating));
    client.println("Requesting Hot Water: " + String(enableHotWater));
    client.println("Requesting Cooling  : " + String(enableCooling));
    client.println("===========================");
  }

  delay(1);
  Serial.println("Client disconnected");
  Serial.println();

}
