#include <Arduino.h>
#include <ArduinoJson.h>
#include <SD.h>
#include <SPI.h>
#include <dht.h>
#include <gsm_gprs_gps_mega.h>
#include <Time.h>

#define DHT_PIN 22
#define CS1 52 // SD Card
#define CS2 49 // Shield

struct ConfigData {
  const char *pin;
  const char *apn;
  const char *user;
  const char *password;
  const char *dht;
  int timeoutInMillis = 2000;
};

ConfigData config;
StaticJsonBuffer<300> jsonConfigBuffer;

dht DHT;
GSM_GPRS_Class GSM(Serial1);
GPS_Class GPS(Serial1);
char str_lon[12];
char str_lan[12];
char Json_Time_String[20];

void setup() {
  Serial.println(F("Entering setup"));
  Serial.begin(115200);

  pinMode(CS1, OUTPUT);
  pinMode(CS2, OUTPUT);

  digitalWrite(CS1, LOW);
  digitalWrite(CS2, LOW);

  while (!Serial)
    ; // wait for serial port to connect. Needed for native USB port only

  digitalWrite(CS1, HIGH);
  while (!SD.begin(53)) {
    Serial.println(F("Initialization for SD failed"));
    delay(1000);
  }
  Serial.println(F("Initialization for SD completed"));
  deserialize(config);
  digitalWrite(CS1, LOW);

  Serial.print(F("Use loop interval of: "));
  Serial.println(config.timeoutInMillis);

  digitalWrite(CS2, HIGH);
  if (GSM.begin() != 1){
    Serial.println(F("Modem is shit :("));
  }

  delay(1000);
  if (!GSM.initialize(config.pin) == 1) // => everything ok?
  {
    Serial.print(F("ME Init error: >")); // => no! Error during GSM initialising
    Serial.print(GSM.GSM_string);        //    here is the explanation
    Serial.println(F("<"));
    delay(1000);
  }

  while (connectToGPRS() == 0);

  setClock();

  Serial.println("CONNTECTED TO GPRS YAY");

  digitalWrite(CS2, LOW);
}

void loop() {
  Serial.println();
  Serial.println(F("Entering loop"));

  StaticJsonBuffer<1000> jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();
  root["cdb_classname"] = "cdb_sensordaten";

  readTemperatureAndHumidity(root);
  readTimestamp(root);

  // use shield
  digitalWrite(CS2, HIGH);
  readGPS(root);

  // TODO: activate when POST to Zimt is fixed
  // retrieveOpenWeatherMapData();

  // TODO: compare data with open weather map and create error if necessary

  // just debug info
  Serial.println(F("DEBUG: Following data is gathered"));
  root.printTo(Serial);
  Serial.println(F("\r\n---------------------------------"));

  // last but not least send data to zimt server
  sendMessageToZimt(root);
  digitalWrite(CS2, LOW);

  // open file to store at sd card TODO: only when its not send
  // use SD card
  digitalWrite(CS1, HIGH);
  writeToSdFile(root);
  digitalWrite(CS1, LOW);

  // delay before next sensor data is read
  delay(config.timeoutInMillis);
}

//read configuration from SD Card
bool deserialize(ConfigData &data) {
  char fileBuf[300];
  File configFile = SD.open(F("config.txt"), FILE_READ);
  if (configFile) {
    for (int i = 0; i < 300 && configFile.available(); ++i) {
      fileBuf[i] = configFile.read();
    }
  }

  // TODO: check somehow if file is read completly
  JsonObject &root = jsonConfigBuffer.parseObject(fileBuf);
  data.apn = root[F("APN")].asString();
  data.user = root[F("USER")].asString();
  data.password = root[F("PASSWORD")].asString();
  data.pin = root[F("PIN")].asString();
  data.dht = root[F("DHT")];

  return root.success();
}

int connectToGPRS() {
  if (GSM.connectGPRS(config.apn, config.user, config.password, config.dht) != 1)
  {
    Serial.print(
        F("GPRS Init error: >")); // => no! Error during GPRS initialising
    Serial.print(GSM.GSM_string); //    here is the explanation
    Serial.println(F("<"));
    delay(1000);
    return 0;
  }

  return 1;
}

// TODO: we need to check if the connection is still OK and try to reconnect if
// not
void sendMessageToZimt(JsonObject &root) {
  int status = GSM.Status(config.dht);

  // Not registered in network, need reconnect
  if (status == 2) {
    connectToGPRS();
    status = GSM.Status(config.dht);
  }

  if (status == 1) {
    // send message to server
    char body[400];
    memset(body, 0, 400);
    root.printTo(body, sizeof(body));
    // TODO api from config
    int response = GSM.sendHTTP_POST_JSON(
        "piwik.contact.de", "/api/v1/collection/cdb_sensordaten", 8080, body);
    if (response == 1) {
      Serial.println(GSM.GSM_string);
    }
  } else {
    Serial.println(F("ERROR: Couldn't fix network connection"));
  }
}

