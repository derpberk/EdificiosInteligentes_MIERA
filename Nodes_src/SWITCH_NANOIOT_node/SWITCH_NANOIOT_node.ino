#include <WiFiNINA.h>
#include <PubSubClient.h>


#define TM 500 /* Milisegundos entre transmision */

/* Definiciones de los nombres de los topics de comunicacion */


#define MQTT_TOPIC_OUT_COMMON "public_topic"
#define MQTT_TOPIC_OUT_STATE "0000000002/Switch/state"

/* Lets introduce our password and ssid */
const char* ssid = "Redmi";        // your network SSID (name)
const char* pass = "123456789";    // your network password (use for WPA, or use as key for WEP)
const char* mqttServer = "localhost";
const char* common_topic_msg = MQTT_TOPIC_OUT_COMMON;

int status = WL_IDLE_STATUS;
long lastMsg = 0;
char msg[50];
int value = 0;


WiFiClient wifiClient;
PubSubClient client(wifiClient);

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void setup() {

  /* Declaramos el LED como salida */
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  /* Inicializamos el puerto serie para depuracion */
  Serial.begin(9600);

  /* Verificamos si esta disponible el modulo WiFi */
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Modulo WiFi no disponible");
    /* Debug exit */
    while (true);
  }
  
  /* Comprobamos que el firmware esta actualizado*/
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Actualice el software, por favor");
  }

  /* Nos conectamos al WiFi */

  while (status != WL_CONNECTED) {
    Serial.print("Tratando de conectar a la red: ");
    Serial.println(ssid);
    /* Nos conectamos al SSID */
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }

  /* Imprimimos los datos de la red */
  printWifiStatus();
  /* Activamos el LED para verificar que estamos conectados a la red*/
  digitalWrite(LED_BUILTIN, HIGH);
 
  /* Nos conectamos al cliente MQTT */
  client.setServer(mqttServer, 1883);
  client.setCallback(callback);
  
}

void reconnect() 
{
  /* Esperamos a conectarnos */
  while (!client.connected()) 
  {
    Serial.print("Attempting MQTT connection...");
    /* Creamos un ID aleatorio para evitar duplicidades */
    String clientId = "ArduinoClient-";
    clientId += String(random(0xffff), HEX);

    /* Intentamos la conexion */
    if (client.connect(clientId.c_str())) 
    {
      Serial.println("connected");
      // ... nos suscribimos a los topics
      /* Escribir aqui los topics */
      client.subscribe("topic_de_entrada");
    } else 
    {
      Serial.print("ERROR, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");

      /* Esperamos 5 segundos para volver a intentarlo */
      delay(5000);
    }
  }
}

int read_switch(void){

  int state_now;
  int state_last;

  state_now = digitalRead(21);

  if( state_now != state_last){

    delay(25);

    if(digitalRead(21) != state_now){
      state_now = state_last;
    }
    
  }

  return state_now
}

void loop() 
{
  /* Verificamos la conexion */
  if (!client.connected()) 
  {
    reconnect();
  }
  client.loop();

  /* Tomamos el tiempo actual */
  long now = millis();

  /* Leemos el estado del switch */
  if (now - lastMsg > TM) 
  { 
    /* Actualizamos el tiempo */
    lastMsg = now;
    
    if(read_switch() == 1){
      snprintf(msg,5,"%d",1);
    }else
    {
      snprintf(msg,5,"%d",0);
    }

    client.publish(MQTT_TOPIC_OUT_STATE, msg);
    client.publish(MQTT_TOPIC_OUT_COMMON, common_topic_msg);

  }
}
