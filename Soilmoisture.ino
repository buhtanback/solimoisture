#include "painlessMesh.h"
#include "esp_timer.h"

#define SOIL_MOISTURE_ANALOG_PIN 34  // Analog pin to which AO is connected

#define MESH_PREFIX     "buhtan"
#define MESH_PASSWORD   "buhtan123"
#define MESH_PORT       5555

Scheduler userScheduler; // to control your personal task
painlessMesh mesh;

unsigned long previousMillis = 0;  // Variable to store the time of the last update
const long interval = 3600000;     // Interval between data transmissions (in milliseconds) (1 hour)
int previousMoisture = -1;         // Variable to store the previous soil moisture value
String lastMessage = "";           // Variable to store the last message

void readSoilMoisture(); // Prototype so PlatformIO doesn't complain

Task taskReadSoilMoisture(TASK_SECOND * 1, TASK_FOREVER, &readSoilMoisture);

// Callback function for receiving messages
void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
}

// Callback for new connections
void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New Connection, nodeId = %u\n", nodeId);
  if (lastMessage != "") {
    bool sendStatus = mesh.sendBroadcast(lastMessage); // Send the last stored message
    Serial.print("Resend status: ");
    Serial.println(sendStatus ? "Success" : "Failed"); // Print the send status
    if (sendStatus) {
      lastMessage = ""; //АClear the buffer if the message was sent successfully
    }
  }
}

// Callback for changed connections
void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

// Callback for adjusted time
void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

// Function to read data from the sensor and output the result to the serial monitor
void readSoilMoisture() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval || previousMillis == 0) {
    previousMillis = currentMillis;

    int analogValue = analogRead(SOIL_MOISTURE_ANALOG_PIN);

    Serial.print("Аналогове значення вологості ґрунту: ");
    Serial.println(analogValue);

    if (analogValue > 2000) {
      Serial.println("Ґрунт сухий.");
    } else {
      Serial.println("Ґрунт вологий.");
    }

    if (analogValue != previousMoisture) { // Send data only when it changes
      String meshMsg = "01" + String(analogValue); // Form the message to send over the PainlessMesh network

      if (mesh.getNodeList().size() > 0) { // Check if there are any connected nodes
        bool sendStatus = mesh.sendBroadcast(meshMsg); // Send the message over the PainlessMesh network
        Serial.print("Send status: ");
        Serial.println(sendStatus ? "Success" : "Failed"); // Print the send status
        if (!sendStatus) {
          lastMessage = meshMsg; // Store the message in the buffer if sending failed
        }
        Serial.print("Message sent: ");
        Serial.println(meshMsg);
      } else {
        lastMessage = meshMsg; // Store the message in the buffer if there are no connected nodes
      }

      previousMoisture = analogValue; // Update the previous soil moisture value
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Setting up mesh network...");

  // Pin initialization
  pinMode(SOIL_MOISTURE_ANALOG_PIN, INPUT);

  // Wait 2 seconds to initialize the serial monitor
  unsigned long startTime = millis();
  while (millis() - startTime < 2000) {
    // Empty waiting loop
  }

  // Initialize the mesh network
  mesh.setDebugMsgTypes(ERROR | STARTUP); // Set basic debug message types
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(& receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  // Add and enable the task
  userScheduler.addTask(taskReadSoilMoisture);
  taskReadSoilMoisture.enable();

  Serial.println("Mesh network setup complete.");
  
  // Read soil moisture and send data immediately after setup
  readSoilMoisture();
}

void loop() {
  mesh.update(); // Update the mesh network
}
