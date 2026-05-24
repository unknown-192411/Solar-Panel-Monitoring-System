/*
 * ============================================================
 *  IoT-Based Solar Panel Monitoring System
 * ============================================================
 *  Hardware  : ESP32 DevKit
 *  Sensors   : DHT11 (Temp/Humidity), BH1750 (Light), Voltage Sensor (0-25V)
 *  Display   : 16x2 I2C LCD
 *  Cloud     : ThingSpeak via HTTP GET
 * ------------------------------------------------------------
 *  Project   : RGIT Mumbai
 *  Guide     : Dr. S.V. Kulkarni
 * ============================================================
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <BH1750.h>

// -------------------- WiFi Credentials --------------------
// Add up to 4 networks; the system will try each in order
const int networkCount = 4;
const char* ssids[networkCount]     = {"SSID1", "SSID2", "SSID3", "SSID4"};
const char* passwords[networkCount] = {"PASS1", "PASS2", "PASS3", "PASS4"};
const unsigned long connectionTimeout = 5000; // ms per network attempt

void connectWiFi() {
  bool connected = false;

  for (int i = 0; i < networkCount; i++) {
    Serial.print("Attempting to connect to ");
    Serial.print(ssids[i]);
    Serial.println("...");

    WiFi.disconnect();
    delay(500);
    WiFi.begin(ssids[i], passwords[i]);

    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < connectionTimeout) {
      delay(500);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnected successfully!");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      connected = true;
      break;
    } else {
      Serial.print("\nFailed to connect to ");
      Serial.println(ssids[i]);
    }
  }

  if (!connected) {
    Serial.println("Failed to connect to any network.");
  }
}

// -------------------- ThingSpeak API --------------------
// Replace with your own ThingSpeak Write API Key
const char* apiKey     = "YOUR_THINGSPEAK_API_KEY";
const char* serverName = "http://api.thingspeak.com/update";

// -------------------- Sensor Pin Definitions --------------------
#define DHT_PIN      27   // DHT11 data pin
#define DHT_TYPE     DHT11
#define VOLTAGE_PIN  35   // Analog input from voltage sensor
#define I2C_SDA      21
#define I2C_SCL      22

// -------------------- Initialize Sensors & Display --------------------
DHT              dht(DHT_PIN, DHT_TYPE);
BH1750           lightMeter;
LiquidCrystal_I2C lcd(0x27, 16, 2);

// -------------------- Read Temperature --------------------
float readTemperature() {
  float temp = dht.readTemperature();
  if (isnan(temp)) {
    Serial.println("Failed to read temperature from DHT sensor!");
    return -999;
  }
  return temp;
}

// -------------------- Read Humidity --------------------
float readHumidity() {
  float hum = dht.readHumidity();
  if (isnan(hum)) {
    Serial.println("Failed to read humidity from DHT sensor!");
    return -999;
  }
  return hum;
}

// -------------------- Read Voltage --------------------
// Voltage sensor divides input by 5 (built-in voltage divider)
// ESP32 ADC: 12-bit (0-4095) mapped to 0-3.3V
float readVoltage() {
  int   analogValue      = analogRead(VOLTAGE_PIN);
  float measuredVoltage  = analogValue * (3.3 / 4095.0);
  float actualVoltage    = measuredVoltage * 5; // Restore original voltage
  return actualVoltage;
}

// -------------------- Read Light Intensity --------------------
float readLightIntensity() {
  return lightMeter.readLightLevel(); // Returns lux (lx)
}

// -------------------- Send Data to ThingSpeak --------------------
void sendDataToThingSpeak(float temperature, float humidity, float voltage, float light) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String url = String(serverName) +
                 "?api_key=" + apiKey +
                 "&field1="  + String(temperature) +
                 "&field2="  + String(humidity) +
                 "&field3="  + String(voltage) +
                 "&field4="  + String(light);

    http.begin(url);
    int httpResponseCode = http.GET();

    Serial.print("ThingSpeak Response: ");
    Serial.println(httpResponseCode);

    http.end();
  } else {
    Serial.println("WiFi not connected — data not sent.");
  }
}

// -------------------- Update LCD Display --------------------
// Rotates through 3 screens, each shown for 3 seconds
void updateLCD(float temperature, float humidity, float voltage, float light) {

  // Screen 1: Temperature & Humidity
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temperature);
  lcd.print("C");
  lcd.setCursor(0, 1);
  lcd.print("Hum: ");
  lcd.print(humidity);
  lcd.print("%");
  delay(3000);

  // Screen 2: Voltage
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Volt: ");
  lcd.print(voltage);
  lcd.print("V");
  delay(3000);

  // Screen 3: Light Intensity
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Light: ");
  lcd.print(light);
  lcd.print(" lx");
  delay(3000);
}

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  connectWiFi();

  // Initialize I2C and LCD
  Wire.begin(I2C_SDA, I2C_SCL);
  lcd.init();
  lcd.backlight();

  // Splash Screen 1: Project title
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("IOT Based Solar");
  lcd.setCursor(0, 1);
  lcd.print("Panel Monitoring");
  delay(3000);

  // Splash Screen 2: Wi-Fi status
  lcd.clear();
  lcd.setCursor(0, 0);
  if (WiFi.status() == WL_CONNECTED) {
    lcd.print("WiFi: Connected");
  } else {
    lcd.print("WiFi: Not Conn.");
  }
  delay(2000);

  // Initialize sensors
  dht.begin();
  lightMeter.begin();
}

// -------------------- Main Loop --------------------
void loop() {
  // Read all sensors
  float temperature    = readTemperature();
  float humidity       = readHumidity();
  float voltage        = readVoltage();
  float lightIntensity = readLightIntensity();

  // Print to Serial Monitor
  Serial.print("Temperature: ");    Serial.print(temperature);    Serial.println(" °C");
  Serial.print("Humidity: ");       Serial.print(humidity);       Serial.println(" %");
  Serial.print("Voltage: ");        Serial.print(voltage);        Serial.println(" V");
  Serial.print("Light Intensity: "); Serial.print(lightIntensity); Serial.println(" lx");

  // Update LCD and send to cloud
  updateLCD(temperature, humidity, voltage, lightIntensity);
  sendDataToThingSpeak(temperature, humidity, voltage, lightIntensity);

  // Wait 20 seconds before next reading
  // (ThingSpeak free tier requires minimum 15s between updates)
  delay(20000);
}
