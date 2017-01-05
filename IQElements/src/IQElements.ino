#include <Arduino.h>
#include <ArduinoJson.h>
#include <SD.h>
#include <SPI.h>
#include <dht11.h>
#include <gsm_gprs_gps_mega.h>
#include <Time.h>
#include <Keypad.h>

#define DHTPIN 22
#define CS1 52 // SD Card
#define CS2 49 // Shield
#define BLED 40

char key = NO_KEY;
const byte rows = 4;
const byte cols = 4;
char keys[rows][cols] = {
  {'4','3','2','1'},
  {'8','7','6','5'},
  {'C','B','A','9'},
  {'G','F','E','D'}
};
byte rowPins[rows] = {33, 32, 31, 30}; //connect to the row pinouts of the keypad
byte colPins[cols] = {37, 36, 35, 34}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, rows, cols );

struct ConfigData {
  const char *pin;
  const char *apn;
  const char *boardid;
  const char *user;
  const char *password;
  int timeoutInMillis = 2000;
};

ConfigData config;
StaticJsonBuffer<300> jsonConfigBuffer;

dht11 DHT;
GSM_GPRS_Class GSM(Serial1);
GPS_Class GPS(Serial1);
float str_lon;
float str_lan;
char Json_Time_String[20];
char session_Key[33];

void setup() {
  Serial.println(F("Entering setup"));
  Serial.begin(115200);

  pinMode(CS1, OUTPUT);
  pinMode(CS2, OUTPUT);
  pinMode(BLED, OUTPUT);

  digitalWrite(CS1, LOW);
  digitalWrite(CS2, LOW);
  digitalWrite(BLED, LOW);

  while (!Serial); // wait for serial port to connect. Needed for native USB port only

  digitalWrite(CS1, HIGH);
  while (!SD.begin(53)) {
    Serial.println(F("Initialization for SD failed"));
    delay(1000);
  }
  Serial.println(F("Initialization for SD completed"));
  if(!deserialize(config)){
    while(1){delay(10000);
      digitalWrite(13, LOW);
      Serial.println(F("Error Jo"));
    }
  }
  delay(10000);
  Serial.println(F("Initialization config completed"));
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

  int i = 0;
  Serial.println(F("Key Open"));
  digitalWrite(BLED, HIGH);
  while(i < 10){
    delay(1000);
    key = keypad.getKey();
    if (key != NO_KEY){
      sendError(key);
      sendCloseToZimt();
      Serial.print("---------------------");
      Serial.println(key);
    }
    ++i;
  }
  digitalWrite(BLED, LOW);

  Serial.println(F("Key Close"));

  StaticJsonBuffer<1000> jsonBuffer;
  JsonObject &root = jsonBuffer.createObject();
  root["cdb_classname"] = "cdb_sensordaten";
  root["fs_board"] = config.boardid;

  readTemperatureAndHumidity(root);
  readTimestamp(root);

  // use shield
  digitalWrite(CS2, HIGH);
  readGPS(root);

  // just debug info
  Serial.println(F("DEBUG: Following data is gathered"));
  root.printTo(Serial);
  Serial.println(F("\r\n---------------------------------"));

  // last but not least send data to zimt server
  retrieveOpenWeatherMapData(root);
  sendMessageToZimt(root);
  sendCloseToZimt();
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
  data.boardid = root[F("BOARDID")].asString();
  data.user = root[F("USER")].asString();
  data.password = root[F("PASSWORD")].asString();
  data.pin = root[F("PIN")].asString();

  return root.success();
}

