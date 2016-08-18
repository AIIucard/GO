/*
gsm_gprs_gps_ping.ino - Example to send a PING to an internet server 
Version:     2.0
Date:        09.05.2014
Company:     antrax Datentechnik GmbH
Use with:    Arduino Mega2560 (ATmega2560)


EXAMPLE:
This code will try to to send a PING to www.google.com

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
    Serial.print  ("ME Init error: >");                              // => no! Error during GSM initialising
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
  }
 
  //---------------------------------------------------------------------------------------------------------------
  // send a PING to "www.google.com" and be pleased of the result
  
  result = GSM.sendPING("www.google.com");  
  if(result == 0)                                                    // => everything ok
  {
    Serial.print  ("PING error: >");                                 // => no! we have a "PING Error"
    Serial.print  (GSM.GSM_string);                                  //    here is the explanation
    Serial.println("<");                                             //    = response of AT#PING (page 455)
    while(1);
  }
  else
  {
    Serial.println("PING successfully ...");                         // => yes!                    
    Serial.print  (GSM.GSM_string);                                  //    here is the answer of the web server
  }
  while(1);                                                          // END OF DEMO
}
