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
char str_lon[12];
char str_lan[12];

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
    delay(1000);
  }
  Serial.println(F("Initialization for SD completed"));

  //read configuration from SD card
  StaticJsonBuffer<300> jsonBuffer;
  char fileBuf[300];
  File configFile = SD.open(F("config.txt"), FILE_READ);
  if (configFile) {
    for (int i = 0; i < 300 && configFile.available(); ++i) {
      fileBuf[i] = configFile.read();
    }
  }
  //TODO: check somehow if file is read completly
  JsonObject& config = jsonBuffer.parseObject(fileBuf);

  //timeout time from config
  if (config.containsKey("timeoutInMillis")) {
    timeoutInMillis = config[F("timeoutInMillis")];
  }
  Serial.print(F("Use loop interval of: "));
  Serial.println(timeoutInMillis);

  //code to init shield
  digitalWrite(CS2, HIGH);

  GSM.begin();
  delay(1000);

  digitalWrite(CS1, LOW);
  if (config.containsKey("PIN")) {
    if (GSM.initialize(config[F("PIN")].asString()) == 0)                                              // => everything ok?
    {
      Serial.print  (F("ME Init error: >"));                                         // => no! Error during GSM initialising
      Serial.print  (GSM.GSM_string);                                             //    here is the explanation
      Serial.println(F("<"));
      delay(1000);
    }
  } else {
    Serial.println(F("ERROR: No PIN available, check config file"));
  }
  digitalWrite(CS1, HIGH);

  while (GSM.connectGPRS(config[F("APN")].asString(),
    config[F("USER")].asString(), config[F("PASSWORD")].asString()) == 0)  // => everything ok?
  {
    Serial.print  (F("GPRS Init error: >"));                                   // => no! Error during GPRS initialising
    Serial.print  (GSM.GSM_string);                                         //    here is the explanation
    Serial.println(F("<"));
    delay(1000);
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
  readTimestamp(root);

  //TODO: activate when POST to Zimt is fixed
  //retrieveOpenWeatherMapData();

  //TODO: compare data with open weather map and create error if necessary

  //just debug info
  Serial.println(F("DEBUG: Following data is gathered"));
  root.printTo(Serial);
  Serial.println(F("\r\n---------------------------------"));

  //last but not least send data to zimt server
  sendMessageToZimt(root);

  //open file to store at sd card TODO: only when its not send
  writeToSdFile(root);

  //delay before next sensor data is read
  delay(timeoutInMillis);
}

//TODO: we need to check if the connection is still OK and try to reconnect if not
void sendMessageToZimt(JsonObject& root){
  //send message to server
  char body[400];
  memset(body, 0, 400);
  root.printTo(body, sizeof(body));
  //TODO api from config
  int response = GSM.sendHTTP_POST_JSON("piwik.contact.de", "/api/v1/collection/cdb_sensordaten", 8080, body);
  if (response == 1){
    Serial.println(GSM.GSM_string);
  }
}

void readTimestamp(JsonObject& root){
  GSM.getTime();
  root[F("datum")] = GSM.Json_Time_String;
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

  //convert GPS coordinates to decimal
  //TODO: trim the char arrays somehow...
  //not that easy because if we manipulate the start of the pointer we might run into problems
  //so instead of manipulating str_lon and str_lan pointers, create another pointer with offset?
  memset(str_lon, 0, 12);
  memset(str_lan, 0, 12);
  dtostrf(getDecimalCoordinate(GPS.longitude), 7, 4, str_lon);
  dtostrf(getDecimalCoordinate(GPS.latitude), 7, 4, str_lan);

  root["laengengrad"] = str_lon;
  root["breitengrad"] = str_lan;
}

//awkward way to convert coordinate to decimals.. wunderful char pointers..
float getDecimalCoordinate(char* coord){
  //use a copy because strtok manipulates the memory
  char coordCopy[20];
  strcpy(coordCopy, coord);

  char delimiter1[] = " ";
  char *ptr;
  float degrees = 0;

  ptr = strtok(coordCopy, delimiter1);
  if (ptr != NULL){
    //first token
    degrees = atof(ptr);
    char delimiter2[] = "ENWS";
    ptr = strtok(NULL, delimiter2);
    if (ptr != NULL){
      //second token
      degrees += (atof(ptr) / 60.0);
    }
  }

  return degrees;
}

void retrieveOpenWeatherMapData(){
  //get current data from open weathermap
  char openWeatherUrl[250];

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
