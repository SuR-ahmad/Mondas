#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <stdlib.h>

// ------------------ Configurable Parameters ------------------------//
#define ORG "0qlmiq"
#define device_type "MainsSwitch"
#define device_id "sw03"

#define RELAY 12         //MTDI Relay (HIGH to turn on)
#define LED 13          //MTCK LED (LOW to turn on)

const char* Current_Status = "Offline";
const char* BtnStatus = "OFF";
const char* RlyStatus = "OFF";

//-------------MQTT SETUP (should come after parameters have been read to setup the device connection to the MQTT server)-------------------//

char  mqtt_server[] = ORG ".messaging.internetofthings.ibmcloud.com";
const int port = 8883;
char event[] = "iot-2/evt/Status/fmt/JSON";                 //Topic for publishing status
char reading[] ="iot-2/evt/Readings/fmt/JSON";              //Topic for publishing readings
char command[] = "iot-2/cmd/Command/fmt/json";              //Topic for incoming messages
char username[] = "use-token-auth";                         //TOKEN based authorization
char password[] = "P&&u@hte_2riGh1b0c";                     //TOKEN from IBM Bluemix for sw03
char clientId[] = "d:" ORG ":" device_type ":" device_id;
const char willTopic[]="iot-2/evt/Will/fmt/string";
int willQoS = 1;
int willRetain = 1;
const char* willMessage="Disconnected";

WiFiClientSecure espClient;
PubSubClient client(mqtt_server,port,espClient);

//---------------------------------------------------------------------------------------------//


void publishstatus()
{
  
  String PayL = "{\"DeviceStatus\":\"";
  PayL += Current_Status;
  PayL += "\",\"ButtonState\":\"";
  PayL += BtnStatus;
  PayL += "\",\"RelayState\":\"";
  PayL += RlyStatus;
  PayL += "\"}";
  client.publish(event,(char*)PayL.c_str());
}

void callback(char* topic, byte* message, unsigned int length) {

    String Stopic=topic;
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& payload = jsonBuffer.parseObject((char*)message);
    String command = payload["Command"];

    if (command=="on")
    {
      digitalWrite(RELAY,HIGH);   //HIGH TO TURN ON
      digitalWrite(LED,LOW);      //LOW TO TURN ON
      RlyStatus = "ON";
      Current_Status = "Online";
      publishstatus();
    }

    else if (command=="off")
    {
      digitalWrite(RELAY,LOW);   //LOW TO TURN OFF
      digitalWrite(LED,HIGH);      //HIGH TO TURN OFF
      RlyStatus = "OFF";
      Current_Status = "Online";
      publishstatus();
    }

    else if (command=="ping")
    {
      Current_Status = "Online";
      publishstatus();
    }
    
    //---------------------------------------- SERIAL DIAGNOSTICS----------------------------------------//
    Serial.println("Message Arrived");
    Serial.print("Topic: ");
    Serial.println(Stopic);
    Serial.println();
    Serial.print("Incoming Command=");
    Serial.println(command);
}

void reconnect() {

  // Loop until we're reconnected
  while (!client.connected()) 
  
  {
    // Attempt to connect
    Serial.print("Attempting to Connect to the MQTT Server..");
    if (client.connect(clientId,username,password,willTopic,willQoS,willRetain,willMessage)) {
      //Once connected, publish an announcement...
      Current_Status = "Online";
      publishstatus();
      // ... and resubscribe
      client.subscribe(command);
    } else {
      Serial.print(".");
      delay(5000);
    }

    //------------------------ Blink -------------------------------------------------------------------//
    digitalWrite(LED,HIGH); //Turn Off
    delay(1000);
    digitalWrite(LED,LOW); //Turn On
    delay(1000);
    digitalWrite(LED,HIGH); //Turn Off
    delay(1000);
    digitalWrite(LED,LOW); //Turn On
    delay(1000);
    digitalWrite(LED,HIGH); //Turn Off
    delay(1000);
    digitalWrite(LED,LOW); //Turn On

    //---------------------------------------- SERIAL DIAGNOSTICS----------------------------------------//
    Serial.println("");
    Serial.println(Current_Status);
    Serial.print("Subscribed to: ");
    Serial.println(command);
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

  //---------------- PIN SETUP-------------------------------//
  pinMode(RELAY,OUTPUT); //
  pinMode(LED,OUTPUT); //
  
  digitalWrite(RELAY,HIGH); //Turn On on Startup
  RlyStatus = "ON";
  digitalWrite(LED,LOW); //Turn On      
  
  //------------------ CONNECT TO WIFI ----------------------------------//  
    WiFiManager wifiManager;
    wifiManager.autoConnect(device_id);

  client.setCallback(callback);                       //configure Callback function
  delay(500);
  reconnect();                                       //Connect to the the MQTT server

  //---------------------------------------- SETUP CONFIGURATION PARAMETERS ----------------------------------------//

  

  //----------------------------------------------------------------------------------------------------------------//
  
  //---------------------------------------- SERIAL DIAGNOSTICS----------------------------------------//
  Serial.begin(115200);
  delay(100);
  Serial.println(Current_Status);
  Serial.print("MQTT Server=");
  Serial.println(mqtt_server);
  Serial.print("Device ID:");
  Serial.println(device_id);
  //----------------------------------------------------------------------------------------------------------------//
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  
  client.loop();
}

