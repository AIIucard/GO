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
  Serial.begin(9600);

  pinMode (CS1, OUTPUT);
  pinMode (CS2, OUTPUT);

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  digitalWrite(CS2, LOW);
  while (!SD.begin(53)) {
    Serial.println("Initialization for SD failed");
    delay(2000);
  }
  //read configuration from SD card
  StaticJsonBuffer<300> jsonBuffer;
  char fileBuf[300];
  File configFile = SD.open("config.txt", FILE_READ);
  if (configFile) {
    for (int i = 0; i < 300 && configFile.available(); ++i) {
      fileBuf[i] = configFile.read();
    }
  }
  //TODO: check if file is read completly
  JsonObject& config = jsonBuffer.parseObject(fileBuf);

  //timeout time from config
  if (config.containsKey("timeoutInMillis")) {
    timeoutInMillis = config["timeoutInMillis"];
  }

  digitalWrite(CS2, HIGH);

  Serial.println("Initialization for SD completed");

  GSM.begin();
  delay(1000);

  digitalWrite(CS1, LOW);

  // blau.de => 9053
  if (GSM.initialize(config["PIN"].asString()) == 0)                                              // => everything ok?
  {
    Serial.print  ("ME Init error: >");                                         // => no! Error during GSM initialising
    Serial.print  (GSM.GSM_string);                                             //    here is the explanation
    Serial.println("<");
  }
  digitalWrite(CS1, HIGH);

  while (GSM.connectGPRS(config["APN"].asString(),
  config["USER"].asString(), config["PASSWORD"].asString()) == 0)  // => everything ok?
  {
    Serial.print  ("GPRS Init error: >");                                   // => no! Error during GPRS initialising
    Serial.print  (GSM.GSM_string);                                         //    here is the explanation
    Serial.println("<");
  }

  GSM.setClock();
}

void loop() {

  digitalWrite(CS1, LOW);
  Serial.println();
  Serial.println("And again");
  StaticJsonBuffer<1000> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  // GPS initialization
  if(GPS.initializeGPS())
  Serial.println("Initialization completed");
  else
  Serial.println("Initialization can't completed");
  GPS.getGPS();                                           // get the current gps coordinates
  // GPS-LED
  if(GPS.coordinates[0] == 'n') {                                            // valid gps signal yet
    delay(20);                                                                // => no!... wait
    GPS.setLED(1);
    delay(20);
    GPS.setLED(0);
    delay(20);
    GPS.setLED(1);
    delay(20);
    GPS.setLED(0);
  }
  else {
    delay(300);                                                               // => yes!... push the button
    GPS.setLED(1);
    delay(500);
    GPS.setLED(0);
  }

  root["longitude"] = GPS.longitude;
  root["latitude"] = GPS.latitude;

  //get current data from open weathermap
  char openWeatherUrl[250];
  char str_lon[12];
  char str_lan[12];
  dtostrf(8.5780000, 9, 7, str_lon);
  dtostrf(53.5420000, 9, 7, str_lan);

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
    Serial.println(endJson);
    if (strstr(endJson - 1, "}}")){
      strlcpy(jsonMessage, startJson, (endJson - startJson) + 1);
    }
    else {

      strlcpy(jsonMessage, startJson, (endJson - startJson) + 2);
    }


    Serial.println(jsonMessage);
    Serial.println();
  }

  digitalWrite(CS1, HIGH);
  if (DHT11.read(DHT11PIN) == DHTLIB_OK) {
    root["humidity"] = DHT11.humidity;
    root["temperature"] = DHT11.temperature;
  }

  //send message to server
  // char body[200] = "";
  // sprintf(body, "{\"temperature\" : %d, \"humidity\" : %d}", DHT11.temperature, DHT11.humidity);
  // GSM.sendHTTP_POST_JSON("88.71.33.250", "/rest/environmentData", 5000, body);
  // Serial.println(GSM.GSM_string);

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.

  //just debug info
  Serial.println("Following data is gathered");
  root.printTo(Serial);
  Serial.println();

  //open file to store at sd card TODO: only when its not send
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

  //delay before next sensor data is read
  delay(timeoutInMillis);
}
