/*
gsm_gprs_gps_recvvoicecall.ino - Beispiel für einen hereinkommenden Sprachanruf 
Version:     2.0
Date:        09.05.2014
Company:     antrax Datentechnik GmbH
Use with:    Arduino Mega2560 (ATmega2560)


EXAMPLE:
This code will receive an incomming voice call and answers with an DTMF tone

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
  // ok, the ME is registered in the GSM network ... let's wait of an incoming call
   
  int time = 0;                                                      // timeout variable
  
  do
  {
    time++;
    result = GSM.RingStatus();                                       // ringing?
  }
  while((result == 0) && (time < 10));

  if(result == 0)                                                    // => RING detected
  {
    Serial.print  ("no incoming call!");                             // => no one calls me
    Serial.println("");
    while(1);
  }
  else
  {
    // Serial.print  ("incoming call: >");                           // => yippi ... someone calls me
    // Serial.print  (GSM.GSM_string);              
    // Serial.println("<");
  }

  //---------------------------------------------------------------------------------------------------------------
  // pick up the phone ...

  delay(2000);
  result = GSM.pickUp();                                             // pick up the phone

  if(result == 0)                                                    // => everything ok?
  {
    Serial.print  ("pick up error: >");                              // => no! ERROR while pick up the phone
    Serial.print  (GSM.GSM_string);                                  //    here ist the explanation
    Serial.println("<");
    while(1);
  }

  //---------------------------------------------------------------------------------------------------------------
  // ... and say "Hello" (with a DTMF tone)
  
  result = GSM.sendDTMF('1');                                        // let's be noisy!

  if(result == 0)                                                    // => everything ok?
  {
    Serial.print  ("DTMF error: >");                                 // => no! ERROR while sending a DTMF tone
    Serial.print  (GSM.GSM_string);                                  //    here ist the explanation
    Serial.println("<");
    while(1);
  }

  //---------------------------------------------------------------------------------------------------------------
  // exit the call
   
  delay(1000);
  result = GSM.exitCall();                                           // bye bye!

  if(result == 0)                                                    // => everything ok?
  {
    Serial.print  ("Hang up error: >");                              // => no! ERROR while hanging up
    Serial.print  (GSM.GSM_string);                                  //    here ist the explanation
    Serial.println("<");
    while(1);
  }
  
  
  while (1);
}

