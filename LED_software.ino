#include <WiFi.h>
#include <PubSubClient.h>


// Update these with values suitable for your network.

const char* ssid = "BRRC";
const char* password = "91567512";
const char* mqtt_server = "192.168.1.104";
const int freq = 8000;
const int resolution = 8;

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0, color[3], state = 0;
float brightness = 0, ctemp = 0, cool = 0, warm = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// callback is called whenever a new message is received from mqtt broker
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  //converting payload to string then to float

  payload[length] = '\0';           //string end character
  String s = String((char*)payload);
  float f = s.toFloat();
  //ON/OFF----------------------------------------
  if (strcmp(topic, "home/lights/on_off") == 0) {
    if (strcmp((char*)payload, "ON") == 0)
      state = 1;
    if (strcmp((char*)payload, "OFF") == 0)
      state = 0;
  }
  if (state) {

    //BRIGHTNESS------------------------------------

    if (strcmp(topic, "home/lights/level") == 0) {
      brightness = f;
    }

    //TEMPERATURE-----------------------------------

    // making white temperatures be maximum in the middle and max out in the ends
    if (strcmp(topic, "home/lights/ctemp") == 0) {
      ctemp = f;
    }

    if ( ctemp <= 50 )
    {
      warm = map(ctemp, 0, 50, 0, brightness);
      cool = brightness;
    }
    if ( ctemp > 50 )
    {
      cool = map(ctemp, 51, 100, brightness, -1);
      warm = brightness;
    }

    ledcWrite(0, warm);//ledcWrite(ledChannel, dutyCycle) - the pins are now refferd to through their channels
    ledcWrite(1, cool);

    //COLOR-----------------------------------------

    if (strcmp(topic, "home/lights/color") == 0) {

      // creating substrings from color valueas and converting them to int

      color[0] = map(s.substring(0, s.indexOf(',')).toInt(), 0, 250, 0, 255);
      color[1] = map(s.substring(s.indexOf(',') + 1, s.lastIndexOf(',')).toInt(), 0, 250, 0, 255);
      color[2] = map(s.substring(s.lastIndexOf(',') + 1).toInt(), 0, 250, 0, 255);

    }
    ledcWrite(2, color[0]);
    ledcWrite(3, color[1]);
    ledcWrite(4, color[2]);
    Serial.print("RED: ");
    Serial.println(color[0]);
    Serial.print("GREEN: ");
    Serial.println(color[1]);
    Serial.print("BLUE: ");
    Serial.println(color[2]);

    Serial.println();
  }
  else {
    ledcWrite(0, 0);
    ledcWrite(1, 0);
    ledcWrite(2, 0);
    ledcWrite(3, 0);
    ledcWrite(4, 0);
  }
}




void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // ... and resubscribe
      //client.subscribe("home/LED/inTopic");
      client.subscribe("home/lights/on_off");
      client.subscribe("home/lights/level");
      client.subscribe("home/lights/ctemp");
      client.subscribe("home/lights/color");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  // configure LED PWM functionalitites
  ledcSetup(0, freq, resolution);
  ledcSetup(1, freq, resolution);
  ledcSetup(2, freq, resolution);
  ledcSetup(3, freq, resolution);
  ledcSetup(4, freq, resolution);


  // attach the channel to the GPIO to be controlled
  ledcAttachPin(13, 0);
  ledcAttachPin(12, 1);
  ledcAttachPin(14, 2);
  ledcAttachPin(27, 3);
  ledcAttachPin(26, 4);
  Serial.begin(9600);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  //  unsigned long now = millis();
  //  if (now - lastMsg > 2000) {
  //    lastMsg = now;
  //    ++value;
  //    snprintf (msg, MSG_BUFFER_SIZE, "hello world #%ld", value);
  //    Serial.print("Publish message: ");
  //    Serial.println(msg);
  //    client.publish("home/LED/outTopic", msg);
  //  }
}