int connectToGPRS() {
  if (GSM.connectGPRS(config.apn, config.user, config.password) != 1)
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


void sendMessageToZimt(JsonObject &root) {
  int status = GSM.Status();

  // Not registered in network, need reconnect
  if (status == 2) {
    connectToGPRS();
    status = GSM.Status();
  }

  if (status == 1) {
    // send message to server
    char body[400];
    memset(body, 0, 400);
    root.printTo(body, sizeof(body));

    Serial.println(body);

    // TODO api from config
    int response = GSM.sendZimtPost(body);
    if (response == 1) {
      Serial.println(GSM.GSM_string);
    }
    else{
      Serial.println("ERROR: Response was not 1");
    }
  } else {
    Serial.println(F("ERROR: Couldn't fix network connection"));
  }
}

void sendError(char *key) {
  int status = GSM.Status();

  // Not registered in network, need reconnect
  if (status == 2) {
    connectToGPRS();
    status = GSM.Status();
  }

  if (status == 1) {
    int response = GSM.sendErrorToZimt(key, config.boardid);
    if (response == 1) {
      Serial.println(GSM.GSM_string);
    }
    else{
      Serial.println("ERROR: Response was not 1");
    }
  } else {
    Serial.println(F("ERROR: Couldn't fix network connection"));
  }
}

void sendCloseToZimt() {
  int status = GSM.Status();

  if (status == 2) {
    connectToGPRS();
    status = GSM.Status();
  }

  if (status == 1) {
    int response = GSM.sendZimtGet();
    if (response == 1) {
      Serial.println(GSM.GSM_string);
    }
    else{
      Serial.println("ERROR: Response was not 1");
    }
  } else {
    Serial.println(F("ERROR: Couldn't fix network connection"));
  }
}

void readTimestamp(JsonObject& root){
  sprintf(Json_Time_String, "%04d-%02d-%02dT%02d:%02d:%02d", year(), month(), day(), hour(), minute(), second());
  root[F("datum")] = Json_Time_String;
}

void readTemperatureAndHumidity(JsonObject& root){
  if (DHT.read(DHTPIN) == DHTLIB_OK) {
    root[F("humidity")] = DHT.humidity;
    root[F("temperature")] = DHT.temperature;
  }
}

void readGPS(JsonObject &root) {
  if (GPS.initializeGPS())
    Serial.println(F("GPS Initialization completed"));
  else
    Serial.println(F("GPS Initialization can't completed"));

  GPS.getGPS();
  str_lon = getDecimalCoordinate(GPS.longitude);
  str_lan = getDecimalCoordinate(GPS.latitude);

  while(str_lon == 0.00 && str_lan == 0.00){
    GPS.getGPS();

    str_lon = getDecimalCoordinate(GPS.longitude);
    str_lan = getDecimalCoordinate(GPS.latitude);

    if (GPS.coordinates[0] == 'n') {
        delay(500);
        GPS.setLED(1);
        delay(500);
        GPS.setLED(0);
    }
  }

  if(str_lon != 0.00 && str_lan != 0.00){
    root["longitude"] = str_lon, 4;
    root["latitude"] = str_lan, 4;
  }
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

void retrieveOpenWeatherMapData(JsonObject &root) {
  // get current data from open weathermap
  StaticJsonBuffer<800> jsonWBuffer;
  int status = GSM.Status();

  // Not registered in network, need reconnect
  if (status == 2) {
    connectToGPRS();
    status = GSM.Status();
  }

  if (status == 1) {
    // send message to server
    char openWeatherUrl[200];
    memset(openWeatherUrl, 0, 200);
    int firstLon = str_lon;
    int endZifLon = (str_lon * 10000) - firstLon;
    endZifLon = endZifLon % 10000;

    int firstLan = str_lan;
    int endZifLan = (str_lan * 10000) - firstLan;
    endZifLan = endZifLan % 10000;

    sprintf(openWeatherUrl, "/data/2.5/weather?lon=%i.%i&lat=%i.%i&units=metric&APPID=8652d302ad6d86ce7c6cbd2c676674f9", firstLon, endZifLon, firstLan, endZifLan);

    Serial.println(openWeatherUrl);

    int response = GSM.sendOpenWeatherGet(openWeatherUrl);
    if (response == 1) {
      char *messageBody = strstr(GSM.GSM_string, "\r\n\r\n");

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
        JsonObject &weather = jsonWBuffer.parseObject(jsonMessage);
        root[F("owm_temperature")] = weather[F("main")][F("temp")];
        root[F("owm_longitude")] = weather[F("coord")][F("lon")];
        root[F("owm_latitude")] = weather[F("coord")][F("lat")];
        root[F("owm_humidity")] = weather[F("main")][F("humidity")];
        Serial.println();
      }
      else{
        Serial.println("Error: Message not Found");
      }
    }
    else{
      Serial.println("ERROR: Response was not 1");
    }
  } else {
    Serial.println(F("ERROR: Couldn't fix network connection"));
  }
}

void writeToSdFile(JsonObject &root) {
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
