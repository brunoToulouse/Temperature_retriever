/*
 * Copyright (C) 2025 Bruno Pirrotta
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESPAsyncWebServer.h>
#include <EEPROM.h>
#include <WiFiUdp.h>
#include <regex>

AsyncWebServer server(80);
#define EEPROM_SIZE 512
const int idespIndex=0;
const int ssidIndex=3;
const int passwordIndex=36;
const int mqtthostIndex=68;
const int mqttuserIndex=90;
const int mqttpwdIndex=122;
const int mqtthostportIndex=154;

String idesp = "";
String ssidWifi = "";
String passwordWifi = "";
String mqtthost = "";
String mqtthostport = "";
String mqttuser ="";
String mqttpwd = "";

IPAddress mqtt_server;
u_int16_t mqtt_port;

// these pins are used to connect to the tempSensor
#define PIN_TEMP 2 
#define BLUE_LED 1



WiFiClient espClient;
PubSubClient client(espClient);
String tempTopic;
char msg[25];
OneWire onewire(2);
DallasTemperature sensors(&onewire);
unsigned long lastMsgTemp=0;

const char* ssid    = "ESP8266-Access-Point";
const char* password = "123456789";
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      html {
        font-family: Arial;
        display: inline-block;
        margin: 0px auto;
        text-align: center;
      }
      h2 {
        font-size: 3.0rem;
      }
      p { 
        font-size: 3.0rem;
      }
      .units {
        font-size: 1.2rem;
      }
      .input {
        font-size: 20px;
        margin-bottom: 10px;
      }
      .wifi-form {
        display: flex;
        flex-direction: column;
        align-items: center;
        justify-content: space-around;
      }
      #sentLabel {
        display: none;
        margin-bottom: 10px;
        font-weight: bold;
      }
    </style>
  </head>
  <body onload="getdata()">
    <div class="wifi-form">
      <h1>Configuration capteur de Temperature</h1>
      <label for="idesp">ID EQUIPEMENT</label>
      <input id="idesp" class="input" type="number" maxlength="1">
      <label for="ssid">WIFI ID (SSID)</label>
      <input id="ssid" class="input" type="text" maxlength="32">
      <label for="password">WIFI PASSWORD</label>
      <input id="password" class="input" type="password">
      <label for="mqtthost">MQTT HOST IP</label>
      <input id="mqtthost" class="input" type="text">
      <label for="mqtthostport">MQTT HOST PORT</label>
      <input id="mqtthostport" class="input" type="text" value="1883">
      <label for="mqttuser">MQTT USER</label>
      <input id="mqttuser" class="input" type="text">
      <label for="mqttpwd">MQTT PASSWORD</label>
      <input id="mqttpwd" class="input" type="password">
      <button onclick="connectToWifi()">Save configuration</button>
      <label id="sentLabel">Informations de configuration sauvegardée</label>
    </div>
  </body>
  <script>
    function getdata() {
      fetch('/getdata')
      .then(response => response.text())
      .then(data => { let array = data.split("%");
              document.getElementById('idesp').value = array[0] || "";
              document.getElementById('ssid').value = array[1] || "";
              document.getElementById('password').value = array[2] || "";
              document.getElementById('mqtthost').value = array[3] || "";
              document.getElementById('mqtthostport').value = array[4] || "";
              document.getElementById('mqttuser').value = array[5] || "";
              document.getElementById('mqttpwd').value = array[6] || "";
              });
    }

    function connectToWifi() {
      var idesp = document.getElementById("idesp").value;
      var ssid = document.getElementById("ssid").value;
      var password = document.getElementById("password").value;
      var mqtthost = document.getElementById("mqtthost").value;
      var mqtthostport = document.getElementById("mqtthostport").value;
      var mqttuser = document.getElementById("mqttuser").value;
      var mqttpwd = document.getElementById("mqttpwd").value;
      var xhr = new XMLHttpRequest(); xhr.open("POST", "/", true);
      xhr.setRequestHeader('Content-Type', 'text/plain');
      xhr.send(idesp + "%" + ssid + "%" + password + "%" + mqtthost + "%" + mqtthostport + "%" + mqttuser + "%" + mqttpwd + "%");
      document.getElementById("sentLabel").style.display = "inline";
    }
  </script>
</html>)rawliteral";

IPAddress stringToIP(String str) {
  int a, b, c, d;
  if (sscanf(str.c_str(), "%d.%d.%d.%d", &a, &b, &c, &d) == 4) {
    return IPAddress(a, b, c, d);
  } else {
    return IPAddress(0, 0, 0, 0); // Valeur par défaut en cas d'erreur
  }
}

void writeStringToEEPROM(String str, int addr ) {
  EEPROM.begin(EEPROM_SIZE);
  for (int i = 0; i < str.length(); ++i) {
    EEPROM.write(addr + i, str[i]);
  }
  EEPROM.write(addr + str.length(), '\0'); // Fin de chaîne
  EEPROM.commit();
  EEPROM.end();
}

String readStringFromEEPROM(int addr) {
  EEPROM.begin(EEPROM_SIZE);
  String result = "";
  char ch;
  while ((ch = EEPROM.read(addr++)) != '\0' && addr < EEPROM_SIZE) {
    result += ch;
  }
  EEPROM.end();
  return result;
}

void createAccessPoint() {
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  // Print ESP8266 Local IP Address
  Serial.println(WiFi.localIP());
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });
  server.on("/getdata", HTTP_GET, [](AsyncWebServerRequest *request) {
    String separator ="%";
    String data = idesp + separator +ssidWifi+separator+passwordWifi+separator+ mqtthost + separator+ mqtthostport + separator+ mqttuser + separator + mqttpwd+separator;
    request->send_P(200, "text/plain", data.c_str());
  });

  server.on("/", HTTP_POST, [](AsyncWebServerRequest *request){},NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    idesp = "";
    ssidWifi = "";
    passwordWifi = "";
    mqtthost ="";
    mqtthostport = "";
    mqttuser ="";
    mqttpwd = "";

    String res((char *)data);
    int index1 = res.indexOf('%');
    int index2 = res.indexOf('%', index1 + 1);
    int index3 = res.indexOf('%', index2 + 1);
    int index4 = res.indexOf('%', index3 + 1);
    int index5 = res.indexOf('%', index4 + 1);
    int index6 = res.indexOf('%', index5 + 1);
    int index7 = res.indexOf('%', index6 + 1);
    
    idesp = res.substring(0, index1);
    ssidWifi = res.substring(index1 +1,index2);
    passwordWifi = res.substring(index2 +1, index3);
    mqtthost = res.substring(index3 +1 , index4);
    mqtthostport = res.substring(index4 +1 , index5);
    mqttuser = res.substring(index5+1, index6);
    mqttpwd = res.substring(index6+1,index7);
    
    writeStringToEEPROM(idesp, idespIndex);
    writeStringToEEPROM(ssidWifi, ssidIndex);
    writeStringToEEPROM(passwordWifi, passwordIndex);
    writeStringToEEPROM(mqtthost, mqtthostIndex);
    writeStringToEEPROM(mqtthostport, mqtthostportIndex);
    writeStringToEEPROM(mqttuser, mqttuserIndex);
    writeStringToEEPROM(mqttpwd, mqttpwdIndex);
    
    request->send(200, "text/plain", "SUCCESS");
  });
  server.begin();
}
unsigned long previousBlinkMillis = 0;
unsigned long previousCheckMillis = 0;
const long checkInterval = 120000; // Vérifier le Wi-Fi toutes les 2 minutes (l access point restera accessible 2 min en cas de pb de connexion, avt de retenter la connexion)
bool ledState = LOW;
bool isAPMode = false;
bool wifiLost = false; // Indique si le Wi-Fi a été perdu

void readParameters() {
      ssidWifi = readStringFromEEPROM(ssidIndex);
      passwordWifi = readStringFromEEPROM(passwordIndex);
      idesp = readStringFromEEPROM(idespIndex);
      mqtthost = readStringFromEEPROM(mqtthostIndex);
      mqtthostport= readStringFromEEPROM(mqtthostportIndex);
      mqttuser =  readStringFromEEPROM(mqttuserIndex);
      mqttpwd = readStringFromEEPROM(mqttpwdIndex);
      mqtt_server = stringToIP(mqtthost);
}

void setup_wifi() {
  if (isAPMode) {
    WiFi.softAPdisconnect(true);
    server.end();
    isAPMode = false;
  }

  if (ssidWifi.length() > 0 && passwordWifi.length() > 0 && WiFi.status() != WL_CONNECTED) {
    Serial.println("Tentative de connexion au Wi-Fi...");
    //digitalWrite(LED_BUILTIN, LOW); // Allumer la LED pendant la connexion

    WiFi.begin(ssidWifi, passwordWifi);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      digitalWrite(BLUE_LED, LOW); // Éteint
      delay(100);
      digitalWrite(BLUE_LED, HIGH); // Allume
      delay(600);
      Serial.print(".");
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnexion Wi-Fi réussie !");
      Serial.print("Adresse IP : ");
      Serial.println(WiFi.localIP());
      wifiLost=false;
    } 
    else {
      Serial.println("\nÉchec de connexion, passage en mode point d'accès...");
      createAccessPoint();
      wifiLost=true;
      isAPMode = true;
    }
  } else {
    Serial.println("Aucune information Wi-Fi trouvée, passage en mode point d'accès...");
    createAccessPoint();
    wifiLost=true;
    isAPMode = true;
  }
}

void blinkLED(long unsigned interval) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousBlinkMillis >= interval) {
      previousBlinkMillis = currentMillis;
      ledState = !ledState;  
      digitalWrite(BLUE_LED, ledState);
    }
}

void checkWifi() {

    if (WiFi.status() != WL_CONNECTED) {
      if (!wifiLost) {
        Serial.println("Wi-Fi perdu, tentative de reconnexion...");
        wifiLost = true;
        WiFi.disconnect();
        readParameters();
        setup_wifi();
      }
    } else {
      if (wifiLost) {
        Serial.println("Wi-Fi reconnecté !");
        Serial.println(WiFi.localIP());
        wifiLost = false;
        isAPMode = false;
      }
    }
}

void reconnectMqtt() {
    if (!client.connected()) {
        String clientId = "scr-temp-";
        clientId += String(idesp);
        client.connect(clientId.c_str(),mqttuser.c_str(),mqttpwd.c_str());
    }
  }
   
void setup() {
    Serial.begin(115200);
    pinMode(PIN_TEMP, OUTPUT);
    pinMode(BLUE_LED,OUTPUT);   
    readParameters(); 
    setup_wifi();
    client.setServer(mqtt_server, std::strtoul(mqtthostport.c_str(),nullptr,0));
    reconnectMqtt();

    //setup temperature sensor
    sensors.begin();
    tempTopic = "scr/" + String(idesp) +"/temperature";
}

void loop() {
    unsigned long currentMillis = millis();

    // Vérifier le Wi-Fi en tâche de fond a interval réguliers
    if (currentMillis - previousCheckMillis >= checkInterval) {
      previousCheckMillis = currentMillis;
      checkWifi();
      reconnectMqtt();
    }
  
    // Gestion du clignotement sans perturber le temps réel
    if (isAPMode) {
      blinkLED(100);
    }
    // Si wifi KO
    else if (!wifiLost && !isAPMode) {
      blinkLED(500);
    } 
    //SI WIFI KO
    else if (wifiLost && !isAPMode) {
      blinkLED (1000);
    }
 
    if (client.connected()) {
        client.loop();  
        unsigned long currentMillis = millis();
        if (currentMillis - lastMsgTemp > 10000) {
            lastMsgTemp = currentMillis;
            char MsgTemp[10];
            sensors.requestTemperatures();
            float h = (sensors.getTempCByIndex(0));
            dtostrf(h,4,2,MsgTemp);
            if (h > -120 ) {
            client.publish(tempTopic.c_str(),String(MsgTemp).c_str(),true);
            }
        }
    }
}