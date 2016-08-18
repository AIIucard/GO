/*
gsm_gprs_gps_recvftp.ino - Example to use FTP 
Version:     2.0
Date:        09.05.2014
Company:     antrax Datentechnik GmbH
Use with:    Arduino Mega2560 (ATmega2560)


EXAMPLE:
This code will try to download a file from the antrax FTP test server

Authentication information for the antrax FTP test server in this example:
FTP-Server: antrax.de
Port:       21
Username:   f007c103
Password:   hywM5Rx3
Path:       /
Filename:   testfile.txt
Content:    Password 1234<cr><lf>
            Parameter ON<cr><lf>
            Time 100<cr><lf>


We only use basic AT sequences (with the library), for detailed information please see the "Telit_AT_Commands_Reference_Guide_rxx.pdf"
(Telit internal document no. 80000ST10025a)  and  "Telit_Modules_Software_User_Guide_rxx.pdf" (Telit internal document no. 1vv0300784)

WARNING: Incorrect or inappropriate programming of the mobile module can lead to increased fees!
*/

#include <gsm_gprs_gps_mega.h>
#include <SPI.h>

void setup()
{
  GSM.begin();
  // Serial.println("init GSM/GPRS/GPS-Shield!");
}

void loop()
{   
  int result;    
  
  //---------------------------------------------------------------------------------------------------------------
  // let's register in the GSM network with the correct SIM pin!
  // ATTENTION: you MUST set your own SIM pin!
  
  result = GSM.initialize("1357");                                   
  if(result == 0)                                                    // => everything ok?
  {
    Serial.print  ("ME Init error: >");                              // => NO! Error during GSM initialising
    Serial.print  (GSM.GSM_string);                                  //    here is the explanation
    Serial.println("<");
    while(1);
  }
  
  //---------------------------------------------------------------------------------------------------------------
  // connect to GPRS with the correct APN (server, access, password) of your provider
  // ATTENTION: you MUST set your own access parameters!

  result = GSM.connectGPRS("public4.m2minternet.com","egal","egal");          // provider "ASPIDER" (with a SIM chip)
  // result = GSM.connectGPRS("internet.t-mobile.de","t-mobile","whatever");  // example for T-Mobile, Germany
  // result = GSM.connectGPRS("gprs.vodafone.de","whatever","whatever");      // example for Vodafone, Germany  
  // result = GSM.connectGPRS("internet.eplus.de","eplus","whatever");        // example for e-plus, Germany 
  // result = GSM.connectGPRS("internet","whatever","gprs");                  // example for O2, Germany 
  if(result == 0)                                                    // => everything ok?
  {
    Serial.print  ("GPRS Init error: >");                            // => no! Error during GPRS initialising
    Serial.print  (GSM.GSM_string);                                  //    here is the explanation
    Serial.println("<");
    while(1);
  }

  //---------------------------------------------------------------------------------------------------------------
  // configure the antrax FTP test server with the correct server name, port, user name, password
  // ATTENTION: later you MUST set your own access parameter!
  
  result = GSM.FTPopen("antrax.de",21,"f007c103","hywM5Rx3");  
  if(result == 0)                                                    // => everything ok?
  {
    Serial.print  ("FTP server Init error: >");                      // => NO! Error during opening FTP server
    Serial.print  (GSM.GSM_string);                                  //    here is the explanation
    Serial.println("<");
    while(1);
  }

  //---------------------------------------------------------------------------------------------------------------
  // download the file "testfile.txt" from the antrax FTP test server
  // ATTENTION: laster you MUST set your own file / path parameter!
  
  result = GSM.FTPdownload("","testfile.txt");  
  if(result == 0)                                                    // => everything ok?
  {
    Serial.print  ("FTP server download error: >");                  // => NO! Error during downloading file
    Serial.print  (GSM.GSM_string);                                  //    here is the explanation
    Serial.println("<");
    while(1);                                                        // Error ---> END OF DEMO
  }
  else
  {
    // Serial.print  ("Content of the requested file: >");           // => YES!
    // Serial.print  (GSM.GSM_string);                               //    here is the file content
    // Serial.println("<");
  }

  //---------------------------------------------------------------------------------------------------------------
  // close FTP service and GPRS context
  
  result = GSM.FTPclose();  
  if(result == 0)                                                    // => everything ok?
  {
    Serial.print  ("FTP server close error: >");                     // => NO! Error during closing FTP server
    Serial.print  (GSM.GSM_string);                                  //    here is the explanation
    Serial.println("<");
  }
 
  while(1);                                                          // END OF DEMO
}
