#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define TOPIC "chicken_coop"
#define DEFAULT_MAX_ENCODER_POS 10000

enum states {INITIALIZING, DOOR_OPENING, DOOR_OPEN, DOOR_CLOSING, DOOR_CLOSED, DOOR_STOPPED};

const char* ssid = "<your ssid>";
const char* password =  "<your wifi password>";
const char* clientName = "ChickenCoopClient";
const char* mqttServer = "<your mqtt server>";
const char* mqttUser = "<your mqqt user>";
const char* mqttPassword = "<your mqtt password>";
const int mqttPort = 1883;

states state;

volatile int encoderPos = 0;

WiFiClient espClient;
PubSubClient client(espClient);
 
void setup() {
  state = INITIALIZING;
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(D7, OUTPUT);
  pinMode(D8, OUTPUT);
  pinMode(D1, INPUT_PULLUP);
  pinMode(D5, INPUT_PULLUP);
  pinMode(D6, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(D1), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(D5), reedClosedISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(D6), reedOpenISR, CHANGE);
  
  setupWifi();
 
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  connectMQTT();

  encoderPos = 0;
}
 
void loop() {
  if (!client.connected())
    connectMQTT();
    
  if (state == INITIALIZING && digitalRead(D6) == HIGH) {
    state = DOOR_OPEN;
    client.publish("stat/" TOPIC, "open");
    Serial.println("Door open");
  }
  else if (state == INITIALIZING && digitalRead(D5) == HIGH) {
    state = DOOR_CLOSED;
    client.publish("stat/" TOPIC, "closed");
    Serial.println("Door closed");
  }
  else if (encoderPos < 0 && (state == DOOR_OPENING || state == DOOR_CLOSING)) {
    stopDoor();
    state = DOOR_STOPPED;
    client.publish("stat/" TOPIC, "stopped");
    Serial.println("Maximum distance exceeded. Door stopped");
  }
       
  client.loop();
}

void setupWifi() {
  client.publish("tele/" TOPIC "/LWT", (const uint8_t*) "offline", 7, true);

  Serial.println("");
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.hostname(clientName);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.print(".");
    delay(250);
    digitalWrite(LED_BUILTIN, LOW);
    delay(250);
  }

  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("");
  Serial.println("Connected to the WiFi network");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void connectMQTT() {
  int retries = 0;

  Serial.println("Connecting to MQTT");
  
  while (!client.connected()) {
    if (retries < 1200) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(250);
    
      if (!client.connect(clientName, mqttUser, mqttPassword )) {
        Serial.print("MQTT connect failed with state ");
        Serial.println(client.state());
      }
    
      digitalWrite(LED_BUILTIN, LOW);
      retries++;
      delay(250);
    }
    else {
      ESP.restart();  
    }
  }

  digitalWrite(LED_BUILTIN, HIGH);
  client.publish("tele/" TOPIC "/LWT", (const uint8_t*) "online", 6, true);
  client.subscribe("cmnd/" TOPIC);
  Serial.println("");
  Serial.println("Connected to MQTT broker");  
}

// --------------- MOTOR Functions ---------------
void stopDoor() {
  digitalWrite(D7, LOW);
  digitalWrite(D8, LOW);
}

void closeDoor() {
 state = DOOR_CLOSING;
 digitalWrite(D7, HIGH);
 digitalWrite(D8, LOW);
 client.publish("stat/" TOPIC, "closing");
}

void openDoor() {
 state = DOOR_OPENING;
 digitalWrite(D7, LOW);
 digitalWrite(D8, HIGH);
 client.publish("stat/" TOPIC, "opening");
}

// ------------ Callbacks and ISRs -------------
void callback(char* topic, byte* payload, unsigned int length) {
  String strTopic = topic;
  Serial.print("Message arrived in topic: ");
  Serial.println(strTopic);

  payload[length] = '\0';
  String strPayload = String((char *) payload);
  Serial.println("Payload:");
  Serial.println(strPayload);
  Serial.println("-----------------------");

  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, payload, length);

  if (err.code() == DeserializationError::Ok) {
    const char* action = doc["action"];

    if (action != NULL) {
      long maxPos = doc["max"] > 0 ? doc["max"] : DEFAULT_MAX_ENCODER_POS;

      if (strcmp(action, "OPEN") == 0 && state != DOOR_OPEN) {
        encoderPos = maxPos;
        openDoor();
      }
      else if (strcmp(action, "CLOSE") == 0 && state != DOOR_CLOSED) {
        encoderPos = maxPos;
        closeDoor();
      }
      else if (strcmp(action, "STOP") == 0) {
        stopDoor();
        state = DOOR_STOPPED;
        client.publish("stat/" TOPIC, "stopped");
        Serial.println("Door stopped");
        Serial.print("End encoderPos: ");
        Serial.println(encoderPos);
      }
    }
    else {
      Serial.println("Error action required");  
    }
  }
  else {
    Serial.print("JSON deserialize failed with code ");
    Serial.println(err.code());
  }
}

ICACHE_RAM_ATTR void encoderISR() {
  --encoderPos;
}

ICACHE_RAM_ATTR void reedOpenISR() {
  if (state == DOOR_OPENING && digitalRead(D6) == HIGH) {
    stopDoor();
    state = DOOR_OPEN;
    client.publish("stat/" TOPIC, "open");
    Serial.println("Door open");
  }
}

ICACHE_RAM_ATTR void reedClosedISR() {
  if (state == DOOR_CLOSING && digitalRead(D5) == HIGH) {
    stopDoor();
    state = DOOR_CLOSED;
    client.publish("stat/" TOPIC, "closed");
    Serial.println("Door closed");
  }
}
