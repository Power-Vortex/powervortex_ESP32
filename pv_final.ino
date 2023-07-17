#include <WiFi.h>
#include <FirebaseESP32.h>

// Wi-Fi credentials
const char* ssid = "Your WiFi SSID";
const char* password = "Your WiFi password";

// Firebase configuration
const char* firebaseHost = "your-firebase-host.firebaseio.com";
const char* firebaseAuth = "your-firebase-authentication-token";

// Pin for controlling the bulb
const int RELAY_PIN = 26;

// ACS712 parameters
const int ACS712_PIN = 34;   // Analog input pin connected to ACS712 output
const float ACS712_SCALE = 0.185; // Sensitivity for ACS712-30A module

// Calibration values for ACS712
const float ACS712_ZERO_CURRENT = 2500.0; // ADC value for zero current
const float ACS712_SENSITIVITY = 66.0; // mV/A sensitivity

// Firebase references
FirebaseData firebaseData;
FirebaseJson firebaseJson;

// Power consumption variables
float current;  // Current in amperes
float powerConsumed;  // Power consumed in watts
float energyConsumed;  // Energy consumed in watt-hours
unsigned long startTime;  // Start time of the current 10-second interval

void setup() 
{
  // Initialize serial communication
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi!");

  // Initialize Firebase
  Firebase.begin(firebaseHost, firebaseAuth);

  // Set up the bulb pin as OUTPUT
  pinMode(RELAY_PIN, OUTPUT);

  // Initialize power consumption variables
  current = 0.0;
  powerConsumed = 0.0;
  energyConsumed = 0.0;
  startTime = 0;

  // Retrieve the previous energy consumed value from Firebase
  if (Firebase.getFloat(firebaseData, "/boards/abcabcab1/1/energy")) 
  {
    if (firebaseData.dataType() == "float") 
    {
      energyConsumed = firebaseData.floatData();
    }
  }
}

void loop() 
{
  // Keep the Firebase connection alive
  if (Firebase.ready()) 
  {
    if (Firebase.get(firebaseData, "/boards/abcabcab1/1/status")) 
    {
      // Check if the status is available and is a boolean value
      if (firebaseData.dataType() == "boolean") 
      {
        // Check if the status is true (bulb is on)
        if (firebaseData.boolData()) 
        {
          // Check if the 10-second interval has started
          if (startTime == 0) 
          {
            startTime = millis();
            powerConsumed = 0.0;  // Reset power consumed for the new interval
          }
          // Turn the bulb on
          digitalWrite(RELAY_PIN, LOW);
          Serial.println("Bulb turned ON");

          // Measure current using ACS712
          int sensorValue = analogRead(ACS712_PIN);
          current = ((sensorValue - ACS712_ZERO_CURRENT) * 5.0) / (1024.0 * ACS712_SENSITIVITY);

          // Calculate power consumed
          powerConsumed += (current * 230.0); // Assuming 230V AC mains voltage

          // Check if the 10-second interval has elapsed
          if (millis() - startTime >= 10000) 
          {
            // Update energy consumed
            energyConsumed += (powerConsumed / (1000.0 * 3600.0));

            Serial.print("Power consumed (10s interval): ");
            Serial.print(powerConsumed);
            Serial.println(" Watts");

            Serial.print("Energy consumed: ");
            Serial.print(energyConsumed);
            Serial.println(" Watt-hours");

            // Send energy consumed values to Firebase
            Firebase.setFloat(firebaseData, "/boards/abcabcab1/1/consumption", abs(energyConsumed));

            // Reset variables for the next interval
            powerConsumed = 0.0;
            startTime = 0;
          }
        } 
        else 
        {
          // Turn the bulb off  
          digitalWrite(RELAY_PIN, HIGH);
          Serial.println("Bulb turned OFF");
          // No consumption when bulb is off
          // Send the previous energy consumed value to Firebase
          Firebase.setFloat(firebaseData, "/boards/abcabcab1/1/consumption", abs(energyConsumed));
        }
      }
    }
  }

  // Handle any errors or disconnections
  if (firebaseData.httpCode() != 200) 
  {
    Serial.print("Firebase error: ");
    Serial.println(firebaseData.errorReason());
    Firebase.reconnectWiFi(true);
  }
  
}