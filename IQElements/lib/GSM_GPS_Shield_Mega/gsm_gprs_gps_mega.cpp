/*
gsm_gprs_gps_mega.cpp - Library for using the GSM-Modul on the
GSM/GPRS/GPS-Shield (Telit GE865-QUAD)
Included Functions
Version:     2.0
Date:        09.05.2014
Company:     antrax Datentechnik GmbH
Use with:    Arduino Mega2560 (ATmega2560)

General:
Please don't use Serial.println("...") to send commands to the mobile module!
Use Serial.print( "... \r") instead. See
"Telit_AT_Commands_Reference_Guide_r15.pdf"

Please note: Some commands differ between the setting "SELINT 0/1" and "2
SELINT".
"SELINT 2" is used as setting in this library.

WARNING: Incorrect or inappropriate programming of the mobile module can lead to
increased fees!
*/

#include "pins_arduino.h"
#include <SPI.h>
#include <gsm_gprs_gps_mega.h>

int state = 0;

//####################################################################################################################################################
//----------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------- GSM / GPRS
//-----------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------------
//####################################################################################################################################################

GSM_GPRS_Class::GSM_GPRS_Class(HardwareSerial &serial) : _HardSerial(serial) {}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
Setting of the used signals / Contacts between Arduino mainboard and the
GSM/GPRS/GPS - Shield
*/
void GSM_GPRS_Class::begin() {
  //  pinMode(GSM_RXD, INPUT);
  //  pinMode(GSM_TXD, OUTPUT);
  pinMode(GSM_RING, INPUT);
  pinMode(GSM_CTS, INPUT);
  pinMode(GSM_DTR, OUTPUT);
  pinMode(GSM_RTS, OUTPUT);
  pinMode(GSM_DCD, INPUT);

  pinMode(SHIELD_POWER_ON, OUTPUT);
  pinMode(TESTPIN, OUTPUT);
}
/*----------------------------------------------------------------------------------------------------------------------------------------------------
Initialisation of the GSM/GPRS/GPS - Shield:
- Set data rate
- Activate shield
- Perform init-sequence of the GE865-QUAD
- register the GE865-QUAD in the GSM network

Return value = 0 ---> Error occured
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response from the mobile
module
*/

int GSM_GPRS_Class::initialize(const char simpin[4]) {
  int time;

  _HardSerial.begin(9600); // 9600 Baud
  _HardSerial.flush();

  digitalWrite(SHIELD_POWER_ON,
               HIGH); // GSM_ON = HIGH = switch on + Serial Line enable

  // Start sequence ("Telit_GE865-QUAD_Hardware_User_Guide_r15.pdf", page 16)
  delay(1500);               // wait for 1500ms
  _HardSerial.print("AT\r"); // send the first "AT"
  delay(50);                 // ignore the answer

  _HardSerial.print("AT+IPR=9600\r"); // set Baudrate
  delay(100);                         // ignore the answer

  state = 0;
  time = 0;
  do {
    if (state == 0) {
      _HardSerial.print("ATE0\r"); // disable Echo
      if (WaitOfReaction(1000) == 1) {
        state += 1;
      } else {
        state = 1000;
      }
    }

    if (state == 1) {
      _HardSerial.print("AT#SELINT=2\r"); // Select Interface Style
      if (WaitOfReaction(1000) == 1) {
        state += 1;
      } else {
        state = 1000;
      }
    }

    if (state == 2) {
      _HardSerial.print("AT#SIMDET=1\r"); // SIM detect!
      time = 0;
      if (WaitOfReaction(1000) == 1) {
        state += 1;
      } else {
        state = 1000;
      }
      delay(2000);
    }

    if (state == 3) {
      _HardSerial.print("AT#QSS=0\r"); // Query SIM Status (disables the
                                       // unsolicited indication)
      if (WaitOfReaction(1000) == 1) {
        state += 1;
      } else {
        state = 1000;
      }
    }

    if (state == 4) {
      _HardSerial.print("AT+CMEE=2\r"); // Report Mobile Equipment Error
      if (WaitOfReaction(3000) == 1) {
        state += 1;
      } else {
        state = 1000;
      }
    }

    if (state == 5) {
      delay(1000);
      _HardSerial.print("AT+CPIN?\r"); // SIM pin need?
      switch (WaitOfReaction(1000)) {
      case 2:
        state += 1;
        break; // get +CPIN: SIM PIN
      case 3:
        state += 2;
        break;   // get +CPIN: READY
      case 11: { // get +CME ERROR
        delay(1000);
        if (time++ < 10) {
          state = state; // stay in this state until timeout
        } else {
          state = 1000; // after 10 sek. not registered
        }
        break;
      }
      default:
        _HardSerial.println("Serious SIM-error!");
        while (1)
          ; // ATTENTION: check your SIM!!!! Don't restart the software!!!
      }
    }

    if (state == 6) {
      _HardSerial.print("AT+CPIN="); // enter pin   (SIM)
      _HardSerial.print(simpin);
      _HardSerial.print("\r");
      time = 0;
      if (WaitOfReaction(1000) == 1) {
        state += 1;
      } else {
        state = 1000;
      }
    }

    if (state == 7) {
      _HardSerial.print("AT&K0\r"); // disable Flowcontrol
      if (WaitOfReaction(1000) == 1) {
        state += 1;
      } else {
        state = 1000;
      }
    }

    if (state == 8) {
      _HardSerial.print("AT+CREG?\r"); // Network Registration Report
      if (WaitOfReaction(1000) == 4) {
        state += 1; // get: Registered in home network or roaming
      } else {
        delay(1000);
        if (time++ < 60) {
          state = state; // stay in this state until timeout
        } else {
          state = 1000; // after 60 sec. not registered
        }
      }
    }

    if (state == 9) {
      return 1; // Registered successfully ... let's go ahead!
    }
  } while (state <= 999);

  return 0; // ERROR ... no Registration in the network
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
Determine current status of the mobile module e.g. of the GSM/GPRS-Networks
All current states are returned in the string "GSM_string"

This function can easily be extended to further queries, e.g. AT#CGSN (= query
IMEI),
AT#CCID (= query CCID), AT#CIMI (= query IMSI) etc.

ATTENTION: Please note length of "Status_string" - adjust if necessary

