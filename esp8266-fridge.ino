#include <ArduinoJson.h>
#include <DallasTemperature.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <OneWire.h>
#include <ThingSpeak.h>
#include <WiFiManager.h>

#define PIN_DS18B20 2
#define PIN_RELAY   4

#define HTTP_SERVER_PORT   80

#define CONFIG_FILE "config.json"

struct Config {
  uint8_t  mode;
  float_t  temperature;
  float_t  hysteresis;
  uint8_t  interval;
  uint32_t tsChannelId;
  String   tsWriteApiKey;
};

#define RANGE_TEMP_MIN      1.0
#define RANGE_TEMP_MAX      25.0
#define RANGE_HYST_MIN      0.1
#define RANGE_HYST_MAX      2.0
#define RANGE_INTERVAL_MIN  1
#define RANGE_INTERVAL_MAX  60

#define DEFAULT_MODE        2
#define DEFAULT_TEMP        13.0
#define DEFAULT_HYST        0.5
#define DEFAULT_INTERVAL    1

const std::vector<String> Modes = {"OFF", "ON", "AUTO"};

#define LOOP_PERIOD_MS  1000 // 1 sec

uint32_t startMillis, currentMillis;
uint16_t loopCnt = 0;

void toggle();
void handleRoot();
void loadConfiguration(const String, Config &);
void saveConfiguration(const String, const Config &);
void printFile(const String);
void handleRoot();
void handleConfig();

Config config;
WiFiClient client;

ESP8266WebServer server(HTTP_SERVER_PORT);

OneWire oneWire(PIN_DS18B20);
DallasTemperature sensors(&oneWire);

const String htmlHeader = "<!doctype html>\n" \
  "<html lang='pl_PL'>\n" \
  "<meta charset='utf-8'>\n" \
  "<meta name='viewport' content='width=device-width, initial-scale=1'/>\n" \
  "<style>body {font-family: Sans;}</style>\n";

const String htmlFooter = "</html>\n";

void setup() {
  Serial.begin(115200);
  while (!Serial) continue;
  
  digitalWrite(PIN_DS18B20, LOW);
  pinMode(PIN_DS18B20, OUTPUT);

  digitalWrite(PIN_RELAY, LOW);
  pinMode(PIN_RELAY, OUTPUT);

  sensors.begin();
  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(180);  // try setting up WiFi but ignore and run fridge control regardless of connection status
  wifiManager.autoConnect();
  
  ThingSpeak.begin(client);

  loadConfiguration(CONFIG_FILE, config);
  printFile(CONFIG_FILE);

  server.on("/", handleRoot);
  server.on("/config.html", handleConfig);
  server.begin();

  Serial.printf("Web server started @ %s\n", WiFi.localIP().toString().c_str());
  
  startMillis = millis();

  ESP.wdtDisable();
}

void loop() {
  currentMillis = millis();
  server.handleClient();

  if (currentMillis - startMillis >= LOOP_PERIOD_MS) {
    startMillis = currentMillis;

    if (!(loopCnt % (config.interval * 60))) {
      toggle();
    }
    loopCnt++;
    ESP.wdtFeed();
  }
}

void loadConfiguration(const String filename, Config &config) {
  Serial.println(F("Loading configuration..."));

  while (!SPIFFS.begin()) {
    delay(1000);
  }
  
  File file = SPIFFS.open(filename, "r");

  StaticJsonDocument<256> doc;
  
  DeserializationError error = deserializeJson(doc, file);
  
  if (error)
    Serial.println(F("Failed to read config file"));

  config.mode           = doc["mode"]          | DEFAULT_MODE;
  config.temperature    = doc["temperature"]   | DEFAULT_TEMP;
  config.hysteresis     = doc["hysteresis"]    | DEFAULT_HYST;
  config.interval       = doc["interval"]      | DEFAULT_INTERVAL;
  config.tsChannelId    = doc["tsChannelId"]   | 0;
  config.tsWriteApiKey  = doc["tsWriteApiKey"] | "";
  
  file.close();
  SPIFFS.end();
}

