/*
gsm_gprs_gps_recvsms.ino - Example to read the SMS stored on the SIM card 
Version:     2.0
Date:        09.05.2014
Company:     antrax Datentechnik GmbH
Use with:    Arduino Mega2560 (ATmega2560)


EXAMPLE:
This code will try to read the SMS stored on the SIM card

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
  int result, numberofSMS;    
  
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
  // ok, the ME is registered in the GSM network ... now let's find out, how many SMS are stored on the SIM card
   
  delay(3000);                                                       // the mobile module needs a bit of time to
                                                                     // read out all SMS from the SIM card memory 
  numberofSMS = GSM.numberofSMS();                                   // how many SMS are stored on the SIM card?

  // Serial.print  ("Number of stored SMS: ");                       // print out the result
  // Serial.println(numberofSMS);                                    // numberofSMS is the index of the latest SMS

  //---------------------------------------------------------------------------------------------------------------
  // read out e.g. the latest stored SMS (ATTENTION: it's a potential error!) 
   
  if(numberofSMS > 0)
  {
    // result = GSM.readSMS(0);                                      // read *ALL* SMS on the SIM card
    result = GSM.readSMS(numberofSMS);                               // read the out the latest SMS on the SIM card
                                                                     // (number of SMS = *NOT* always the index of the latest SMS)    
    // Serial.print  ("SMS message: >");                             // here comes the SMS message including
    // Serial.print  (GSM.GSM_string);                               // status, senders mobile number, timestamp
    // Serial.println("<");                                          // and text 
  }

  //---------------------------------------------------------------------------------------------------------------
  // delete ALL or a special SMS on the SIM card 
   
  result = GSM.deleteSMS(0);                                         // e.g. delete ALL SMS on the SIM card
  // result = GSM.deleteSMS(2);                                      // e.g. delete the SMS with index = 2 on the SIM card
    
  if(result == 0)                                                    // => everything ok?
  {
    Serial.print  ("can't delete SMS: >");                           // => no! Error during deleting SMS
    Serial.print  (GSM.GSM_string);                                  //    here is the explanation
    Serial.println("<");                                             //    
    while(1);
  }

  // Serial.println("SMS is delete");                                // => yes!                    
  // Serial.println(GSM.GSM_string);                                 //    here is the answer of the mobile module


  while(1);                                                          // END OF DEMO
}
