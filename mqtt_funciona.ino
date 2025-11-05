#include <Wire.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
#include "rgb_lcd.h"
#include "Si115X.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <NTPClient.h>


#define PIN_SDA 8
#define PIN_SCL 9
#define SI115X_ADDR 0x53 
 
// --------- WiFi ---------- 
const char* SSID_WIFI = "SBC";
const char* PASSWORD = "SBCwifi$";

// --------- Broker MQTT ---------- 
const char* MQTT_HOST = "iot.etsisi.upm.es";
const uint16_t MQTT_PORT = 1883;
const char* GROUP_NAME = "grupo_05";  
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

WiFiUDP ntpUDP;

const int TIMEZONE_OFFSET = 3600; // UTC +1

NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); // UTC, sin offset inicial


rgb_lcd lcd;
Si115X si1151;
Adafruit_BME680 bme;  
 
char* json_telem;
// --------- AP ---------- 
// IP por defecto: 192.168.4.1
const char* SSID = "grupo_05";
 
 
 
// --------- Web ----------
WebServer server(80);  // Puerto: 80 (http)
 
// --------- Página web ----------
const char WEB_PAGE[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html>
<body>
  <h1>Telemetries</h1>
  <p id="telemetry" class="value"></p>
 
  <script>
    const telemetry = document.getElementById('telemetry');
 
    async function getTelemetry() {
      try {
        const res = await fetch('/api/telemetries', { cache: 'no-store' });
        if (!res.ok) throw new Error(`HTTP ${res.status}`);
        telemetry.innerHTML = await res.text();
      } catch (e) {
        console.error(e);
      }
    }
 
    setInterval(getTelemetry, 1000);  // Auto refresh (1 seg)
    getTelemetry();  // First time
  </script>
</body>
</html>
)HTML";
 
// ------- Handlers HTTP ------
void sendTelemetries() {
  server.send(200, "/html", (String) 
    "Telemetry 1: "  + "<br>" + 
    "Telemetry 2: " 
  );
}
 
void initWebServer() {
  server.on("/", []() {server.send(200, "text/html; charset=utf-8", WEB_PAGE);});
  server.on("/api/telemetries", HTTP_GET, sendTelemetries);
 
  server.begin();
}
 
// ------- WiFi AP -------
void initAccessPoint() {
  WiFi.softAP(SSID);
  Serial.println("IP address: " + WiFi.softAPIP().toString());
}

// ------- WiFi STA -------
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID_WIFI, PASSWORD);
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Conectándose a la red Wi-Fi ...");
    delay(1000);
  }
  timeClient.begin();
  Serial.println("\n# Conexión Wi-Fi exitosa #");


  Serial.println("IP address: " + WiFi.localIP().toString());
}

// ------- MQTT -------
void initMqtt(){
  Serial.println("# Conectándose al broker MQTT #");
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  mqttClient.connect(GROUP_NAME);
  while(!mqttClient.connected()){
    Serial.println("Connecting to MQTT Broker ...");
    delay(1000);
  }
  Serial.println("# Conectado al broker de MQTT #");
  Serial.println("# host: " + String(MQTT_HOST));
  Serial.println("# port: " + MQTT_PORT);
  Serial.println("# # # # # # # # # # # # # # # #");

  // Declaración de los Topics a los que se va a suscribir
  mqttClient.subscribe(("iot/" + String(GROUP_NAME) + "/in").c_str());
}

// ------- MQTT Subscription CallBacks -------
void mqttCallback(char* topic, uint8_t* payload, unsigned int length){
  Serial.println("Received msg on: " + String(topic));

  // Parsear JSON
  JsonDocument json;
  deserializeJson(json, payload);

  // Mostrar por puerto serie el JSON
  serializeJson(json, Serial);
  Serial.println("");

  // Lógica del programa
  if(String(topic) == "iot/"+String(GROUP_NAME)+"/in"){

  }
}
 
//------------------- Codigo principal
void setup() {
  Wire.begin(PIN_SDA, PIN_SCL);  
 
  Serial.begin(9600);
  while (!Serial);  // Esperar a que se abra el monitor serial
 
  if (!bme.begin(0x76)) {
    Serial.println("¡No se encontró el sensor BME680 en 0x76!");
    while (1);  
  }
  if (!si1151.Begin(SI115X_ADDR)) {
    Serial.println("Si1151 no está listo!");
    while (1);
 
  }
 
  // Configuración del sensor
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setGasHeater(320, 150);  // 320°C durante 150 ms
 
  lcd.begin(16, 2);             
  lcd.clear();

  initWiFi(); // Iniciar conexión Wifi
  initMqtt(); // Iniciar Servidor Web
  
  delay(1000);
}

unsigned long now;

void loop() {
  if (!bme.performReading()) {
    Serial.println("Error al leer datos del sensor");
    return;
  }

  timeClient.update(); // Actualiza la hora del NTP

  unsigned long epochTime = timeClient.getEpochTime(); // Obtiene la hora en segundos desde 1970
  
  // Ajustar la hora para la zona horaria (aplicando el offset)
  epochTime += TIMEZONE_OFFSET; 

  // Convertir el tiempo epoch ajustado a una estructura tm
  struct tm *ptm = localtime((time_t *)&epochTime); // `localtime` ajusta automáticamente la zona horaria

  // Crear el formato con la fecha y la zona horaria
  char isoTime[30]; // Espacio adicional para zona horaria
  snprintf(isoTime, sizeof(isoTime), "%04d-%02d-%02d %02d:%02d:%02d+01", 
           ptm->tm_year + 1900,   // Año (desde 1900)
           ptm->tm_mon + 1,       // Mes (0-11, por eso sumamos 1)
           ptm->tm_mday,          // Día
           ptm->tm_hour,          // Hora
           ptm->tm_min,           // Minutos
           ptm->tm_sec);          // Segundos
  
  // Convertir a String si es necesario
  String now = String(isoTime);

  float longitude = 40.350;
  float latitude = -3.920;
 
  JsonDocument json;
  json["timestamp"]=now;
  json["latitude"]=latitude;
  json["longitude"]=longitude;
  json["temperature"] = bme.temperature;
  json["humidity"]=bme.humidity;
  json["atmosphericPressure"]=bme.pressure;
  json["illuminance"]=si1151.ReadVisible();
  json["ir"]=si1151.ReadIR();
  // Mostrar datos
  lcd.setCursor(0, 1);
  lcd.print("Visible: ");
  lcd.print(json["illuminance"].as<float>());
  lcd.print(" IR: ");
  lcd.print(json["ir"].as<float>());  
 
 
  lcd.setCursor(0, 0);   // 0 = columna, 1=fila
  lcd.print(" Temp=");
  lcd.print(json["temperature"].as<float>());
  lcd.print("C ");
 
  lcd.print(" Hum=");
  lcd.print(json["humidity"].as<float>());
  lcd.print("% ");
 
  lcd.print(" Pres=");
  lcd.print(json["atmosphericPressure"].as<float>());
  lcd.print("hPa ");
 
  lcd.scrollDisplayLeft();  // Desplaza todo el texto una posición a la izquierda 
 
  Serial.println(json['timestamp'].as<float>());

  mqttClient.loop();

  String payload;
  serializeJson(json,payload);
  mqttClient.publish(("iot/"+String(GROUP_NAME)+"/out").c_str(), payload.c_str());
  delay(300);  // Esperar 2 segundos antes de la próxima lectura
}