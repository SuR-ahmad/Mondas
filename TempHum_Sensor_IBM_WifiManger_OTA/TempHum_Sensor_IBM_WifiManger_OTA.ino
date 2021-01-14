#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <stdlib.h>
#include <DNSServer.h>

#include <ESP8266WebServer.h>
#include <WiFiManager.h>

#define DHTPIN 4
#define DHTTYPE DHT22
#define device_type "TempHum_Sensor"
#define device_id "HashimsRoom"
#define TOKEN "iu*qag!GG8v+25@JDT"
#define ORG "0qlmiq"

const char* Current_Status = "Offline";
float current_temperature = 0.0;
float current_humidity = 0.0;
const char* Local_IP = "255.255.255.255";

char mqtt_server[] = ORG ".messaging.internetofthings.ibmcloud.com";
const int port= 8883;
char event[] = "iot-2/evt/Status/fmt/JSON";
char reading[] ="iot-2/evt/Readings/fmt/JSON";
char command[] = "iot-2/cmd/Ping/fmt/JSON";
char username[] = "use-token-auth";
char password[] = TOKEN;
char clientId[] = "d:" ORG ":" device_type ":" device_id;
const char willTopic[]="iot-2/evt/Will/fmt/string";
int willQoS = 1;
int willRetain = 1;
const char* willMessage="Disconnected";

DHT dht(DHTPIN, DHTTYPE);
WiFiClientSecure espClient;
PubSubClient client(mqtt_server,port,espClient);


void publishstatus(){
  String PayL = "{\"Status\":\"";
  PayL += Current_Status;
  PayL += "\"}";
  client.publish(event,(char*)PayL.c_str());
}

void publishreadings()
{

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  //float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    Current_Status="Sensor Fault";
    publishstatus();
  }
  

  if (current_temperature != t || current_humidity!=h)
  {
      char Temp[10];
      dtostrf(t, 3, 1, Temp);  //3 is mininum width, 1 is precision

      char Humidity[10];
      dtostrf(h, 3, 1, Humidity);  //3 is mininum width, 1 is precision
  
  
       String PayL = "{\"Temperature\":\"";
       PayL += Temp;
       PayL += "\",\"Humidity\":\"";
       PayL += Humidity;
       PayL += "\"}";
       client.publish(reading,(char*)PayL.c_str());
       current_temperature = t;
  }

  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.print(" *C \n");
  
}

void callback(char* topic, byte* message, unsigned int length) {

  Serial.println("Message Arrived");
  String ctopic=topic;
  if (ctopic==command)        //every command is treated as a Ping
  {
    Current_Status="Connected";
    publishstatus();
    publishreadings();
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting to Connect to the MQTT Server..");   
    
    if (client.connect(clientId,username,password,willTopic,willQoS,willRetain,willMessage)) // connect to IBM IoT Service with will message
    {
      Current_Status = "Online";
      publishstatus();                  // publish status
      publishreadings();                // publish readings
      client.subscribe(command);        // ... and resubscribe
    } 
    else 
    {
      Serial.print(".");
      delay(5000);
    }

    Serial.println("");
    Serial.print(Current_Status);
    Serial.println(" View the published data on Watson at: "); 
    Serial.print("https://"); 
    Serial.print(ORG);
    Serial.print(".internetofthings.ibmcloud.com/dashboard/#/devices/browse/drilldown/");
    Serial.print(device_type);
    Serial.print("/");
    Serial.println(device_id);
    
  }
}

void setup() {

  Serial.begin(115200);
  delay(100);
  dht.begin();
   
  //---------------------------------- Setup Wifi -----------------------------------------------------//  
  WiFiManager wifiManager;
  wifiManager.autoConnect(device_id "_" device_type);
  
  client.setCallback(callback);        //configure Callback function
  delay(50);
  
  //---------------------------------- Setup OTA -----------------------------------------------------//
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(device_id "_" device_type);

  // No authentication by default
  ArduinoOTA.setPassword((const char *)"Monads");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  //-----------------------------------------------------------------------------------------------//

   //------------------------------ Serial Update ------------------------------------------------//
  Serial.println(Current_Status);
  Serial.print("MQTT Server=");
  Serial.println(mqtt_server);
  Serial.print("Device ID:");
  Serial.println(device_id);
  //----------------------------------------------------------------------------------------//
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  delay(300000);         //update readings every 5 minutes
  publishreadings();
  ArduinoOTA.handle();
  client.loop();
}

