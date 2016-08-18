/*
gsm_gprs_gps_httpget.ino - Example to send a HTTP GET to an internet server 
Version:     2.0
Date:        09.05.2014
Company:     antrax Datentechnik GmbH
Use with:    Arduino Mega2560 (ATmega2560)


EXAMPLE:
This code will try to send a HTTP GET which contents gps coordinates to the antrax test loop server.

We only use basic AT sequences (with the library), for detailed information please see the "Telit_AT_Commands_Reference_Guide_rxx.pdf"
(Telit internal document no. 80000ST10025a)  and  "Telit_Modules_Software_User_Guide_rxx.pdf" (Telit internal document no. 1vv0300784)

WARNING: Incorrect or inappropriate programming of the mobile module can lead to increased fees!
*/

#include <gsm_gprs_gps_mega.h>
#include <SPI.h>

void setup()
{
  // GSM
  GSM.begin();
  // Serial.println("init GSM/GPRS/GPS-Shield!");
  
  // GPS initialization
  if(GPS.initializeGPS())
    Serial.println("Initialization completed");
  else
    Serial.println("Initialization can't completed");
  
  delay(1000);  
  
}

void loop()
{   
  int result;
  int i,j;
  char httpget_str[100] = ""; 
  char filtered_gps_coordinates[50] = "";
  
  //---------------------------------------------------------------------------------------------------------------
  // let's register in the GSM network with the correct SIM pin!
  // ATTENTION: you MUST set your own SIM pin!
  
  result = GSM.initialize("1357");                                   
  if(result == 0)                                                               // => everything ok?
  {
    Serial.print  ("ME Init error: >");                                         // => no! Error during GSM initialising
    Serial.print  (GSM.GSM_string);                                             //    here is the explanation
    Serial.println("<");
    while(1);
  }
  

  //---------------------------------------------------------------------------------------------------------------
  // let's wait for a valid gps signal. You can see when the led flashs slowly (1 time in a second) that there is 
  // a valid gps signal. 
  // When you push the button "s1" you will send the current coordinates to the "antrax Test Loop Server"

  while(1)
  {
    GPS.getGPS();                                                               // get the current gps coordinates
    //Serial.print(GPS.coordinates);                                            // show the current gps coordinates
    
    // GPS-LED
    if(GPS.coordinates[0] == 'n')                                               // valid gps signal yet?
    {
      delay(20);                                                                // => no!... wait
      GPS.setLED(1);
      delay(20);  
      GPS.setLED(0); 
      delay(20);
      GPS.setLED(1);
      delay(20);  
      GPS.setLED(0);
    }
    else
    {
      delay(300);                                                               // => yes!... push the button
      GPS.setLED(1);
      delay(500);  
      GPS.setLED(0); 
    }
    
    if(!GPS.checkS1())                                                          // did you push the button?          
    {      
      //---------------------------------------------------------------------------------------------------------------
      // connect to GPRS with the correct APN (server, access, password) of your provider
      // ATTENTION: you MUST set your own access parameters!
      
      result = GSM.connectGPRS("public4.m2minternet.com","egal","egal");        // provider "ASPIDER" (with a SIM chip)
      // result = GSM.connectGPRS("internet.t-mobile.de","t-mobile","whatever");// example for T-Mobile, Germany
      // result = GSM.connectGPRS("gprs.vodafone.de","whatever","whatever");    // example for Vodafone, Germany  
      // result = GSM.connectGPRS("internet.eplus.de","eplus","whatever");      // example for e-plus, Germany 
      // result = GSM.connectGPRS("internet","whatever","gprs");                // example for O2, Germany 
      if(result == 0)                                                           // => everything ok?
      {
        Serial.print  ("GPRS Init error: >");                                   // => no! Error during GPRS initialising
        Serial.print  (GSM.GSM_string);                                         //    here is the explanation
        Serial.println("<");
        while(1);
      }
    
      //---------------------------------------------------------------------------------------------------------------
      // send the following HTTP GET: http://www.antrax.de/WebServices/responder.php?data=52+06.4720N,08+39.7816E 
      // this is a HTTP GET to the "antrax Test Loop Server" and you can look for the result 
      // under http://www.antrax.de/WebServices/responderlist.html
      // The parameter are the GPS coordinates
      
      // replace all spaces with '+' character in the GPS coordinates (this is necessary for spaces in an url)
      memset((void *)filtered_gps_coordinates, 0, sizeof(filtered_gps_coordinates));
      j = 0;
      for(i = 0; i < strlen(GPS.coordinates); i++)
      {
        if(GPS.coordinates[i] == ' ')
        {
          filtered_gps_coordinates[j++] = '+';
        }
        else
        {
          filtered_gps_coordinates[j++] = GPS.coordinates[i];
        }
      }
    
      // build http-get string
      sprintf(httpget_str, "%s%s%s", "GET /WebServices/responder.php?data=", filtered_gps_coordinates, " HTTP/1.1");

      // send the http-get string
      result = GSM.sendHTTPGET("www.antrax.de", httpget_str);  
      if(result == 0)                                                           // => everything ok
      {
        Serial.print  ("HTTP GET error: >");                                    // => no! Error during data sending
        Serial.print  (GSM.GSM_string);                                         //    here is the explanation
        Serial.println("<");
        while(1);
      }
    
      Serial.println("HTTP GET successfully ...");                              // => yes!                    
      Serial.println(GSM.GSM_string);                                           //    here is the answer of the web server
    
    
      while(1);         // END OF DEMO
    }
  }
}