void setClock(){
  GSM.getTime();

  char second[3];
  char minute[3];
  char hour[3];
  char day[3];
  char month[3];
  char year[5];

  year[0] = '2';
  year[1] = '0';
  year[2] = GSM.GSM_string[10];
  year[3] = GSM.GSM_string[11];
  year[4] = '\0';

  month[0] = GSM.GSM_string[13];
  month[1] = GSM.GSM_string[14];
  month[2] = '\0';

  day[0] = GSM.GSM_string[16];
  day[1] = GSM.GSM_string[17];
  day[2] = '\0';

  hour[0] = GSM.GSM_string[19];
  hour[1] = GSM.GSM_string[20];
  hour[2] = '\0';

  minute[0] = GSM.GSM_string[22];
  minute[1] = GSM.GSM_string[23];
  minute[2] = '\0';

  second[0] = GSM.GSM_string[25];
  second[1] = GSM.GSM_string[26];
  second[2] = '\0';

  setTime(atoi(hour), atoi(minute), atoi(second), atoi(day), atoi(month), atoi(year));
}

void readTimestamp(JsonObject& root){
  sprintf(Json_Time_String, "%04d-%02d-%02dT%02d:%02d:%02d", year(), month(), day(), hour(), minute(), second());
  root[F("datum")] = Json_Time_String;
}

void readTemperatureAndHumidity(JsonObject& root){
  digitalWrite(CS1, HIGH);
  int returnCode = 1; //value thats not a default DHTLIB value

  if (strstr(config.dht, "DHT11")){
    Serial.println(F("Read DHT 11 sensor"));
    returnCode = DHT.read11(DHT_PIN);
  }
  else if (strstr(config.dht, "DHT22")){
    Serial.println(F("Read DHT 22 sensor"));
    returnCode = DHT.read22(DHT_PIN);
  } else {
    Serial.println("No DHT Setting in config");
    Serial.println(config.dht);
  }

  switch (returnCode)
  {
    case DHTLIB_OK:
    root[F("luftfeuchtigkeit")] = DHT.humidity;
    root[F("temperatur")] = DHT.temperature;
		break;
    case DHTLIB_ERROR_CHECKSUM:
		Serial.print("Checksum error,\t");
		break;
    case DHTLIB_ERROR_TIMEOUT:
		Serial.print("Time out error,\t");
		break;
    case DHTLIB_ERROR_CONNECT:
        Serial.print("Connect error,\t");
        break;
    case DHTLIB_ERROR_ACK_L:
        Serial.print("Ack Low error,\t");
        break;
    case DHTLIB_ERROR_ACK_H:
        Serial.print("Ack High error,\t");
        break;
    default:
		Serial.print("Unknown error,\t");
		break;
  }
}

void readGPS(JsonObject &root) {
  // GPS initialization
  if (GPS.initializeGPS())
    Serial.println(F("GPS Initialization completed"));
  else
    Serial.println(F("GPS Initialization can't completed"));
  GPS.getGPS(); // get the current gps coordinates
  // GPS-LED
  if (GPS.coordinates[0] == 'n') { // valid gps signal yet
    delay(50);                     // => no!... wait
    GPS.setLED(1);
    delay(50);
    GPS.setLED(0);
    delay(50);
    GPS.setLED(1);
    delay(50);
    GPS.setLED(0);
  }

  // convert GPS coordinates to decimal
  // TODO: trim the char arrays somehow...
  // not that easy because if we manipulate the start of the pointer we might
  // run into problems
  // so instead of manipulating str_lon and str_lan pointers, create another
  // pointer with offset?
  memset(str_lon, 0, 12);
  memset(str_lan, 0, 12);
  dtostrf(getDecimalCoordinate(GPS.longitude), 7, 4, str_lon);
  dtostrf(getDecimalCoordinate(GPS.latitude), 7, 4, str_lan);

  root["laengengrad"] = str_lon;
  root["breitengrad"] = str_lan;
}

// awkward way to convert coordinate to decimals.. wunderful char pointers..
float getDecimalCoordinate(char *coord) {
  // use a copy because strtok manipulates the memory
  char coordCopy[20];
  strcpy(coordCopy, coord);

  char delimiter1[] = " ";
  char *ptr;
  float degrees = 0;

  ptr = strtok(coordCopy, delimiter1);
  if (ptr != NULL) {
    // first token
    degrees = atof(ptr);
    char delimiter2[] = "ENWS";
    ptr = strtok(NULL, delimiter2);
    if (ptr != NULL) {
      // second token
      degrees += (atof(ptr) / 60.0);
    }
  }

  return degrees;
}

void retrieveOpenWeatherMapData() {
  // get current data from open weathermap
  char openWeatherUrl[250];

  // TODO: API Key from Config
  sprintf(openWeatherUrl, "/data/2.5/"
                          "weather?lon=%s&lat=%s&units=metric&APPID="
                          "226487074b292a9461c9e8bf6d5e78dd",
          str_lon, str_lan);
  GSM.sendHTTPGET("api.openweathermap.org", openWeatherUrl, 80);
  char *messageBody = strstr(GSM.GSM_string, "\r\n\r\n");

  Serial.println(GSM.GSM_string);
  Serial.println();

  if (messageBody) {
    Serial.println("Message body found");
    char jsonMessage[500];
    char *startJson = strpbrk(messageBody, "{");
    char *endJson = strrchr(messageBody, '}');
    if (strstr(endJson - 1, "}}")) {
      strlcpy(jsonMessage, startJson, (endJson - startJson) + 1);
    } else {
      strlcpy(jsonMessage, startJson, (endJson - startJson) + 2);
    }

    Serial.println(jsonMessage);
    Serial.println();
  }
}

void writeToSdFile(JsonObject &root) {
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
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
}
