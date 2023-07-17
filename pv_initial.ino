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

// Firebase references
FirebaseData firebaseData;
FirebaseJson firebaseJson;

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
        // Check if the status is true
        if (firebaseData.boolData()) 
        {
          // Turn the bulb on
          digitalWrite(RELAY_PIN, LOW);
          Serial.println("Bulb turned ON");
        } else 
        {
          // Turn the bulb off
          digitalWrite(RELAY_PIN, HIGH);
          Serial.println("Bulb turned OFF");  
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
