#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#define WIFI_SSID "YvPhone"
#define WIFI_PASSWORD "12345678"
#define LED 5

// Function declarations
String makeGetRequest(const char* endpoint);
String makePostRequest(const char* endpoint, String jsonBody);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nStarting WiFi connection process...");
  
  // Scan for networks
  Serial.println("Scanning for WiFi networks...");
  int n = WiFi.scanNetworks();
  if (n == 0) {
    Serial.println("No networks found!");
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      Serial.printf("%d: %s (%d)\n", i+1, WiFi.SSID(i).c_str(), WiFi.RSSI(i));
      delay(10);
    }
  }
  
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW); // Start with LED off

  // Clear any previous WiFi configuration
  WiFi.disconnect(true);
  delay(1000);
  
  // Connect using WiFi.begin
  WiFi.mode(WIFI_STA);
  Serial.print("Attempting to connect to: ");
  Serial.println(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  unsigned long startAttemptTime = millis();
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    digitalWrite(LED, !digitalRead(LED)); // Blink LED while connecting
    delay(500);
    
    // Timeout after 20 seconds
    if (millis() - startAttemptTime > 20000) {
      Serial.println("\nFailed to connect! Please check:");
      Serial.println("1. Password is correct");
      Serial.println("2. iPhone hotspot is active and accepting connections");
      Serial.println("3. Try enabling 'Maximize Compatibility' in hotspot settings");
      break;
    }
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
}

// Make a GET request to the API
String makeGetRequest(const char* endpoint) {
  HTTPClient http;
  String url = "https://esp32-wifi-api.vercel.app" + String(endpoint);
  
  // Begin HTTP connection
  http.begin(url);
  
  // Send HTTP GET request
  int httpResponseCode = http.GET();
  String payload = "{}"; 
  
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  
  // Free resources
  http.end();
  
  return payload;
}

// Make a POST request with JSON data
String makePostRequest(const char* endpoint, String jsonBody) {
  HTTPClient http;
  String url = "https://esp32-wifi-api.vercel.app" + String(endpoint);
  
  // Begin HTTP connection
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  
  // Send HTTP POST request
  int httpResponseCode = http.POST(jsonBody);
  String payload = "{}";
  
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  
  // Free resources
  http.end();
  
  return payload;
}

void loop() {
  // Indicate WiFi status with LED
  digitalWrite(LED, WiFi.status() == WL_CONNECTED);
  
  // Only make API requests if connected to WiFi
  if (WiFi.status() == WL_CONNECTED) {
    static unsigned long lastRequestTime = 0;
    if (millis() - lastRequestTime > 10000) {  // Every 10 seconds
      lastRequestTime = millis();
      
      // Example: Get data from API
      String response = makeGetRequest("/");
      Serial.println("API Response: " + response);
    }
  }
  
  delay(1000);
}