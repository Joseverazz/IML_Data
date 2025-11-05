#include <Wire.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
#include "rgb_lcd.h"
#include "Si115X.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#define PIN_SDA 8
#define PIN_SCL 9
#define SI115X_ADDR 0x53 

rgb_lcd lcd;
Si115X si1151;
Adafruit_BME680 bme;  

JsonDocument json;
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
  String html = "";
  html += "n: " + String(json["n"].as<const char*>()) + "<br>";
  html += "id: " + String(json["id"].as<const char*>()) + "<br>";
  html += "description: " + String(json["description"].as<const char*>()) + "<br>";
  html += "timestamp: " + String(json["timestamp"].as<float>()) + "<br>";
  html += "latitude: " + String(json["latitude"].as<float>()) + "<br>";
  html += "longitude: " + String(json["longitude"].as<float>()) + "<br>";
  html += "temperature: " + String(json["temperature"].as<float>()) + "<br>";
  html += "humidity: " + String(json["humidity"].as<float>()) + "<br>";
  html += "atmosphericPressure: " + String(json["atmosphericPressure"].as<float>()) + "<br>";
  html += "illuminance: " + String(json["illuminance"].as<float>()) + "<br>";
  html += "ir: " + String(json["ir"].as<float>()) + "<br>";

  server.send(200, "text/html", html);
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
  delay(1000);
}
 
void loop() {
  
  if (!bme.performReading()) {
    Serial.println("Error al leer datos del sensor");
    return;
  }
  // Lectura de datos
  int now = 0;
  float longitude = -3.703790;
  float latitude = 40.4116775;

  
  json["n"]="Estación meteorológica";
  json["id"]="grupo_05";
  json["description"]= "Estación meteorológica para SAT";
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
  delay(300);  // Esperar 2 segundos antes de la próxima lectura
}