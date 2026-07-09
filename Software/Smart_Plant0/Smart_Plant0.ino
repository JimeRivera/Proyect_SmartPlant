#include <WiFi.h>
#include <SPI.h>
#include <Wire.h>                 // Librería para I2C (BME280)
#include <Adafruit_Sensor.h>      // Librería de sensor (BME280)
#include <Adafruit_BME280.h>      // Librería BME280
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"

//=====================
// PANTALLA
//=====================
#define TFT_CS   5
#define TFT_DC   4
#define TFT_RST  -1

Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_RST);

//=====================
// DATOS WIFI
//=====================
#define WIFI_SSID "TIGO-2A92"        
#define WIFI_PASS "4NJ667301571"

//=====================
// ADAFRUIT IO
//=====================
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883

#define AIO_USERNAME    "Jimeriv"
#define AIO_KEY         "aio_gYnK91l4BLJ4plCtQ72eMm3lhvFD" 

//=====================
// SENSORES
//=====================
// Sensor Suelo
#define PIN_SENSOR 34
int lecturaADC = 0;
int humedadSueloVal = 0;

// Sensor BME280
#define SDA_PIN 33
#define SCL_PIN 32
Adafruit_BME280 bme;
float temperaturaVal = 0.0;
float humedadAmbienteVal = 0.0;

//=====================
// MQTT CLIENT & FEEDS
//=====================
WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// Feeds
Adafruit_MQTT_Publish feedHumedadSuelo = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/humedad_suelo");
Adafruit_MQTT_Publish feedTemperatura  = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/temperatura");
Adafruit_MQTT_Publish feedHumedadAmb   = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/humedad_ambiente");

//=====================
// DECLARACIÓN DE FUNCIONES
//=====================
void conectarMQTT();
void dibujarPantalla();

//=====================
// SETUP
//=====================
void setup() {
  Serial.begin(115200);

  // Iniciar Pantalla
  tft.begin();
  tft.setRotation(0);

  Serial.println();
  Serial.println("Conectando al WiFi...");

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  // Iniciar I2C y BME280
  Wire.begin(SDA_PIN, SCL_PIN);
  
  if (!bme.begin(0x76) && !bme.begin(0x77)) {
    Serial.println("No se encontró el sensor BME280. Revisa las conexiones!");
  } else {
    Serial.println("BME280 inicializado correctamente.");
  }
}

//=====================
// LOOP
//=====================
void loop() {
  conectarMQTT();

  // 1. Lectura del Suelo
  lecturaADC = analogRead(PIN_SENSOR);
  humedadSueloVal = map(lecturaADC, 4095, 850, 0, 100); // Calibración
  humedadSueloVal = constrain(humedadSueloVal, 0, 100);

  // 2. Lectura del BME280
  temperaturaVal = bme.readTemperature();
  humedadAmbienteVal = bme.readHumidity();

  // 3. Imprimir en Pantalla (Diseño Nuevo)
  dibujarPantalla();

  // 4. Imprimir en Consola
  Serial.print("Suelo: "); 
  Serial.print(humedadSueloVal); 
  Serial.print("% | Temp: "); 
  Serial.print(temperaturaVal); 
  Serial.print(" C | Aire: "); 
  Serial.print(humedadAmbienteVal); 
  Serial.println(" %");

  // 5. Publicar en Adafruit IO
  feedHumedadSuelo.publish((int32_t)humedadSueloVal);
  feedTemperatura.publish(temperaturaVal);
  feedHumedadAmb.publish(humedadAmbienteVal);

  // 6. Esperar 10 segundos para no saturar Adafruit IO
  delay(10000); 
}

//=====================
// CONECTAR MQTT
//=====================
void conectarMQTT() {
  int8_t ret;
  if (mqtt.connected()) return;

  Serial.print("Conectando a Adafruit IO... ");
  while ((ret = mqtt.connect()) != 0) {
    Serial.println(mqtt.connectErrorString(ret));
    mqtt.disconnect();
    delay(5000);
  }
  Serial.println("Conectado!");
}

//=====================
// NUEVO DISEÑO DE PANTALLA
//=====================
void dibujarPantalla() {
  tft.fillScreen(GC9A01A_BLACK); // Fondo negro

  int16_t x1, y1; 
  uint16_t w, h;

  // 1. Título "SmartPlant"
  tft.setTextColor(GC9A01A_GREEN);
  tft.setTextSize(2);
  String titulo = "SmartPlant";
  tft.getTextBounds(titulo, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((240 - w) / 2, 15); // Centrado en X, arriba en Y
  tft.print(titulo);

  // 2. Etiqueta "Humedad Suelo"
  tft.setTextColor(GC9A01A_WHITE);
  tft.setTextSize(2);
  String etiqueta = "Humedad Suelo";
  tft.getTextBounds(etiqueta, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((240 - w) / 2, 45); 
  tft.print(etiqueta);

  // 3. Porcentaje Principal (Super Grande)
  tft.setTextSize(6); 
  String valorSuelo = String(humedadSueloVal) + "%";
  tft.getTextBounds(valorSuelo, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((240 - w) / 2, 75); 
  tft.print(valorSuelo);

  // 4. Estado de Alerta
  tft.setTextSize(2);
  String alerta;
  if (humedadSueloVal < 30) {
    tft.setTextColor(GC9A01A_RED);
    alerta = "Agregar Agua";
  }
  else if (humedadSueloVal < 70) {
    tft.setTextColor(GC9A01A_GREEN);
    alerta = "Ideal";
  }
  else {
    tft.setTextColor(GC9A01A_BLUE);
    alerta = "Muy Humedo";
  }
  tft.getTextBounds(alerta, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((240 - w) / 2, 135); 
  tft.print(alerta);

  // 5. Línea Divisora Horizontal
  tft.drawLine(30, 165, 210, 165, GC9A01A_WHITE);

  // 6. Temperatura del Aire
  tft.setTextColor(GC9A01A_WHITE);
  tft.setTextSize(2);
  String textoTemp = "Temp: " + String(temperaturaVal, 1) + "C";
  tft.getTextBounds(textoTemp, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((240 - w) / 2, 180); 
  tft.print(textoTemp);

  // 7. Humedad del Aire
  String textoAire = "Aire: " + String(humedadAmbienteVal, 1) + "%";
  tft.getTextBounds(textoAire, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((240 - w) / 2, 205); 
  tft.print(textoAire);
}
