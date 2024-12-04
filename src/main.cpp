#include <Arduino.h>

#define WORKSHOP_OTA // comment out this line when deployed to world, or else device searching for wifi network will bootloop.

#ifdef WORKSHOP_OTA
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

const char *ssid = "deathpedals";
const char *password = "";
#endif

// pin numbers and moar
int ledPin = 2;          // onboard LED
int speedSensorPin = 12; // pin D6
int brakePin = 14;       // pin D5
int testPin = 13;        // pin D7
unsigned long previousMillis = 0;
unsigned long deltaTimeMillis;
bool triggerState = 0;

#ifdef WORKSHOP_OTA
// To make Arduino software autodetect OTA device
WiFiServer TelnetServer(8266);
#endif

void IRAM_ATTR speedSensorHandler()
{
  // delay(1); // cheap effective debouncing control? NOPE!
  // Serial.println("Here goes the Handler!");

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis <= deltaTimeMillis)
  {
    digitalWrite(brakePin, HIGH); // brake is activated on controller by grounding brake signal "active low"
    digitalWrite(ledPin, LOW);   // indication, illuminates
    // Serial.println("Brake Trigger!");
    triggerState = 1; // useful bool
  }
  else
  {
    digitalWrite(brakePin, LOW); // turn off brake
    digitalWrite(ledPin, HIGH);   // turn off LED
    // Serial.println("No trigger..");
    triggerState = 0;
  }

  // save the last time the speedSensor triggered.
  previousMillis = currentMillis;
}

void setup()
{
#ifdef WORKSHOP_OTA
  // To make Arduino software autodetect OTA device
  TelnetServer.begin();
#endif

  Serial.begin(115200);
  Serial.println("Booting...");
#ifdef WORKSHOP_OTA
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("esp-wheelspeed");
  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"xxxxx");

  ArduinoOTA.onStart([]()
                     { Serial.println("OTA Start"); });
  ArduinoOTA.onEnd([]()
                   {
    Serial.println("OTA End");
    Serial.println("Rebooting..."); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                        { Serial.printf("Progress: %u%%\r\n", (progress / (total / 100))); });
  ArduinoOTA.onError([](ota_error_t error)
                     {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed"); });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
#endif

  // set LED pin as output
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);

  // set speed sensor pin as input
  pinMode(speedSensorPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(speedSensorPin), speedSensorHandler, FALLING);

  // set brake pin as output
  pinMode(brakePin, OUTPUT);
  digitalWrite(brakePin, LOW); // initial setting

  // set testing pin as low output
  //pinMode(testPin, OUTPUT);
  //digitalWrite(testPin, LOW);

  // wheel diameter in meters
  float wheelDiameter = 0.725;

  // circumerence = pi * D.
  float wheelCircumference = (PI * wheelDiameter);

  // legal speed limit 25kph in meters per second
  float speedLimit = 6.94444;

  // wheel rotation time deltaTime = distance / speed
  float deltaTime = wheelCircumference / speedLimit * 1000.0;

  deltaTimeMillis = deltaTime + 1.0;

  Serial.print("deltaTimeMillis = ");
  Serial.println(deltaTimeMillis);
}

void loop()
{
#ifdef WORKSHOP_OTA
  ArduinoOTA.handle();
  delay(100);
  Serial.println(triggerState);
#endif
  // turn off trigger state if no signal being triggered, just in case.
  if (millis() - previousMillis >= deltaTimeMillis)
  {
    triggerState = 0;
    digitalWrite(ledPin, HIGH);
    digitalWrite(brakePin, LOW);
  }
}