void saveConfiguration(const String filename, const Config &config) {
  Serial.println(F("Saving configuration..."));
  while (!SPIFFS.begin()) delay(1000);

  SPIFFS.remove(filename);   // Delete existing file, otherwise the configuration is appended to the file

  // Open file for writing
  File file = SPIFFS.open(filename, "w");
  if (!file) {
    Serial.println(F("Failed to create file"));
    return;
  }

  StaticJsonDocument<256> doc;

  // Set the values in the document
  doc["mode"]          = config.mode;
  doc["temperature"]   = config.temperature;
  doc["hysteresis"]    = config.hysteresis;
  doc["interval"]      = config.interval;
  doc["tsChannelId"]   = config.tsChannelId;
  doc["tsWriteApiKey"] = config.tsWriteApiKey;

  if (serializeJson(doc, file) == 0) {
    Serial.println(F("Failed to write to file"));
  }

  file.close();

  SPIFFS.end();
}

void printFile(const String filename) {
  while (!SPIFFS.begin()) delay(1000);

  // Open file for reading
  File file = SPIFFS.open(filename, "r");
  if (!file) {
    Serial.println(F("Failed to read file"));
    return;
  }

  // Extract each characters by one by one
  while (file.available()) {
    Serial.print((char)file.read());
  }
  Serial.println();

  // Close the file
  file.close();
  SPIFFS.end();
}

void handleRoot() {
  String html;
  html  = htmlHeader;
  html += "<body>\n";

  boolean configUpdate = false;
  std::vector<String> statusUpdate;
  String statusThingspeak, statusHtml;

  for (uint8_t i = 0; i < server.args(); i++) {
    
    if (String(server.argName(i)) == "mode") {
      uint8_t reqMode =  server.arg(i).toInt();

      if (reqMode < Modes.size() && reqMode != config.mode) {
        config.mode = server.arg(i).toInt();
        statusUpdate.push_back("Mode: " + String(config.mode));
        configUpdate = true;
      }
    }

    if (String(server.argName(i)) == "temp") {
      float_t reqTemp = server.arg(i).toFloat();

      if (reqTemp >= RANGE_TEMP_MIN && reqTemp <= RANGE_TEMP_MAX && reqTemp != config.temperature) {
        config.temperature = reqTemp;
        statusUpdate.push_back("Temp: " + String(config.temperature));
        configUpdate = true;
      }
    }

    if (String(server.argName(i)) == "hyst") {
      float_t reqHysteresis = server.arg(i).toFloat();

      if (reqHysteresis >= RANGE_HYST_MIN && reqHysteresis <= RANGE_HYST_MAX && reqHysteresis != config.hysteresis) {
        config.hysteresis = reqHysteresis;
        statusUpdate.push_back("Hysteresis: " + String(config.hysteresis));
        configUpdate = true;
      }
    }

    if (String(server.argName(i)) == "interval") {
      uint8_t reqInterval = server.arg(i).toInt();

      if (reqInterval >= RANGE_INTERVAL_MIN && reqInterval <= RANGE_INTERVAL_MAX && reqInterval != config.interval) {
        config.interval = reqInterval;
        statusUpdate.push_back("Interval: " + String(config.interval));
        configUpdate = true;
      }
    }

    if (String(server.argName(i)) == "tsChannelId") {
      uint32_t reqTsChannelId = server.arg(i).toInt();

      if (reqTsChannelId > 0 && reqTsChannelId != config.tsChannelId) {
        config.tsChannelId = reqTsChannelId;
        statusUpdate.push_back("ThingSpeak Channel ID: " + String(config.tsChannelId));
        configUpdate = true;
      }
    }

    if (String(server.argName(i)) == "tsWriteApiKey") {
      String reqTsWriteApiKey = server.arg(i);

      if (reqTsWriteApiKey.length() > 0 && reqTsWriteApiKey != config.tsWriteApiKey) {
        config.tsWriteApiKey = reqTsWriteApiKey;
        statusUpdate.push_back("ThingSpeak Write API Key: ***");
        configUpdate = true;
      }
    }
  }

  if (configUpdate) {
    saveConfiguration(CONFIG_FILE, config);

    for (size_t i = 0; i < statusUpdate.size(); i++) {
      statusThingspeak += statusUpdate.at(i) + " ";
      statusHtml += "<p>" + statusUpdate.at(i) + "</p>\n";
    }
    
    ThingSpeak.setStatus(statusThingspeak);
    printFile(CONFIG_FILE);
    toggle();
  }

  sensors.requestTemperatures(); // Send the command to get temperatures
  html += statusHtml;
  html += "<form method='post' style='padding: 20px; box-sizing: border-box;border: 2px solid #ccc;border-radius: 4px;background-color: #f8f8f8;'>\n";
  for (std::size_t i = 0; i < Modes.size(); i++) {
    html += "<label><input type='radio' name='mode' value='" + String(i) + "' " + (i == config.mode ? "checked" : "") + "/> " + Modes.at(i) + "</label>&nbsp;\n";
  }
  html += "<br/>Temperature: \n";
  html += "<input name='temp' value='" + String(config.temperature) + "' size=3/> (" + String(RANGE_TEMP_MIN) + " - " + String(RANGE_TEMP_MAX) + ")<br/>\n";
  html += "Hysteresis: <input name='hyst' value='" + String(config.hysteresis) + "' size=3/> (" + String(RANGE_HYST_MIN) + " - " + String(RANGE_HYST_MAX) + ")<br/>\n";
  html += "Interval: <input name='interval' value='" + String(config.interval) + "' size=3/> (" + String(RANGE_INTERVAL_MIN) + " - " + String(RANGE_INTERVAL_MAX) + ")<br/>\n";
  html += "<input type='submit' value=OK></form>\n";
  html += "<p>Current temp: " + String(sensors.getTempCByIndex(0)) + "</p>\n";
  html += "<p>Cooling pin state: " + String(digitalRead(PIN_RELAY)) + "</p>\n";
  html += "<p><a href='/config.html'>config &raquo;</p>\n";
  html += "</body>\n";
  html += htmlFooter;

  server.send(200, "text/html", html);
}

