#include <WiFi.h>
#include <HTTPClient.h>

// Replace with your network credentials
const char* ssid = "mq";  // Your WiFi SSID
const char* password = "Derse321";  // Your WiFi Password

// Google Sheets Web App URL (from your deployed Google Apps Script)
const String serverUrl = "https://script.google.com/macros/s/AKfycbw-YGJjrgdlrs7ZX_U89oTYArWkiqpO0pUXa2RUsgNQ_onCuyXGGxv3lniLcm7INlgv/exec";

// Moisture sensor pin
const int moisturePin = 34;  // GPIO34 for moisture sensor

// Variables to track changes
int previousMoisture = -1;
String previousStatus = "";

// Read the moisture sensor and convert it to percentage
int readMoisture() {
  int sensorValue = analogRead(moisturePin);  // Read sensor value (0 to 4095)
  int moisturePercentage = map(sensorValue, 0, 4095, 0, 100);  // Map to 0-100%
  return moisturePercentage;
}

// Determine the moisture status category
String getStatus(int moisture) {
  if (moisture < 30) return "Too Dry";
  else if (moisture > 70) return "Too Wet";
  else return "Optimal";
}

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  // Print the IP address
  Serial.println("IP Address: " + WiFi.localIP().toString());
}

void loop() {
  int moistureLevel = readMoisture();
  String status = getStatus(moistureLevel);

  Serial.print("Moisture Level: ");
  Serial.print(moistureLevel);
  Serial.print("% - Status: ");
  Serial.println(status);

  bool shouldSend = false;

  // Send only when status is "Too Dry" or "Too Wet"
  if (status == "Too Dry" || status == "Too Wet") {
    // Send if it's the first reading
    if (previousMoisture == -1) {
      shouldSend = true;
    }
    // Send if status zone has changed
    else if (status != previousStatus) {
      shouldSend = true;
    }
    // Send if change in value is significant (>5%)
    else if (abs(moistureLevel - previousMoisture) >= 5) {
      shouldSend = true;
    }
  }

  if (shouldSend && WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String payload = "{\"moisture\": " + String(moistureLevel) + ", \"status\": \"" + status + "\"}";
    
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(payload);

    if (httpResponseCode > 0) {
      Serial.print("POST Response Code: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Error in sending POST request: ");
      Serial.println(httpResponseCode);
    }

    http.end();

    // Update previous values
    previousMoisture = moistureLevel;
    previousStatus = status;
  } else if (!shouldSend) {
    Serial.println("No abnormal change. Data not sent.");
  } else {
    Serial.println("Error: WiFi not connected");
  }

  delay(500);  // Delay 5 seconds (you can adjust this to 5 mins = 300000)
}