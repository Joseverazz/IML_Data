# Ejemplo STA (cliente) con mensajes JSON por MQTT

## I. Configuración WiFi

1.  **Crear el WiFiClient.**
2.  **Función `initWifi()`:**
    * Poner en modo STA (Station).
    * Comenzar con SSID y contraseña.
    * Esperar hasta que `WiFi.status()` sea `WL_CONNECTED`.
    * Verificar IP: `WiFi.localIP().toString()`.

## II. Cliente MQTT

1.  **Crear el cliente MQTT a través de WiFi (Ethernet).**
2.  **Función `initMQTT()`:**
    * Configurar servidor `setServer` (Host, Puerto).
    * Payload es JSON pero debe ser `string`.
    * Configurar `callback` para ser suscriptor.
    * Configurar la conexión:
        * `mqttClient.connect(nombre)`
    * Declarar los temas a los que se va a suscribir (`.subscribe()`).

## III. LED y JSON

* **LED:**
    * `pinMode(38, OUTPUT)`
    * `rgbLed.write(R, G, B)`
* **JSON:**
    * `JSON_DOCUMENT`

## IV. Función Callback

* Recibe todos los mensajes publicados.
* Ejecuta la lógica del programa.
* Tipo: `deserializeJson(json, payload) : string` -> `Object`
* `serializeJson(json, var) : JSON` -> `string`
* `doc["texto"]`

## V. Bucle (Loop)

1.  `mqttClient.loop()` (**IMPORTANTE**)
2.  `mqttClient.publish(topic, payload)`:
    * Declarar variables `doc` y `payload`.
```http://googleusercontent.com/image_generation_content/0
