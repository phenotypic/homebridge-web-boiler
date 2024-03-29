#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <OpenTherm.h>
#include <DHT.h>
#include <PID_v1.h>
#include <PID_AutoTune_v0.h>

// D2 = INPUT
// D1 = OUTPUT
// D6 = DHT11

/////////////////// CHANGE THESE VALUES //////////////////////
const char* ssid = "SSID"; // Name of your network
const char* password = "PASSWORD"; // Password for your network
const char* mdns = "boiler"; // mDNS name
double Setpoint = 22.0; // Initial heating setpoint
bool targetHeatingCoolingState = false; // Initial heating state
float dhwTargetTemperature = 45.0; // Initial DHW setpoint
bool dhwTargetState = true; // Initial DHW state
double Kp = 32, Ki = 3, Kd = 1; // Heating PID tunings
//////////////////////////////////////////////////////////////

// Pin declarations
const int inPin = 4;
const int outPin = 5;
const int dhtPin = 12;

DHT dht(dhtPin, DHT11);
OpenTherm ot(inPin, outPin);

// PID declarations
double MinTemp, MaxTemp, Input, Output;
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

// PID tuning declarations
double aTuneStep, aTuneStartValue;
const int aTuneLookBack = 30;
const double aTuneNoise = 0.1;
bool tuningState = false;
PID_ATune aTune(&Input, &Output);

// OpenTherm declarations
const bool enableCooling = false;
unsigned long request, response, ts;

// Variable declarations
int currentHeatingCoolingState;
float boilerCurrentTemp, boilerTargetTemp, relativeHumidity;
int dhwCurrentState;
float dhwCurrentTemperature, dhwHigh, dhwLow;

ESP8266WebServer server(80);

// OT setup
void ICACHE_RAM_ATTR handleInterrupt() {
  ot.handleInterrupt();
}

void setup() {

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

  server.on("/status", []() {
    size_t capacity = JSON_OBJECT_SIZE(15);
    DynamicJsonDocument doc(capacity);

    doc["targetHeatingCoolingState"] = targetHeatingCoolingState;
    doc["currentHeatingCoolingState"] = currentHeatingCoolingState;
    doc["targetTemperature"] = Setpoint;
    doc["currentTemperature"] = Input;
    doc["dhwTargetState"] = dhwTargetState;
    doc["dhwCurrentState"] = dhwCurrentState;
    doc["dhwTargetTemperature"] = dhwTargetTemperature;
    doc["dhwCurrentTemperature"] = dhwCurrentTemperature;
    doc["currentRelativeHumidity"] = relativeHumidity;
    doc["boilerTargetTemp"] = boilerTargetTemp;
    doc["boilerCurrentTemp"] = boilerCurrentTemp;
    doc["tuningState"] = tuningState;
    doc["Kp"] = myPID.GetKp();
    doc["Ki"] = myPID.GetKi();
    doc["Kd"] = myPID.GetKd();

    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
  });

  server.on("/diag", []() {
    String message = "==========HEATING==========";
    message += "\nTarget State       : " + String(targetHeatingCoolingState);
    message += "\nCurrent State      : " + String(currentHeatingCoolingState);

    message += "\n\nTarget Temperature : " + String(Setpoint);
    message += "\nCurrent Temperature: " + String(Input);

    message += "\n\nMinimum Temperature: " + String(MinTemp);
    message += "\nMaximum Temperature: " + String(MaxTemp);
    message += "\n============DHW============";
    message += "\nTarget State       : " + String(dhwTargetState);
    message += "\nCurrent State      : " + String(dhwCurrentState);

    message += "\n\nTarget Temperature : " + String(dhwTargetTemperature);
    message += "\nCurrent Temperature: " + String(dhwCurrentTemperature);

    message += "\n\nMinimum Temperature: " + String(dhwLow);
    message += "\nMaximum Temperature: " + String(dhwHigh);
    message += "\n===========OTHER===========";
    message += "\nBoiler Target      : " + String(boilerTargetTemp);
    message += "\nBoiler Temperature : " + String(boilerCurrentTemp);

    message += "\n\nRequesting Cooling : " + String(enableCooling);

    message += "\n\nRelative Humidity  : " + String(relativeHumidity);
    message += "\n===========TUNE============";
    message += "\nTuning state       : " + String(tuningState);

    message += "\n\nKp                 : " + String(myPID.GetKp());
    message += "\nKi                 : " + String(myPID.GetKi());
    message += "\nKd                 : " + String(myPID.GetKd());
    message += "\n===========================\n";
    server.send(200, "text/plain", message);
  });

  server.on("/targetHeatingCoolingState", []() {
    targetHeatingCoolingState = server.arg("value").toInt();
    server.send(200);
  });

  server.on("/targetTemperature", []() {
    Setpoint = server.arg("value").toFloat();
    server.send(200);
  });

  server.on("/dhwTargetState", []() {
    dhwTargetState = server.arg("value").toInt();
    server.send(200);
  });

  server.on("/dhwTargetTemperature", []() {
    dhwTargetTemperature = server.arg("value").toFloat();
    server.send(200);
  });

  server.on("/tuningState", []() {
    tuningState = server.arg("value").toInt();
    if (tuningState) {
      Output = MaxTemp;
    } else {
      aTune.Cancel();
    }
    server.send(200);
  });

  // Start the server
  server.begin();

  dht.begin();

  ts = millis();
  ot.begin(handleInterrupt);

  // Get central heating bounds
  request = ot.buildRequest(
    OpenThermRequestType::READ,
    OpenThermMessageID::MaxTSetUBMaxTSetLB,
    0x0000
  );

  do {
    delay(5000);
    response = ot.sendRequest(request);
  } while (!ot.isValidResponse(response));

  MaxTemp = (response & 0xff00) >> 8;
  MinTemp = response & 0x00ff;

  Serial.println();
  Serial.println("Heating upper bound: " + String(MaxTemp));
  Serial.println("Heating lower bound: " + String(MinTemp));

  myPID.SetMode(AUTOMATIC);
  myPID.SetOutputLimits(MinTemp, MaxTemp);

  aTuneStep = (MaxTemp - MinTemp) / 2;

  aTune.SetNoiseBand(aTuneNoise);
  aTune.SetOutputStep(aTuneStep);
  aTune.SetLookbackSec(aTuneLookBack);

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
}

