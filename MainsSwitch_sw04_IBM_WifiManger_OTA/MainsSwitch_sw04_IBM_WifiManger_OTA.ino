#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <stdlib.h>

// ------------------ Configurable Parameters ------------------------//
#define ORG "0qlmiq"
#define device_type "MainsSwitch"
#define device_id "sw04"

#define RELAY 12         //MTDI Relay (HIGH to turn on)
#define LED 13          //MTCK LED (LOW to turn on)
#define BUTTON 0

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
char password[] = "zVEp9SZ&HnQZ34XQMs";                     //TOKEN from IBM Bluemix for sw03
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

  Serial.begin(115200);
  delay(100);

  //---------------- PIN SETUP-------------------------------//
  pinMode(RELAY,OUTPUT); //
  pinMode(LED,OUTPUT); //
  
  digitalWrite(RELAY,HIGH); //Turn On on Startup
  RlyStatus = "ON";
  digitalWrite(LED,LOW); //Turn On      
  
  //------------------ CONNECT TO WIFI ----------------------------------//  
    WiFiManager wifiManager;
    wifiManager.autoConnect(device_id "_" device_type);

   //---------------------------------- Setup OTA -----------------------------------------------------//

  Serial.println("Settin up OTA");
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("MainsSwitch_sw04");

  // No authentication by default
  ArduinoOTA.setPassword((const char *)"Strip02Monads");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
    digitalWrite(LED_BUILTIN, LOW);
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    //digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
    //delay(500);                      // Wait for a 0.5 second
    //digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH
    //delay(500); 
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
  Serial.println("OTA_Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  //-------------------------------------- CALLBACK FUNCTION FOR MQTT --------------------------------//
    
  client.setCallback(callback);        //configure Callback function
  delay(50);
  //----------------------------------------------------------------------------------------------------------------//
  
  //---------------------------------------- SERIAL DIAGNOSTICS----------------------------------------//

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
  
  yield();
  ArduinoOTA.handle();
  client.loop();
}

