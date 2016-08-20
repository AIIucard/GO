#include <Arduino.h>

#include <dht11.h>
#include <SPI.h>
#include <SD.h>
#include <gsm_gprs_gps_mega.h>
#include <ArduinoJson.h>

#define DHT11PIN 22
#define CS1 52
#define CS2 49

dht11 DHT11;
GSM_GPRS_Class GSM(Serial1);
GPS_Class GPS(Serial1);
int timeoutInMillis = 2000; // overwrite with settings

void setup() {
  Serial.println(F("Entering setup"));
  Serial.begin(9600);

  pinMode (CS1, OUTPUT);
  pinMode (CS2, OUTPUT);

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  digitalWrite(CS2, LOW);
  while (!SD.begin(53)) {
    Serial.println(F("Initialization for SD failed"));
    delay(2000);
  }
  //read configuration from SD card
  StaticJsonBuffer<300> jsonBuffer;
  char fileBuf[300];
  File configFile = SD.open(F("config.txt"), FILE_READ);
  if (configFile) {
    for (int i = 0; i < 300 && configFile.available(); ++i) {
      fileBuf[i] = configFile.read();
    }
  }
  //TODO: check if file is read completly
  JsonObject& config = jsonBuffer.parseObject(fileBuf);

  //timeout time from config
  if (config.containsKey("timeoutInMillis")) {
    timeoutInMillis = config[F("timeoutInMillis")];
  }

  digitalWrite(CS2, HIGH);

  Serial.println(F("Initialization for SD completed"));

  GSM.begin();
  delay(1000);

  digitalWrite(CS1, LOW);

  if (GSM.initialize(config[F("PIN")].asString()) == 0)                                              // => everything ok?
  {
    Serial.print  (F("ME Init error: >"));                                         // => no! Error during GSM initialising
    Serial.print  (GSM.GSM_string);                                             //    here is the explanation
    Serial.println(F("<"));
  }
  digitalWrite(CS1, HIGH);

  while (GSM.connectGPRS(config[F("APN")].asString(),
  config[F("USER")].asString(), config[F("PASSWORD")].asString()) == 0)  // => everything ok?
  {
    Serial.print  (F("GPRS Init error: >"));                                   // => no! Error during GPRS initialising
    Serial.print  (GSM.GSM_string);                                         //    here is the explanation
    Serial.println(F("<"));
  }

  //GSM.setClock();
}

void loop() {
  Serial.println();
  Serial.println(F("Entering loop"));

  StaticJsonBuffer<1000> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["cdb_classname"] = "cdb_sensordaten";

  readGPS(root);
  readTemperatureAndHumidity(root);

  //TODO: activate when POST to Zimt is fixed
  //retrieveOpenWeatherMapData();

  //TODO: compare data with open weather map and create error if necessary

  //just debug info
  Serial.println(F("DEBUG: Following data is gathered"));
  root.printTo(Serial);
  Serial.println(F("---------------------------------"));

  //last but not least send data to zimt server
  sendMessageToZimt(root);

  //open file to store at sd card TODO: only when its not send
  writeToSdFile(root);

  //delay before next sensor data is read
  delay(timeoutInMillis);
}

void sendMessageToZimt(JsonObject& root){
  //send message to server
  char body[400];
  memset(body, 0, 400);
  root.printTo(body, sizeof(body));
  int response = GSM.sendHTTP_POST_JSON("piwik.contact.de", "/api/v1/collection/cdb_sensordaten", 8080, body);
  if (response == 1){
    Serial.println(GSM.GSM_string);
  }
}


void readTemperatureAndHumidity(JsonObject& root){
  digitalWrite(CS1, HIGH);
  if (DHT11.read(DHT11PIN) == DHTLIB_OK) {
    root[F("luftfeuchtigkeit")] = DHT11.humidity;
    root[F("temperatur")] = DHT11.temperature;
  }
}

void readGPS(JsonObject& root){
  digitalWrite(CS1, LOW);
  // GPS initialization
  if(GPS.initializeGPS())
    Serial.println(F("GPS Initialization completed"));
  else
    Serial.println(F("GPS Initialization can't completed"));
  GPS.getGPS();                                           // get the current gps coordinates
  // GPS-LED
  if(GPS.coordinates[0] == 'n') {                                            // valid gps signal yet
    delay(50);                                                                // => no!... wait
    GPS.setLED(1);
    delay(50);
    GPS.setLED(0);
    delay(50);
    GPS.setLED(1);
    delay(50);
    GPS.setLED(0);
  }

  root["laengengrad"] = GPS.longitude;
  root["breitengrad"] = GPS.latitude;
}

void retrieveOpenWeatherMapData(){
  //get current data from open weathermap
  char openWeatherUrl[250];
  char str_lon[12];
  char str_lan[12];
  dtostrf(8.5780000, 9, 7, str_lon);
  dtostrf(53.5420000, 9, 7, str_lan);

  //TODO: API Key from Config
  sprintf(openWeatherUrl, "/data/2.5/weather?lon=%s&lat=%s&units=metric&APPID=226487074b292a9461c9e8bf6d5e78dd", str_lon, str_lan);
  GSM.sendHTTPGET("api.openweathermap.org", openWeatherUrl, 80);
  char* messageBody = strstr(GSM.GSM_string, "\r\n\r\n");

  Serial.println(GSM.GSM_string);
  Serial.println();

  if (messageBody){
    Serial.println("Message body found");
    char jsonMessage[500];
    char* startJson = strpbrk(messageBody, "{");
    char* endJson = strrchr(messageBody, '}');
    if (strstr(endJson - 1, "}}")){
      strlcpy(jsonMessage, startJson, (endJson - startJson) + 1);
    }
    else {
      strlcpy(jsonMessage, startJson, (endJson - startJson) + 2);
    }


    Serial.println(jsonMessage);
    Serial.println();
  }
}

void writeToSdFile(JsonObject& root){
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.

  digitalWrite(CS2, LOW);
  File myFile = SD.open("json.txt", FILE_WRITE);
  if (myFile) {
    Serial.println("Open file to buffer data");
    root.printTo(myFile);
    myFile.println();
    myFile.close();
    Serial.println("Closed file");
  } else {
    Serial.println("Failed to open file");
  }
  digitalWrite(CS2, HIGH);
}