Return value = 0 ---> Error occured
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response from the mobile
module
*/
int GSM_GPRS_Class::Status() {
  char response[100];

  _HardSerial.print("AT+CREG?\r");
  readResponseIntoBuffer(response, sizeof(response), 2000);

  Serial.print(F("AT+CREG: "));
  Serial.println(response);
  if (!strstr(response, "+CREG: 0,1")) {
    return 2;
  }

  _HardSerial.print("AT+CGREG?\r");
  readResponseIntoBuffer(response, sizeof(response), 2000);

  Serial.print(F("AT+CGREG: "));
  Serial.println(response);

  if (!strstr(response, "OK")) {
    return 1;
  }

  return 1;

  // state = 0;
  // do
  // {
  //   if(state == 0)
  //   {
  //     _HardSerial.print("AT+CREG?\r"); // Query register state of GSM network
  //     WaitOfReaction(1000);
  //     strcpy(Status_string, GSM_string);
  //     state += 1;
  //   }
  //
  //   if(state == 1)
  //   {
  //     _HardSerial.print("AT+CGREG?\r");
  //     // Query register state of GPRS network
  //     WaitOfReaction(1000);
  //     strcat(Status_string, GSM_string);
  //     state += 1;
  //   }
  //
  //   if(state == 2)
  //   {
  //     _HardSerial.print("AT+CSQ\r"); // Query the RF signal strength
  //     WaitOfReaction(1000);
  //     strcat(Status_string, GSM_string);
  //     state += 1;
  //   }
  //
  //   if(state == 3)
  //   {
  //     _HardSerial.print("AT+COPS?\r"); // Query the current selected operator
  //     WaitOfReaction(1000);
  //     strcat(Status_string, GSM_string);
  //     state += 1;
  //   }
  //
  //   if(state == 4)
  //   {
  //     strcpy(GSM_string, Status_string);
  //     return 1;
  //   }
  // }
  // while(state <= 999);
  //
  // return 0; // ERROR while dialing
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
Detect if there is an incoming call

Return value = 0 ---> there was no incoming call in the last 5 seconds
Return value = 1 ---> there is a RING currently
The public variable "GSM_string" contains the last response from the mobile
module
*/
int GSM_GPRS_Class::RingStatus() {
  state = 0;
  do {
    if (state == 0) {
      if (WaitOfReaction(5000) == 11) // 11 = "RING" detected
      {
        return 1;
      } // Congratulations ... ME rings
      else {
        return 0;
      } // "no one ever calls!"
    }
  } while (state <= 999);

  return 0; // ERROR
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
"Pick-up the phone" = Accept Voicecall

Return value = 0 ---> Error occured
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response from the mobile
module
*/
int GSM_GPRS_Class::pickUp() {
  state = 0;
  do {
    if (state == 0) {
      _HardSerial.print("ATA\r"); // Answers an incoming call
      if (WaitOfReaction(1000) == 1) {
        return 1;
      } // Congratulations ... you are through
      else {
        return 0;
      } // ERROR
    }
  } while (state <= 999);

  return 0; // ERROR
}

/*
Check Telit_Modules_Software_User_Guide_r16: 3.3 Automatic Data/Time updating
*/
int GSM_GPRS_Class::setClock() {
  state = 0;
  do {
    if (state == 0) {
      _HardSerial.print("AT#NITZ?\r");
      if (WaitOfReaction(1000) == 1) {
        state += 1;
      } else {
        state = 1000;
      }
    }

    if (state == 1) {
      _HardSerial.print("AT#NITZ=15,1\r");
      if (WaitOfReaction(1000) == 1) {
        state += 1;
      } else {
        state = 1000;
      }
    }

    if (state == 2) {
      _HardSerial.print("AT&W0\r");
      if (WaitOfReaction(1000) == 1) {
        state += 1;
      } else {
        state = 1000;
      }
    }

    if (state == 3) {
      _HardSerial.print("AT&P0\r");
      if (WaitOfReaction(1000) == 1) {
        return 1;
      } else {
        state = 1000;
      }
    }
  } while (state <= 999);

  return 0;
}

/*
Check Telit_Modules_Software_User_Guide_r16: 3.8.5.2. Read current date and time
*/
int GSM_GPRS_Class::getTime() {
  state = 0;
  do {
    if (state == 0) {
      _HardSerial.print("AT+CCLK?\r");
      if(WaitOfReaction(1000) == 1)
      {
        return 1;
      } else {
        state = 1000;
      }
    }
  } while (state <= 999);

  return 0;
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------------
Send a SMS
Parameter:
char number[50] = Call number of the destination (national or international
format)
char text[180] = Text of the SMS (ASCII-Text)

Return value = 0 ---> Error occured
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response from the mobile
module (the message reference number)
*/
int GSM_GPRS_Class::sendSMS(char number[50], char text[180]) {
  state = 0;
  do {
    if (state == 0) {
      _HardSerial.print("AT+CREG?\r"); // Network Registration Report
      if (WaitOfReaction(1000) == 4) {
        state += 1;
      } else {
        state = 1000;
      }
    }

    if (state == 1) {
      _HardSerial.print("AT+CMGF=1\r"); // use text-format for SMS
      if (WaitOfReaction(1000) == 1) {
        state += 1;
      } else {
        state = 1000;
      }
    }

    if (state == 2) {
      _HardSerial.print("AT+CMGS=\""); // send Message
      _HardSerial.print(number);
      _HardSerial.print("\"\r");
      if (WaitOfReaction(5000) == 5) {
        state += 1;
      } else {
        state = 1000;
      } // get the prompt ">"
    }

    if (state == 3) {
      _HardSerial.print(text); // Message-Text
      _HardSerial.write(26);   // CTRL-Z
      if (WaitOfReaction(5000) == 1) {
        return 1;
      } // Congratulations ... the SMS is gone
      // GSM_string contains the message reference number
      else {
        return 0;
      } // ERROR while sending a SMS
    }
  } while (state <= 999);

  return 0; // ERROR while sending a SMS
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
Determine number of SMS stored on the SIM card

ATTENTION: The return value contains the "total number" of stored SMS! The index
of the most recent SMS
is possibly higher, if SMS in the "lower" part from the list have already been
deleted!

Return value = 0 ---> Error occured
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response from the mobile
module
*/
int GSM_GPRS_Class::numberofSMS() {
  int numberofSMS;

  state = 0;
  do {
    if (state == 0) {
      _HardSerial.print("AT+CPMS?\r"); // Preferred SMS message storage
      if (WaitOfReaction(1000) == 13) {
        state += 1;
      } else {
        state = 1000;
      }
    }

    if (state == 1) {
      // string to analyse e.g.: +CPMS: "SM",3,20,"SM",3,20,"SM",3,20
      // look for the parameter between the first two ","
      // (how "strtok" works -->
      // http://www.cplusplus.com/reference/cstring/strtok/)
      char *s = strtok(GSM_string, ",");
      s = strtok(NULL, ",");
      numberofSMS = atoi(s);

      return numberofSMS;
    }
  } while (state <= 999);

  return 0; // ERROR while sending a SMS
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
Read SMS stored on the SIM card
Parameter "index" = 0       --->  write *all* SMS in succession in the string
"GSM_string"
Parameter "index" = 1 .. n  --->  write SMS with the indicated index in the
string "GSM_string"

ATTENTION: Please note length of "GSM_string" - adjust if necessary

Return value = 0 ---> Error occured
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response from the mobile
module
*/
int GSM_GPRS_Class::readSMS(int index) {
  state = 0;
  do {
    if (state == 0) {
      _HardSerial.print("AT+CREG?\r"); // Network Registration Report
      if (WaitOfReaction(1000) == 4) {
        state += 1;
      } else {
        state = 1000;
      }
    }

    if (state == 1) {
      _HardSerial.print("AT+CMGF=1\r"); // use text-format for SMS
      if (WaitOfReaction(1000) == 1) {
        state += 1;
      } else {
        state = 1000;
      }
    }

    if (state == 2) {
      if (index == 0) {
        _HardSerial.print("AT+CMGL=\"ALL\"\r"); // read *all* SMS messages
      } else {
        _HardSerial.print("AT+CMGR="); // read SMS message (page 88)
        _HardSerial.print(index);      // index range = 1 ... x
        _HardSerial.print("\r");
      }
      if (WaitOfReaction(1000) == 1)
      // the SMS message is in "GSM_string" now
      {
        return 1;
      } // Congratulations ... the SMS with the given index is read
      else {
        return 0;
      } // ERROR while reading a SMS
    }
  } while (state <= 999);

  return 0; // ERROR while reading a SMS
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
Delete SMS stored on the SIM card
Parameter "index" = 0       --->  delete *all* SMS from the SIM card
Parameter "index" = 1 .. n  --->  delete SMS with the indicated index

ATTENTION: Potential risk!
If single SMS are deleted, the highest index (= index of the latest SMS)
possibly does not correspond anymore
with the number of the in total stored SMS

Return value = 0 ---> Error occured
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response from the mobile
module
*/
int GSM_GPRS_Class::deleteSMS(int index) {
  state = 0;
  do {
    if (state == 0) {
      _HardSerial.print("AT+CREG?\r"); // Network Registration Report
      if (WaitOfReaction(1000) == 4) {
        state += 1;
      } else {
        state = 1000;
      }
    }

    if (state == 1) {
      _HardSerial.print("AT+CMGF=1\r"); // use text-format for SMS
      if (WaitOfReaction(1000) == 1) {
        state += 1;
      } else {
        state = 1000;
      }
    }

    if (state == 2) {
      if (index == 0) {
        _HardSerial.print("AT+CMGD=1,4\r"); // Ignore the value of index and
                                            // delete *all* SMS messages
      } else {
        _HardSerial.print(
            "AT+CMGD="); // delete the SMS with the given index (page 84)
        _HardSerial.print(index); // index range = 1 ... x
        _HardSerial.print("\r");
      }
      if (WaitOfReaction(1000) == 1) {
        return 1;
      } // Congratulations ... the SMS is/are deleted
      else {
        return 0;
      } // ERROR while deleting a SMS
    }
  } while (state <= 999);

  return 0; // ERROR while deleting a SMS
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
//-- G P R S + T C P / I P
//---------------------------------------------------------------------------------------------------------------------------
/*----------------------------------------------------------------------------------------------------------------------------------------------------
Initialize GPRS connection (previously the module needs to be logged into the
GSM network already)
With the successful set-up of the GPRS connection the base for processing
internet (TCP(IP, UDP etc.), e-mail (SMTP)
and PING commands is given.

Parameter:
char APN[50] = APN of the SIM card provider
char USER[30] = Username for this
char PASSW[50] = Password for this

ATTENTION: This SIM card data is provider-dependent and can be obtained from
them.
For example "internet.t-mobile.de","t-mobile","whatever" for T-Mobile, Germany
and "gprs.vodafone.de","whatever","whatever" for Vodafone, Germany
ATTENTION: The SIM card must be suitable or enabled for GPRS data transmission.
Not all SIM cards
(as for example very inexpensive SIM cards) are automatically enabled!!!

Return value = 0 ---> Error occured
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response from the mobile
module
*/
int GSM_GPRS_Class::connectGPRS(const char APN[50], const char USER[30],
                                const char PWD[50]) {
  int time = 0;

  state = 0;
  do {
    if (state == 0) {
      _HardSerial.print("AT+CREG?\r"); // Network Registration Report
      if (WaitOfReaction(1000) == 4) {
        state += 1;
      } else {
        state = 1000;
      } // need 0,1 or 0,5
    }

    if (state == 1) // Judge network?
    {
      _HardSerial.print("AT+CGATT?\r"); // attach to GPRS service?
      if (WaitOfReaction(1000) == 7)    // need +CGATT: 1
      {
        state += 1; // get: attach
      } else {
        delay(2000);
        if (time++ < 60) {
          state = state; // stay in this state until timeout
        } else {
          state = 1000; // after 120 sek. (60 x 2000 ms) not attach
        }
      }
    }

    if (state == 2) {
      _HardSerial.print("AT+CGDCONT=1,\"IP\",\""); // Define PDP Context
      _HardSerial.print(APN);
      _HardSerial.print("\"");
      _HardSerial.print("\r");
      if (WaitOfReaction(1000) == 1) {
        state += 1;
      } else {
        state = 1000;
      } // need OK
    }

    if (state == 3) {
      _HardSerial.print("AT#USERID=\""); // Configure Authentication User ID
      _HardSerial.print(USER);
      _HardSerial.print("\"\r");
      if (WaitOfReaction(1000) == 1) {
        state += 1;
      } else {
        state = 1000;
      } // need OK
    }

    if (state == 4) {
      _HardSerial.print("AT#PASSW=\""); // Configure Authentication Password
      _HardSerial.print(PWD);
      _HardSerial.print("\"\r");
      if (WaitOfReaction(1000) == 1) {
        state += 1;
      } else {
        state = 1000;
      } // need OK
    }

    if (state == 5) {
      _HardSerial.print("AT#SGACT=1,1\r"); // Context Activation
      if (WaitOfReaction(150000) == 1) {
        state += 1;
      } else {
        state = 1000;
      } // need IP and OK
    }

    if (state == 6) {
      return 1; // GPRS connect successfully ... let's go ahead!
    }
  } while (state <= 999);

  return 0; // ERROR ... no GPRS connect
}

/*
Reads the modems response into the given buffer.
Waits up to "timout" milliseconds to retrieve the first byte from the modem
*/
void GSM_GPRS_Class::readResponseIntoBuffer(char *buffer, size_t bufferSize,
                                            long timeout) {
  char byteBuf[1];
  int bytesRead = 0;

  memset(buffer, 0, bufferSize);
  memset(byteBuf, 0, 1);

  _HardSerial.setTimeout(timeout);

  bytesRead = _HardSerial.readBytes(byteBuf, 1);
  if (bytesRead > 0) {
    _HardSerial.setTimeout(30);
    for (unsigned int i = 0; i < bufferSize && bytesRead > 0; ++i) {
      buffer[i] = byteBuf[0];
      memset(byteBuf, 0, 1);
      bytesRead = _HardSerial.readBytes(byteBuf, 1);
    }
  }
}

/*
Send a HTTP POST Request to the given server
*/
int GSM_GPRS_Class::sendHTTP_POST_JSON(char *server, char *url, int port,
                                       char *body) {
  memset(GSM_string, 0, BUFFER_SIZE);
  // wait until all outgoing data is sent
  _HardSerial.flush();

  // clear incoming data buffer
  while (_HardSerial.available() > 0) {
    _HardSerial.read();
  }

  // Opening the socket connection
  // AT#SD = <Conn Id>,<Protocol>, <Remote Port>, <IP address> [, <Closure Type>
  // [, <Local Port>]]
  // Where:
  //  Conn Id is the connection identifier.
  //  Protocol is 0 for TCP and 1 for UDP.
  //  Remote Port is the port of the remote machine.
  //  IP address is the remote address.
  _HardSerial.print("AT#SD=1,0,");
  _HardSerial.print(port);
  _HardSerial.print(",\"");
  _HardSerial.print(server);
  _HardSerial.print("\",0\r");

  // wait until all outgoing data is sent
  _HardSerial.flush();

  delay(2000); // need to wait 1 seconds
  // get response from modem
  char response[100];
  readResponseIntoBuffer(response, sizeof(response), 4000);

  // response should be "CONNECT"
  if (!strstr(response, "CONNECT")) {
    Serial.print(F("ERROR: Socket connection could not be established: "));
    Serial.println(response);
    return -1;
  }
  Serial.print(F("Socket connected: "));
  Serial.println(response);

  // send the HTTP message
  _HardSerial.print("POST ");
  _HardSerial.print(url);
  _HardSerial.print(" HTTP/1.1\r\n");

  _HardSerial.print("Host: ");
  _HardSerial.print(server);
  _HardSerial.print("\r\n");

  _HardSerial.print("User-Agent: ArduinoMega\r\n"); // Header Field "User-Agent"
                                                    // (MUST be "antrax" when
                                                    // use with portal
                                                    // "WebServices")
  _HardSerial.print("Authorization: Basic Y2FkZG9rOg==\r\n");
  _HardSerial.print("Connection: close\r\n"); // Header Field "Connection"
  _HardSerial.print("Content-type: application/json\r\n");
  _HardSerial.print("Content-length: "); // Header Field "Connection"
  _HardSerial.print(strlen(body));

  _HardSerial.print("\r\n\r\n");
  _HardSerial.print(body);
  // wait for transmission
  _HardSerial.flush();

  // read HTTP answer
  readResponseIntoBuffer(GSM_string, BUFFER_SIZE, 2000);

  // clear incoming data buffer
  while (_HardSerial.available() > 0) {
    _HardSerial.read();
  }

  // Exit data mode with escape sequence +++
  // try as hard as possible with while loop >.>
  _HardSerial.print("+++"); // no linefeed here!
  _HardSerial.flush();
  delay(2000); // need to wait 2 seconds
  readResponseIntoBuffer(response, sizeof(response), 1000);

  if (!strstr(response, "OK")) {
    Serial.println(F("ERROR: Failed to exit data mode"));
    return -1;
  }

  Serial.println(F("Exited socket data mode, try to close socket now"));
  _HardSerial.print("AT#SH=1\r");
  _HardSerial.flush();
  delay(2000); // need to wait 2 seconds
  readResponseIntoBuffer(response, sizeof(response), 2000);

  if (!strstr(response, "OK")) {
    Serial.print(F("ERROR: Socket connection was not closed properly: "));
    Serial.println(response);
    return -1;
  }
  Serial.print(F("Socket closed, everything went fine :)"));

  return 1;
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
Send HTTP GET
This corresponds to the "call" of a URL (such as the with the internet browser)
with appended parameters

Parameter:
char server[50] = Address of the server (= URL)
char parameter_string[200] = parameters to be appended

ATTENTION: Before using this function a GPRS connection must be set up.

You may use the "antrax Test Loop Server" for testing. All HTTP GETs to the
server are logged in a list,
that can be viewed on the internet
(http://www.antrax.de/WebServices/responderlist.html)

Example for a correct URL for the transmission of the information "HelloWorld":

http://www.antrax.de/WebServices/responder.php?data=HelloWorld
whereby
- "www.antrax.de" is the server name
- "GET /WebServices/responder.php?data=HelloWorld HTTP/1.1" the parameter

On the server the URL of the PHP script "responder.php" is accepted and analyzed
in the subdirectory "WebServices".
The part after the "?" corresponds to the transmitted parameters. ATTENTION: The
parameters must not
contain spaces. The source code of the PHP script "responder.php" is located in
the documentation.

Return value = 0 ---> Error occured
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response from the mobile
module
*/
int GSM_GPRS_Class::sendHTTPGET(char server[50], char parameter_string[200],
                                int port) {

  state = 0;
  do {
    if (state ==
        0) { // See "Telit_AT_Commands_Reference_Guide_r15.pdf", page 391
      Serial.print("Sending request to: ");
      Serial.println(server);
      Serial.print("URL: ");
      Serial.println(parameter_string);

      _HardSerial.print("AT#SD=1,0,"); // Execution command opens a remote TCP
                                       // connection via socket
      _HardSerial.print(port);
      _HardSerial.print(",\"");
      _HardSerial.print(server);
      _HardSerial.print("\",0\r");
      if (WaitOfReaction(140000UL) == 9) {
        state += 1;
      } else {
        state = 1000;
      } // need CONNECT
    }

    if (state == 1) {
      // for HTTP GET must include: "GET
      // /subdirectory/name.php?test=parameter_to_transmit HTTP/1.1"
      // for example to use with "www.antrax.de/WebServices/responderlist.html":
      // "GET /WebServices/responder.php?test=HelloWorld HTTP/1.1"
      _HardSerial.print("GET ");
      _HardSerial.print(parameter_string); // Message-Text
      _HardSerial.print(" HTTP/1.1");

      // Header Field Definitions in
      // http://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html
      _HardSerial.print("\r\nHost: "); // Header Field "Host"
      _HardSerial.print(server);
      _HardSerial.print("\r\nUser-Agent: antrax"); // Header Field "User-Agent"
                                                   // (MUST be "antrax" when use
                                                   // with portal "WebServices")
      _HardSerial.print(
          "\r\nConnection: close\r\n\r\n"); // Header Field "Connection"
      _HardSerial.write(26);                // CTRL-Z

      if (WaitOfReaction(20000) == 8) {
        state += 1;
      } else {
        state = 1000;
      } // Congratulations ... the parameter_string was send to the server
    }

    if (state == 2) {
      if (WaitOfReaction(20000) == 6) {
        state += 1;
      } else {
        state = 1000;
      } // wait of NO CARRIER
    }

    if (state == 3) {
      return 1; // HTTP GET successfully ... let's go ahead!
    }

  } while (state <= 999);

  return 0; // ERROR ... no GPRS connect
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
Send PING
Ping is a diagnostic function, which is used to check whether a given host can
be reached via GPRS.
The complete description of the PING command (AT#PING) can be viewed here:
"Telit_AT_Commands_Reference_Guide_r15.pdf", page 454

Parameter:
char server[50] = address of the remote host, string type. This parameter can be
either:
- any valid IP address in the format: �xxx.xxx.xxx.xxx�
- any host name to be solved with a DNS query (= URL)

ATTENTION: Before send PING Request the GPRS context must have been activated by
AT#SGACT=1,1.

For testing, a well-to-reach server can be specified, e.g. "www.google.com"

Return value = 0 ---> Error occured
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response from the mobile
module
*/
int GSM_GPRS_Class::sendPING(char server[50]) {
  state = 0;
  do {
    if (state == 0) {
      _HardSerial.print("AT#PING=\""); // send a Ping
      _HardSerial.print(server);
      _HardSerial.print("\",1\r");
      if (WaitOfReaction(20000) == 1) {
        if (strstr(GSM_string, "600,255")) {
          return 0; // Note1: when the Echo Request timeout expires
          // (no reply received on time) the response will contain
          // <replyTime> set to 600 and <ttl> set to 255
        } else {
          return 1; // Congratulations ... receive the corresponding Echo Reply
        }
      } else {
        return 0;
      } // ERROR, the DNS query failed
    }
  } while (state <= 999);

  return 0; // ERROR ...
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
Disconnect GPRS connection

ATTENTION: The frequent disconnection and rebuilding of GPRS connections can
lead to unnecessarily high charges
(e.g. due to "rounding up costs"). It is necessary to consider whether a GPRS
connection for a longer time period
(without data transmission) shall remain active!

No return value
The public variable "GSM_string" contains the last response from the mobile
module
*/
void GSM_GPRS_Class::disconnectGPRS() {
  state = 0;
  do {
    if (state == 0) {
      _HardSerial.print("AT#SGACT=1,0\r"); // Deactivate GPRS context
      if (WaitOfReaction(10000) == 1) {
        state += 1;
      } else {
        state = 1000;
      } // need OK
    }
  } while (state <= 999);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
//-- E-M A I L
//---------------------------------------------------------------------------------------------------------------------------------------
/*----------------------------------------------------------------------------------------------------------------------------------------------------
Setting up the SMTP-Server (outgoing mail server) to be used

Parameter:
char SMTP[50] = Address of the SMTP server
char USER[30] = Username for identification with the SMTP server
char PWD[30] = Password for identification with the SMTP server

All figures above are identically to the figures used for installation of an
e-mail program on a PC
(for example Outlook, Thunderbird etc.).

With this function the listed parameters are transferred to the mobile module,
but do not activate any further action,
such as plausibility or access control, radio transmission, etc.

Return value = 0 ---> Error occured
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response from the mobile
module
*/
int GSM_GPRS_Class::EMAILconfigureSMTP(char SMTP[50], char USER[30],
                                       char PWD[30]) {
  state = 0;
  do {
    if (state == 0) {
      _HardSerial.print("AT#ESMTP=\""); // Configure SMTP server
      _HardSerial.print(SMTP);
      _HardSerial.print("\"\r");
      if (WaitOfReaction(1000) == 1) {
        state += 1;
      } else {
        state = 1000;
      } // need OK
    }

    if (state == 1) {
      _HardSerial.print("AT#EUSER=\""); // Configure USER NAME
      _HardSerial.print(USER);
      _HardSerial.print("\"\r");
      if (WaitOfReaction(1000) == 1) {
        state += 1;
      } else {
        state = 1000;
      } // need OK
    }

    if (state == 2) {
      _HardSerial.print("AT#EPASSW=\""); // Configure PASSWORD
      _HardSerial.print(PWD);
      _HardSerial.print("\"\r");
      if (WaitOfReaction(1000) == 1) {
        state += 1;
      } else {
        state = 1000;
      } // need OK
    }

    if (state == 3) {
      return 1; // Configure SMTP server successfully ... let's go ahead!
    }

  } while (state <= 999);

  return 0; // ERROR ... during configure SMTP server
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
Configure and prepare an outgoing e-mail

Parameter:
char SENDEREMAIL[30] = E-mail address of the sender

With this function the listed parameters are transferred to the mobile module,
but do not activate any further action,
such as plausibility or access control, radio transmission, etc.

Return value = 0 ---> Error occured
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response of the mobile module
*/
int GSM_GPRS_Class::EMAILconfigureSender(char SENDEREMAIL[30]) {
  state = 0;
  do {
    if (state == 0) {
      _HardSerial.print("AT#EADDR=\""); // Configure senders EMAIL ADDRESS
      _HardSerial.print(SENDEREMAIL);
      _HardSerial.print("\"\r");
      if (WaitOfReaction(1000) == 1) {
        state += 1;
      } else {
        state = 1000;
      } // need OK
    }

    if (state == 1) {
      return 1; // Configure sender information successfully ... let's go ahead!
    }

  } while (state <= 999);

  return 0; // ERROR ... during configure sender information
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
Send an e-mail

Parameter:
char RECIPIENT[30] = Destination e-mail address
char TITLE[30] = Subject of the e-mail
char BODY[200] = Text of the e-mail

Return value = 0 ---> Error occured
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response from the mobile
module
*/
int GSM_GPRS_Class::EMAILsend(char RECIPIENT[30], char TITLE[30],
                              char BODY[200]) {
  state = 0;
  do {
    if (state == 0) {
      _HardSerial.print("AT#EMAILD=\""); // Configure the
      _HardSerial.print(RECIPIENT);      // destination mail address
      _HardSerial.print("\",\"");        // and the
      _HardSerial.print(TITLE);          // subject of the mail
      _HardSerial.print("\"\r");
      if (WaitOfReaction(20000) == 5) {
        state += 1;
      } else {
        state = 1000;
      } // need >
    }

    if (state == 1) {
      _HardSerial.print(BODY); // send the text of the email
      _HardSerial.write(26);   // CTRL-Z
      if (WaitOfReaction(150000) == 1) {
        state += 1;
      } else {
        state = 1000;
      } // wait of OK up to 150s
    }

    if (state == 2) {
      return 1; // sending email was successfully!
    }

  } while (state <= 999);

  return 0; // ERROR ... during configure Titel/Text
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
//-- F T P
//-------------------------------------------------------------------------------------------------------------------------------------------
/*----------------------------------------------------------------------------------------------------------------------------------------------------
Set up to use and process/open a connection to the FTP server

Parameter:
char HOST[50] = Address of the FTP server
char PORT = Port to be used (mostly 21)
char USER[30] = Username for the identification with the FTP server
char PASS[30] = Password for the identification with the FTP server

All figures above are identically to the figures used for installation of a FTP
program on a PC
(for example FileZilla, FTP Voyager etc.).

With this function the listed parameters are transferred to the mobile module,
but do not activate any further action,
such as plausibility or access control, radio transmission, etc.

Return value = 0 ---> Error occured
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response from the mobile
module
*/
int GSM_GPRS_Class::FTPopen(char HOST[50], int PORT, char USER[30],
                            char PASS[30]) {
  state = 0;
  do {
    if (state == 0) {
      _HardSerial.print("AT#FTPTO=5000\r"); // Configure FTP timeout
      if (WaitOfReaction(500000) == 1) {
        state += 1;
      } else {
        state = 1000;
      } // need OK
    }

    if (state == 1) {
      _HardSerial.print("AT#FTPOPEN=\""); // Open FTP Connection
      _HardSerial.print(HOST);
      _HardSerial.print(":");
      _HardSerial.print(PORT);
      _HardSerial.print("\",\"");
      _HardSerial.print(USER);
      _HardSerial.print("\",\"");
      _HardSerial.print(PASS);
      _HardSerial.print("\",0\r");
      if (WaitOfReaction(100000) == 1) {
        state += 1;
      } else {
        state = 1000;
      } // need OK
    }

    if (state == 2) {
      _HardSerial.print("AT#FTPTYPE=1\r"); // Set the communication type
      if (WaitOfReaction(500000) == 1) {
        state += 1;
      } else {
        state = 1000;
      } // need OK
    }

    if (state == 3) {
      return 1;
    }
  } while (state <= 999);

  return 0; // ERROR ... during opening FTP server
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
Download a file from the ftp server

Parameter:
char PATH[50] = Path of the file
char FILENAME[50] = Name of the file

The contents of the file to be downloaded via FTP is written into the variable
"GSM_string".

ATTENTION: Please note length "GSM_string" - adjust if necessary

Return value = 0 ---> Error occured
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response from the mobile
module
*/
int GSM_GPRS_Class::FTPdownload(char PATH[50], char FILENAME[50]) {
  state = 0;
  do {
    if (state == 0) {
      _HardSerial.print("AT#FTPGET=\""); // downloading file
      _HardSerial.print(PATH);
      _HardSerial.print(FILENAME);
      _HardSerial.print("\"\r");
      if (WaitOfReaction(500000) == 9) {
        state += 1;
      } else {
        state = 1000;
      } // wait 500 seconds of "CONNECT" as reaction
    }

    if (state == 1) {
      WaitOfDownload(500000); // wait 500 seconds of the content of the file
      state += 1;             // and of "NO CARRIER\r" as reaction
    }

    if (state == 2) {
      return 1; // downloading file from the server successfully ... let's go
                // ahead!
    }

  } while (state <= 999);

  return 0; // ERROR ... during opening FTP server
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
Disconnect from the FTP server and GPRS connection

Parameter:
none

Return value = 0 ---> Error occured
Return value = 1 ---> OK
The public variable "GSM_string" contains the last response from the mobile
module
*/
int GSM_GPRS_Class::FTPclose() {
  state = 0;
  do {
    if (state == 0) {
      _HardSerial.print("AT#FTPCLOSE\r"); // close the FTP service
      if (WaitOfReaction(500000) == 1) {
        state += 1;
      } else {
        state = 1000;
      } // need OK
    }

    if (state == 1) {
      _HardSerial.print("AT#SGACT=1,0\r"); // deactivate GPRS/CSD context
      if (WaitOfReaction(1000) == 1) {
        state += 1;
      } else {
        state = 1000;
      } // need OK
    }

    if (state == 2) {
      return 1; // close FTP server successfully ... let's go ahead!
    }

  } while (state <= 999);

  return 0; // ERROR ... during closing FTP server
}

/*----------------------------------------------------------------------------------------------------------------------------------------------------
Special receive routine of the serial interface for downloads
Used for receiving files from FTP

similar to "int GSM_GPRS_Class::WaitOfReaction(long int timeout)"
*/
int GSM_GPRS_Class::WaitOfDownload(long int timeout) {
  int index = 0;
  int inByte = 0;
  char WS[3];

  //----- erase GSM_string
  memset(GSM_string, 0, BUFFER_SIZE);

  //----- clear _HardSerial Line Buffer
  _HardSerial.flush();
  while (_HardSerial.available()) {
    _HardSerial.read();
  }

  //----- wait of the first character for "timeout" ms
  _HardSerial.setTimeout(timeout);
  inByte = _HardSerial.readBytes(WS, 1);
  GSM_string[index++] = WS[0];

  //----- wait of further characters until a pause of "timeout" ms occures
  while (inByte > 0) {
    inByte = _HardSerial.readBytes(WS, 1);
    GSM_string[index++] = WS[0];
    if ((strstr(GSM_string, "NO CARRIER")) && (WS[0] == '\r')) {
      break;
    }
  }

  delay(100);
  return 0;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------------
/*----------------------------------------------------------------------------------------------------------------------------------------------------
Central receive routine of the serial interface

ATTENTION: This function is used by all other functions (see above) and must not
be deleted

Parameter:
int TIMEOUT = Waiting time to receive the first character (in milliseconds)


Then contents of the received characters is written to the variable
"GSM_string". If no characters is received
for 30 milliseconds, the reception is completed and the contents of "GSM_string"
is being analyzed.

ATTENTION: The length of the reaction times of the mobile module depend on the
condition of the mobile module, for example
quality of wireless connection, provider, etc. and thus can vary. Please keep
this in mind in case this routine is
is changed.

Return value = 0      ---> No known response of the mobile module detected
Return value = 1 - 18 ---> Response detected (see below)
The public variable "GSM_string" contains the last response from the mobile
module
*/
int GSM_GPRS_Class::WaitOfReaction(long int timeout) {
  int index = 0;
  int inByte = 0;
  char WS[3];

  //----- erase GSM_string
  memset(GSM_string, 0, BUFFER_SIZE);
  memset(WS, 0, 3);

  //----- clear _HardSerial Line Buffer
  _HardSerial.flush();
  while (_HardSerial.available()) {
    _HardSerial.read();
  }

  //----- wait of the first character for "timeout" ms
  _HardSerial.setTimeout(timeout);
  inByte = _HardSerial.readBytes(WS, 1);
  GSM_string[index++] = WS[0];

  //----- wait of further characters until a pause of 30 ms occurs
  while (inByte > 0) {
    _HardSerial.setTimeout(30);
    inByte = _HardSerial.readBytes(WS, 1);
    GSM_string[index++] = WS[0];
  }
  GSM_string[index++] = '\0';

  //----- analyze the reaction of the mobile module
  if (strstr(GSM_string, "SIM PIN\r\n")) {
    return 2;
  }
  if (strstr(GSM_string, "READY\r\n")) {
    return 3;
  }
  if (strstr(GSM_string, "0,1\r\n")) {
    return 4;
  }
  if (strstr(GSM_string, "0,5\r\n")) {
    return 4;
  }
  if (strstr(GSM_string, "\n>")) {
    return 5;
  } // prompt for SMS text
  if (strstr(GSM_string, "NO CARRIER\r\n")) {
    return 6;
  }
  if (strstr(GSM_string, "+CGATT: 1\r\n")) {
    return 7;
  }
  if (strstr(GSM_string, "html\r\n\r\nOK")) {
    return 8;
  }
  if (strstr(GSM_string, "CONNECT\r\n")) {
    return 9;
  }
  if (strstr(GSM_string, "RING\r\n")) {
    return 11;
  }
  if (strstr(GSM_string, "+CPMS:")) {
    return 13;
  }

  if (strstr(GSM_string, "OK\r\n")) {
    return 1;
  }

  return 0;
}

//####################################################################################################################################################
//----------------------------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------- GPS
//------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------------
//####################################################################################################################################################

/*----------------------------------------------------------------------------------------------------------------------------------------------------
Setting of the used signals / Contacts between Arduino mainboard and the
GSM/GPRS/GPS - Shield
*/

GPS_Class::GPS_Class(HardwareSerial &serial) : _HardSerial(serial) {}

char GPS_Class::initializeGPS() {
  char test_data = 0;
  char clr_register = 0;

  _HardSerial.begin(9600); // 9600 Baud
  _HardSerial.flush();

  // set pin's for SPI
  pinMode(MOSI, OUTPUT);
  pinMode(MISO, INPUT);
  pinMode(SCK, OUTPUT);
  pinMode(EN_LVL_GPS, OUTPUT);
  digitalWrite(EN_LVL_GPS, LOW);

  SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR1) |
         (1 << SPR0);  // Initialize the SPI-Interface
  clr_register = SPSR; // read register to clear them
  clr_register = SPDR;
  delay(10);

  WriteByte_SPI_CHIP(LCR, 0x80); // set Bit 7 so configure baudrate
  WriteByte_SPI_CHIP(DLL, 0x18); // 0x18 = 9600 with Xtal = 3.6864MHz
  WriteByte_SPI_CHIP(DLM, 0x00); // 0x00 = 9600 with Xtal = 3.6864MHz

  WriteByte_SPI_CHIP(LCR, 0xBF); // configure uart
  WriteByte_SPI_CHIP(EFR, 0x10); // activate enhanced registers
  WriteByte_SPI_CHIP(LCR, 0x03); // Uart: 8,1,0
  WriteByte_SPI_CHIP(FCR, 0x06); // Reset FIFO registers
  WriteByte_SPI_CHIP(FCR, 0x01); // enable FIFO Mode

  // configure GPIO-Ports
  WriteByte_SPI_CHIP(IOCTL, 0x01);   // set as GPIO's
  WriteByte_SPI_CHIP(IODIR, 0x04);   // set the GPIO directions
  WriteByte_SPI_CHIP(IOSTATE, 0x00); // set default GPIO state

  // Check functionality
  WriteByte_SPI_CHIP(SPR, 'A'); // write an a to register and read it
  test_data = ReadByte_SPI_CHIP(SPR);

  if (test_data == 'A')
    return 1;
  else
    return 0;
}

void GPS_Class::getGPS() {
  char gps_data_buffer[20];
  char in_data;
  char no_gps_message[22] = "no valid gps signal";
  int high_Byte;
  int i, j;
  int GPGGA;
  int Position;

  GPGGA = 0;
  i = 0;

  do {
    if (ReadByte_SPI_CHIP(LSR) & 0x01) {
      in_data = ReadByte_SPI_CHIP(RHR);

      // FIFO-System to buffer incomming GPS-Data
      gps_data_buffer[0] = gps_data_buffer[1];
      gps_data_buffer[1] = gps_data_buffer[2];
      gps_data_buffer[2] = gps_data_buffer[3];
      gps_data_buffer[3] = gps_data_buffer[4];
      gps_data_buffer[4] = gps_data_buffer[5];
      gps_data_buffer[5] = gps_data_buffer[6];
      gps_data_buffer[6] = gps_data_buffer[7];
      gps_data_buffer[7] = gps_data_buffer[8];
      gps_data_buffer[8] = gps_data_buffer[9];
      gps_data_buffer[9] = gps_data_buffer[10];
      gps_data_buffer[10] = gps_data_buffer[11];
      gps_data_buffer[11] = gps_data_buffer[12];
      gps_data_buffer[12] = gps_data_buffer[13];
      gps_data_buffer[13] = gps_data_buffer[14];
      gps_data_buffer[14] = gps_data_buffer[15];
      gps_data_buffer[15] = gps_data_buffer[16];
      gps_data_buffer[16] = gps_data_buffer[17];
      gps_data_buffer[17] = gps_data_buffer[18];
      gps_data_buffer[18] = in_data;

      if ((gps_data_buffer[0] == '$') && (gps_data_buffer[1] == 'G') &&
          (gps_data_buffer[2] == 'P') && (gps_data_buffer[3] == 'G') &&
          (gps_data_buffer[4] == 'G') && (gps_data_buffer[5] == 'A')) {
        GPGGA = 1;
      }

      if ((GPGGA == 1) && (i < 80)) {
        if ((gps_data_buffer[0] ==
             0x0D)) // every answer of the GPS-Modul ends with an cr=0x0D
        {
          i = 80;
          GPGGA = 0;
        } else {
          gps_data[i] = gps_data_buffer[0]; // write Buffer into public variable
          i++;
        }
      }
    }
  } while (i < 80);

  // filter gps data

  if (int(gps_data[18]) == 44) {
    j = 0;
    for (i = 0; i < 20; i++) {
      coordinates[j] = no_gps_message[i]; // no gps data available at present!
      j++;
    }
  } else {
    j = 0; // format latitude
    for (i = 18; i < 29; i++) {
      if (gps_data[i] != ',') {
        latitude[j] = gps_data[i];
        j++;
      }

      if (j == 2) {
        latitude[j] = ' ';
        j++;
      }
    }

    j = 0;
    for (i = 31; i < 42; i++) { // format longitude
      if (gps_data[i] != ',') {
        longitude[j] = gps_data[i];
        j++;
      }

      if (j == 2) {
        longitude[j] = ' ';
        j++;
      }
    }

    for (i = 0; i < 40; i++) // clear coordinates
      coordinates[i] = ' ';

    j = 0;
    for (i = 0; i < 11; i++) // write gps data to coordinates
    {
      coordinates[j] = latitude[i];
      j++;
    }

    coordinates[j] = ',';
    j++;

    coordinates[j] = ' ';
    j++;

    for (i = 0; i < 11; i++) {
      coordinates[j] = longitude[i];
      j++;
    }

    coordinates[j++] = '\0';
  }
}

char GPS_Class::checkS1() {
  int value;
  value = (ReadByte_SPI_CHIP(IOSTATE) & 0x01); // read S1 button state
  return value;
}

char GPS_Class::checkS2() {
  int value;
  value = (ReadByte_SPI_CHIP(IOSTATE) & 0x02); // read S2 button state
  return value;
}

void GPS_Class::setLED(char state) {
  if (state == 0)
    WriteByte_SPI_CHIP(IOSTATE, 0x00); // turn off LED
  else
    WriteByte_SPI_CHIP(IOSTATE, 0x07); // turn on LED
}

void GPS_Class::WriteByte_SPI_CHIP(char adress, char data) {
  char databuffer[2];
  int i;

  databuffer[0] = adress;
  databuffer[1] = data;

  EnableLevelshifter(
      EN_LVL_GPS); // enable GPS-Levelshifter and pull CS-line low

  for (i = 0; i < 2; i++)
    SPI.transfer(databuffer[i]); // write data

  DisableLevelshifter(
      EN_LVL_GPS); // disable GPS-Levelshifter and release CS-Line
}

char GPS_Class::ReadByte_SPI_CHIP(char adress) {
  char incomming_data;

  adress = (adress | 0x80);

  EnableLevelshifter(
      EN_LVL_GPS); // enable GPS-Levelshifter and pull CS-line low

  SPI.transfer(adress);
  incomming_data = SPI.transfer(0xFF);

  DisableLevelshifter(
      EN_LVL_GPS); // disable GPS-Levelshifter and release CS-Line

  return incomming_data;
}

void GPS_Class::EnableLevelshifter(char lvl_en_pin) {
  digitalWrite(lvl_en_pin, HIGH);
}

void GPS_Class::DisableLevelshifter(char lvl_en_pin) {
  digitalWrite(lvl_en_pin, LOW);
}
