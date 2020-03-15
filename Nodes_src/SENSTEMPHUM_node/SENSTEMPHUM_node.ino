#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// Definimos el pin digital donde se conecta el sensor
#define DHTPIN 4 // GPIO 4 == D2
// Dependiendo del tipo de sensor
#define DHTTYPE DHT11

// Inicializamos el sensor DHT11
DHT dht(DHTPIN, DHTTYPE);

// Update these with values suitable for your network.
const char* mqtt_server = "192.168.1.134";
const char* ssid = "MIWIFI_2G_69FZ";
const char* password = "VHqTa9kt";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void setup() {
  dht.begin(); // Inicia el sensor de temperatura y humedad
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      client.publish("temperatura", "Conectado ESP8266");
      client.publish("humedad", "Conectado ESP8266");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  client.publish("public_topic","0000000001/SensTempHum/temp");
  client.publish("public_topic","0000000001/SensTempHum/hum");
  long now = millis();
  // Mensajes cada x segundos
  if (now - lastMsg > 5000) {
    // Leemos la temperatura en grados centígrados (por defecto)
    float t = dht.readTemperature();
    // Leemos la humedad relativa
    int h = dht.readHumidity();
    snprintf (msg, 75, "%.1f", t);
    Serial.println(msg);
    client.publish("0000000001/SensTempHum/temp", msg);
    snprintf (msg, 75, "%d", (int)h);
    Serial.println(msg);
    client.publish("0000000001/SensTempHum/hum", msg); 
    lastMsg = now;
  }
}
