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
double Setpoint = 22; //Initial heating setpoint
bool targetHeatingCoolingState = false; //Initial heating state
bool dhwTargetState = true; //Initial DHW state
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
bool enableCooling = false;
unsigned long ts = 0, new_ts = 0;
OpenTherm ot(inPin, outPin);
unsigned long request, response;

// Variable declarations
int currentHeatingCoolingState;
float boilerCurrentTemp, boilerTargetTemp, ambientTemp, relativeHumidity;
int dhwCurrentState;
float dhwCurrentTemperature, dhwTargetTemperature, dhwHigh, dhwLow;

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

  // Get domestic hot water bounds
  request = ot.buildRequest(
    OpenThermRequestType::READ,
    OpenThermMessageID::TdhwSetUBTdhwSetLB,
    0x0000
  );
  response = ot.sendRequest(request);
  dhwHigh = (response & 0xff00) >> 8;
  dhwLow = response & 0x00ff;
  Serial.println();
  Serial.println("DHW upper bound: " + String(dhwHigh));
  Serial.println("DHW lower bound: " + String(dhwLow));

  // Get DHW set point
  request = ot.buildRequest(
    OpenThermRequestType::READ,
    OpenThermMessageID::TdhwSet,
    0x0000
  );
  response = ot.sendRequest(request);
  dhwTargetTemperature = ot.getTemperature(response);
  Serial.println();
  Serial.println("DHW set point: " + String(dhwTargetTemperature));
}

void loop() {

  // Get DHT11 data
  Input = dht.readTemperature();
  relativeHumidity = dht.readHumidity();

  // Get and set boiler parameters
  new_ts = millis();
  if (new_ts - ts > 1000) {
    ts = new_ts;
    if (targetHeatingCoolingState) {
      myPID.Compute();
      boilerTargetTemp = Output;
    } else {
      boilerTargetTemp = MinTemp;
    }
    response = ot.setBoilerStatus(targetHeatingCoolingState, dhwTargetState, enableCooling);
    if (ot.isValidResponse(response)) {
      currentHeatingCoolingState = ot.isCentralHeatingActive(response);
      boilerCurrentTemp = ot.getBoilerTemperature();
      ot.setBoilerTemperature(boilerTargetTemp);

      dhwCurrentState = ot.isHotWaterActive(response);
      // Get DHW temperature
      request = ot.buildRequest(
        OpenThermRequestType::READ,
        OpenThermMessageID::Tdhw,
        0x0000
      );
      response = ot.sendRequest(request);
      dhwCurrentTemperature = ot.getTemperature(response);
      // Set DHW temperature
      unsigned int data = ot.temperatureToData(dhwTargetTemperature);
      request = ot.buildRequest(
        OpenThermRequestType::WRITE,
        OpenThermMessageID::TdhwSet,
        data
      );
      ot.sendRequest(request);
    } else {
      Serial.println("Error: Invalid boiler response " + String(response, HEX));
    }
  }

  Serial.println();
  Serial.println();
  Serial.println("==========HEATING==========");
  Serial.println("Target State       : " + String(targetHeatingCoolingState));
  Serial.println("Current State      : " + String(currentHeatingCoolingState));
  Serial.println();
  Serial.println("Target Temperature : " + String(Setpoint));
  Serial.println("Current Temperature: " + String(Input));
  Serial.println();
  Serial.println("Minimum Temperature: " + String(MinTemp));
  Serial.println("Maximum Temperature: " + String(MaxTemp));
  Serial.println("============DHW============");
  Serial.println("Target State       : " + String(dhwTargetState));
  Serial.println("Current State      : " + String(dhwCurrentState));
  Serial.println();
  Serial.println("Target Temperature : " + String(dhwTargetTemperature));
  Serial.println("Current Temperature: " + String(dhwCurrentTemperature));
  Serial.println();
  Serial.println("Minimum Temperature: " + String(dhwLow));
  Serial.println("Maximum Temperature: " + String(dhwHigh));
  Serial.println("===========OTHER===========");
  Serial.println("Boiler Target     : " + String(boilerTargetTemp));
  Serial.println("Boiler Temperature: " + String(boilerCurrentTemp));
  Serial.println();
  Serial.println("Requesting Cooling: " + String(enableCooling));
  Serial.println();
  Serial.println("Relative Humidity : " + String(relativeHumidity));
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

  if (request.indexOf("/dhwTargetState") != -1) {
    dhwTargetState = request.substring(20, 21).toInt();
  }

  if (request.indexOf("/dhwTargetTemperature") != -1) {
    dhwTargetTemperature = request.substring(26, 28).toFloat();
  }

  if (request.indexOf("/status") != -1) {
    client.println("{\"targetHeatingCoolingState\": " + String(targetHeatingCoolingState) + ",");
    client.println("\"currentHeatingCoolingState\": " + String(currentHeatingCoolingState) + ",");
    client.println("\"targetTemperature\": " + String(Setpoint) + ",");
    client.println("\"currentTemperature\": " + String(Input) + ",");

    client.println("\"dhwTargetState\": " + String(dhwTargetState) + ",");
    client.println("\"dhwCurrentState\": " + String(dhwCurrentState) + ",");
    client.println("\"dhwTargetTemperature\": " + String(dhwTargetTemperature) + ",");
    client.println("\"dhwCurrentTemperature\": " + String(dhwCurrentTemperature) + ",");

    client.println("\"currentRelativeHumidity\": " + String(relativeHumidity) + "}");
  }

  if (request.indexOf("/diag") != -1) {
    client.println("==========HEATING==========");
    client.println("Target State       : " + String(targetHeatingCoolingState));
    client.println("Current State      : " + String(currentHeatingCoolingState));
    client.println();
    client.println("Target Temperature : " + String(Setpoint));
    client.println("Current Temperature: " + String(Input));
    client.println();
    client.println("Minimum Temperature: " + String(MinTemp));
    client.println("Maximum Temperature: " + String(MaxTemp));
    client.println("============DHW============");
    client.println("Target State       : " + String(dhwTargetState));
    client.println("Current State      : " + String(dhwCurrentState));
    client.println();
    client.println("Target Temperature : " + String(dhwTargetTemperature));
    client.println("Current Temperature: " + String(dhwCurrentTemperature));
    client.println();
    client.println("Minimum Temperature: " + String(dhwLow));
    client.println("Maximum Temperature: " + String(dhwHigh));
    client.println("===========OTHER===========");
    client.println("Boiler Target     : " + String(boilerTargetTemp));
    client.println("Boiler Temperature: " + String(boilerCurrentTemp));
    client.println();
    client.println("Requesting Cooling: " + String(enableCooling));
    client.println();
    client.println("Relative Humidity : " + String(relativeHumidity));
    client.println("===========================");
  }

  delay(1);
  Serial.println("Client disconnected");
  Serial.println();

}
