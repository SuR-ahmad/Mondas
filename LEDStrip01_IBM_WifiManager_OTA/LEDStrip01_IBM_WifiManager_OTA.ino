#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <stdlib.h>
#include <DNSServer.h>

#include <ESP8266WebServer.h>
#include <WiFiManager.h>


#define Run01 4         //Strip Run 01
#define Run02 5         //Strip Run 02

#define device_type "LED_Stip"
#define device_id "Strip01"
#define TOKEN "t4TY2TAlp(*8OSV&QP"
#define ORG "0qlmiq"

char* Device_Status = "Offline";
char Run01_Status[10] = "0";
char Run02_Status[10] = "0";

char mqtt_server[] = ORG ".messaging.internetofthings.ibmcloud.com";
const int port= 8883;
char event[] = "iot-2/evt/Status/fmt/JSON";
char command[] = "iot-2/cmd/Command/fmt/json";
char username[] = "use-token-auth";
char password[] = TOKEN;
char clientId[] = "d:" ORG ":" device_type ":" device_id;
const char willTopic[]="iot-2/evt/Will/fmt/string";
int willQoS = 1;
int willRetain = 1;
const char* willMessage="Disconnected";


WiFiClientSecure espClient;
PubSubClient client(mqtt_server,port,espClient);


void publishstatus(){
  
  String PayL = "{\"DeviceStatus\":\"";
  PayL += Device_Status;
  PayL += "\",\"Run01_Status\":\"";
  PayL += Run01_Status;
  PayL += ",\"Run02_Status\":\"";
  PayL += Run02_Status;
  PayL += "}";
  client.publish(event,(char*)PayL.c_str());
}

void callback(char* topic, byte* message, unsigned int length) {

    String Stopic=topic;
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& payload = jsonBuffer.parseObject((char*)message);
    String COMMAND = payload["Command"];
    int RUN01_PWM = payload["Run01_PWM"];
    int RUN02_PWM = payload["Run02_PWM"];

    if (COMMAND=="Set")
    {
      analogWrite(Run01,RUN01_PWM);   //HIGH TO TURN ON
      analogWrite(Run02,RUN02_PWM);   //HIGH TO TURN ON
      itoa (RUN01_PWM,Run01_Status,10);
      itoa (RUN02_PWM,Run02_Status,10);
      Device_Status = "Online";
      publishstatus();
    }

    else if (COMMAND=="Ping")
    {
      Device_Status = "Online";
      publishstatus();
    }
    
    //---------------------------------------- SERIAL DIAGNOSTICS----------------------------------------//
    Serial.println("Message Arrived");
    Serial.print("Topic: ");
    Serial.println(Stopic);
    Serial.println();
    Serial.print("Incoming Command=");
    Serial.println(command);
    Serial.print("RUN01_PWM=");
    Serial.println(RUN01_PWM);
    Serial.print("RUN02_PWM=");
    Serial.println(RUN02_PWM);
}

void reconnect() {

  // Loop until we're reconnected
  while (!client.connected()) {
    // Attempt to connect
    Serial.print("Attempting to Connect to the MQTT Server..");
    if (client.connect(clientId,username,password,willTopic,willQoS,willRetain,willMessage)) 
    {
      //Once connected, publish an announcement...
      Device_Status = "Online";
      publishstatus();
      // ... and resubscribe
      client.subscribe(command);
    } 
    else 
    {
      Serial.print(".");
      delay(50);
    }
    
    //---------------------------------------- SERIAL DIAGNOSTICS----------------------------------------//
    Serial.println("");
    Serial.println(Device_Status);
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
  pinMode(Run01,OUTPUT); //
  pinMode(Run02,OUTPUT); //
  
  analogWriteRange(100);
  analogWrite(Run01,100); //Turn On on Startup
  analogWrite(Run02,100); //Turn On on Startup
  itoa (100,Run01_Status,10);
  itoa (100,Run02_Status,10);
  
  //---------------------------------- Setup Wifi -----------------------------------------------------//  
  WiFiManager wifiManager;
  wifiManager.autoConnect(device_id "_" device_type);

  //---------------------------------- Setup OTA -----------------------------------------------------//

  Serial.println("Settin up OTA");
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("LED_Strip01");

  // No authentication by default
  ArduinoOTA.setPassword((const char *)"Strip01Monads");

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
  Serial.println("OTA_Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  //-------------------------------------- CALLBACK FUNCTION FOR MQTT --------------------------------//
    
  client.setCallback(callback);        //configure Callback function
  delay(50);

  //---------------------------------------- SERIAL DIAGNOSTICS----------------------------------------//
  Serial.println(Device_Status);
  Serial.print("MQTT Server=");
  Serial.println(mqtt_server);
  Serial.print("Device ID:");
  Serial.println(device_id);

  client.setCallback(callback);        //configure Callback function
  delay(50);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  yield();
  ArduinoOTA.handle();
  client.loop();
}