void handleConfig() {
  String html;
  html  = htmlHeader;
  html += "<body>\n";
  html += "<form method='post' action='/' style='padding: 20px; box-sizing: border-box;border: 2px solid #ccc;border-radius: 4px;background-color: #f8f8f8;'>\n";
  html += "<h3>ThingSpeak settings</h3>";
  html += "Channel ID: <input name='tsChannelId' value='" + String(config.tsChannelId) + "' size=16/><br/>\n";
  html += "Write API Key: <input type='password' name='tsWriteApiKey' value='" + config.tsWriteApiKey + "' size=16/><br/>\n";
  html += "<input type='submit' value=OK></form>\n";
  html += "</form>\n";
  html += "<p><a href='/'>&laquo; back</p>\n";
  html += "</body>\n";
  html += htmlFooter;

  server.send(200, "text/html", html);
}

void toggle() {
  sensors.requestTemperatures();
  float_t currentTemp = sensors.getTempCByIndex(0);

  Serial.println(currentTemp);

  switch (config.mode)
  {
  case 0:
    digitalWrite(PIN_RELAY, LOW);
    break;
  
  case 1:
    digitalWrite(PIN_RELAY, HIGH);
    break;

  case 2:
    if (currentTemp - config.hysteresis / 2 >= config.temperature) {
      digitalWrite(PIN_RELAY, HIGH);
    }
    else if (currentTemp + config.hysteresis / 2 <= config.temperature) {
      digitalWrite(PIN_RELAY, LOW);
    }
    break;
  }

  if (WiFi.status() == WL_CONNECTED) {
    ThingSpeak.setField(1, currentTemp);
    ThingSpeak.setField(2, digitalRead(PIN_RELAY));
    uint16_t tsRes = ThingSpeak.writeFields(config.tsChannelId, config.tsWriteApiKey.c_str());
    
    if (tsRes == 200) {
      Serial.println(F("Channel update successful"));
    }
    else {
      Serial.println("Problem updating channel. HTTP error code " + String(tsRes));
    }
  }
}
