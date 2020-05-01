#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServerSecure.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <OpenTherm.h>
#include <DHT.h>
#include <PID_v1.h>
#include <PID_AutoTune_v0.h>

// GitHub Page = https://github.com/Tommrodrigues/homebridge-web-boiler

// D2 = INPUT
// D1 = OUTPUT
// D6 = DHT11

/////////////////// CHANGE THESE VALUES //////////////////////
const char* ssid = "SSID"; // Name of your network
const char* password = "PASSWORD"; // Password for your network
const char* mdns = "boiler"; // mDNS name
const char* requestKey = "passsword"; // Request key
double Setpoint = 22.0; // Initial heating setpoint
bool targetHeatingCoolingState = false; // Initial heating state
float dhwTargetTemperature = 45.0; // Initial DHW setpoint
bool dhwTargetState = true; // Initial DHW state
double Kp = 32, Ki = 3, Kd = 1; // Heating PID tunings
//////////////////////////////////////////////////////////////

BearSSL::ESP8266WebServerSecure server(80);

static const char serverCert[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----

-----END CERTIFICATE-----
)EOF";

static const char serverKey[] PROGMEM =  R"EOF(
-----BEGIN RSA PRIVATE KEY-----

-----END RSA PRIVATE KEY-----
)EOF";

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
float boilerCurrentTemp, boilerTargetTemp, ambientTemp, relativeHumidity;
int dhwCurrentState;
float dhwCurrentTemperature, dhwHigh, dhwLow;

// OT setup
void ICACHE_RAM_ATTR handleInterrupt() {
  ot.handleInterrupt();
}

void setup(void){
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin(mdns)) {
    Serial.println("MDNS responder started");
  }

  server.setRSACert(new BearSSL::X509List(serverCert), new BearSSL::PrivateKey(serverKey));

  server.on("/status", [](){
    if (server.arg("requestKey") == requestKey) {
      const size_t capacity = JSON_OBJECT_SIZE(15);
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
      server.send(200, "text/plain", json);
    } else {
      server.send(200);
    }
  });

  server.on("/targetHeatingCoolingState", [](){
    if (server.arg("requestKey") == requestKey) {
      targetHeatingCoolingState = server.arg("value").toInt();
    }
    server.send(200);
  });

  server.on("/targetTemperature", [](){
    if (server.arg("requestKey") == requestKey) {
      Setpoint = server.arg("value").toFloat();
    }
    server.send(200);
  });

  server.on("/dhwTargetState", [](){
    if (server.arg("requestKey") == requestKey) {
      dhwTargetState = server.arg("value").toInt();
    }
    server.send(200);
  });

  server.on("/tuningState", [](){
    if (server.arg("requestKey") == requestKey) {
      tuningState = server.arg("value").toInt();
      if (tuningState) {
        Output = MaxTemp;
      } else {
        aTune.Cancel();
      }
    }
    server.send(200);
  });

  server.begin();
  Serial.println("HTTPS server started");

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
}

void loop(void){
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
      Serial.println("Invalid boiler response: " + String(response, HEX));
    }
  }

  server.handleClient();
  MDNS.update();
}
