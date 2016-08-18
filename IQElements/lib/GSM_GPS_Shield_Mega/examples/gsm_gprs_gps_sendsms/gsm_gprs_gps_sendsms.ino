/*
gsm_gprs_gps_sendsms.ino - Example to send a SMS to a given mobile number 
Version:     2.0
Date:        09.05.2014
Company:     antrax Datentechnik GmbH
Use with:    Arduino Mega2560 (ATmega2560)


EXAMPLE:
This code will try to send a SMS to a given mobile number.

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
  // ok, the ME is registered in the GSM network ... now send the SMS
  // ATTENTION: you MUST set the right destination mobile number!
   
  result = GSM.sendSMS("+491701234567","hello world ...");           // let's send a SMS!
    
  if(result == 0)                                                    // => everything ok?
  {
    Serial.print  ("SMS sending error: >");                          // => no! Error during sending SMS
    Serial.print  (GSM.GSM_string);                                  //    here is the explanation
    Serial.println("<");
    while(1);
  }

  // Serial.println("SMS was send ...");                             // => yes!                    
  // Serial.println(GSM.GSM_string);                                 //    here is the answer of the mobile module
                                                                     //    (the message reference number)

  while(1);                                                          // END OF DEMO
}
