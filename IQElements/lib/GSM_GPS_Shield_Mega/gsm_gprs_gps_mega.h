/*
gsm_gprs_gps_mega.h - Libary for using the GSM-Modul on the GSM_GPRS_GPS-Shield (Telit GE865-QUAD)
Version:     2.0
Date:        09.05.2014
Company:     antrax Datentechnik GmbH
Use with:    Arduino Mega2560 (ATmega2560)

*/

#ifndef GSM_GPRS_GPS_h
#define GSM_GPRS_GPS_h

#include <Arduino.h>
#include <SPI.h>

//-------------------------------
// PIN definition

//- GSM -//
#define GSM_RXD     		0
#define GSM_TXD     		1
#define GSM_RING   		  2
#define GSM_CTS     		3
#define GSM_DTR    		  4
#define GSM_RTS     		5
#define GSM_DCD     		6

//- General -//
#define SHIELD_POWER_ON 7

#define TESTPIN    		  8

//- GPS -//
// Adresses of the SCI16IS750 registers
#define RHR         0x00 << 3
#define FCR         0x02 << 3
#define LCR         0x03 << 3
#define LSR         0x05 << 3
#define SPR         0x07 << 3
#define IODIR       0x0A << 3
#define IOSTATE     0x0B << 3
#define IOCTL       0x0E << 3

// special registers
#define DLL         0x00 << 3
#define DLM         0x01 << 3
#define EFR         0x02 << 3

// SPI
#define EN_LVL_GPS  49
#define MISO        50
#define MOSI        51
#define SCK         52



#define BUFFER_SIZE 1000

//------------------------------------------------------------------------------

class GSM_GPRS_Class
{
public:
  // Variables
  char GSM_string[BUFFER_SIZE];

  // Constructor
  GSM_GPRS_Class(HardwareSerial& serial);

  // Functions
  int  begin();
  int  initialize(const char simpin[4]);
  int  Status();

  int  setClock();
  int  getTime();

  int  sendErrorToZimt(char key, char *id);
  int  sendZimtPost(char *body);
  int  sendZimtGet();
  int  connectGPRS(const char *APN, const char *USER, const char *PWD);
  int  sendOpenWeatherGet(char *url);
  int  sendHTTPGET(char *server, char *url, int port);
  int  sendHTTP_POST_JSON(char *server, char *url, int port, char *body);
  void disconnectGPRS();

private:
  int  WaitOfReaction(long int timeout);
  void readResponseIntoBuffer(char* buffer, size_t bufferSize, long timeout);
  bool checkIfContains(char* buffer, const char* expected);
  HardwareSerial& _HardSerial;
};

//------------------------------------------------------------------------------

class GPS_Class
{
public:
  // Constructor
  GPS_Class(HardwareSerial& serial);

  // Variables
  char gps_data[80];
  char latitude[20];
  char longitude[20];
  char coordinates[40];

  // Functions
  char initializeGPS();
  void getGPS();
  char checkS1();
  char checkS2();
  void setLED(char state);

private:
  void WriteByte_SPI_CHIP(char adress, char data);
  char ReadByte_SPI_CHIP(char adress);
  void EnableLevelshifter(char lvl_en_pin);
  void DisableLevelshifter(char lvl_en_pin);
  HardwareSerial& _HardSerial;
};

//------------------------------------------------------------------------------

#endif