void loop() {

  server.handleClient();
  MDNS.update();

  // Get DHT11 data
  Input = dht.readTemperature();
  relativeHumidity = dht.readHumidity();

  // Get and set boiler parameters
  if (millis() - ts > 1000) {
    ts = millis();
    if (targetHeatingCoolingState) {
      if (tuningState) {
        byte val = (aTune.Runtime());
        if (val != 0) {
          tuningState = false;
        }
        if(!tuningState) {
          Kp = aTune.GetKp();
          Ki = aTune.GetKi();
          Kd = aTune.GetKd();
          myPID.SetTunings(Kp, Ki, Kd);
        }
      } else {
        myPID.Compute();
      }
      boilerTargetTemp = Output;
    } else {
      boilerTargetTemp = MinTemp;
      aTune.Cancel();
      tuningState = false;
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
      dhwCurrentTemperature = ot.getFloat(response);
      // Set DHW temperature
      unsigned int data = ot.temperatureToData(dhwTargetTemperature);
      request = ot.buildRequest(
        OpenThermRequestType::WRITE,
        OpenThermMessageID::TdhwSet,
        data
      );
      ot.sendRequest(request);
    } else {
      Serial.println("Invalid boiler response: " + String(response, HEX));
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
  Serial.println("Boiler Target      : " + String(boilerTargetTemp));
  Serial.println("Boiler Temperature : " + String(boilerCurrentTemp));
  Serial.println();
  Serial.println("Requesting Cooling : " + String(enableCooling));
  Serial.println();
  Serial.println("Relative Humidity  : " + String(relativeHumidity));
  Serial.println("===========TUNE============");
  Serial.println("Tuning state       : " + String(tuningState));
  Serial.println();
  Serial.println("Kp                 : " + String(myPID.GetKp()));
  Serial.println("Ki                 : " + String(myPID.GetKi()));
  Serial.println("Kd                 : " + String(myPID.GetKd()));
  Serial.println("===========================");

}
