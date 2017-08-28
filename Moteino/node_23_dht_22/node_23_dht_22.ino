//Node 3 !!
// **********************************************************************************************************
// WeatherShield sketch that works with Moteinos equipped with RFM69W/RFM69HW and WeatherShield
// It sends periodic highly accurate weather readings (temp, hum, atm pressure) from the
//      WeatherShield to the base node/gateway Moteino
// Can be adapted to use Moteinos/Arduinos using RFM12B or other RFM69 variants (RFM69CW, RFM69HCW)
// For use with MoteinoMEGA you will have to revisit the pin definitions defined below
// http://www.LowPowerLab.com/WeatherShield
// Used in this project: http://lowpowerlab.com/blog/2015/07/24/attic-fan-cooling-tests/
// 2015-07-23 (C) Felix Rusu of http://www.LowPowerLab.com/
// **********************************************************************************************************
// License
// **********************************************************************************************************
// This program is free software; you can redistribute it 
// and/or modify it under the terms of the GNU General    
// Public License as published by the Free Software       
// Foundation; either version 3 of the License, or        
// (at your option) any later version.                    
//                                                        
// This program is distributed in the hope that it will   
// be useful, but WITHOUT ANY WARRANTY; without even the  
// implied warranty of MERCHANTABILITY or FITNESS FOR A   
// PARTICULAR PURPOSE. See the GNU General Public        
// License for more details.                              
//                                                        
// You should have received a copy of the GNU General    
// Public License along with this program.
// If not, see <http://www.gnu.org/licenses/>.
//                                                        
// Licence can be viewed at                               
// http://www.gnu.org/licenses/gpl-3.0.txt
//
// Please maintain this license information along with authorship
// and copyright notices in any redistribution of this code
// ***************************************************************************************************************************
#include <RFM69.h>         //get it here: http://github.com/lowpowerlab/rfm69
#include <RFM69_ATC.h>     //get it here: https://github.com/lowpowerlab/rfm69
#include <SPIFlash.h>      //get it here: http://github.com/lowpowerlab/spiflash
//#include <WirelessHEX69.h> //get it here: https://github.com/LowPowerLab/WirelessProgramming
#include <SPI.h>           //comes with Arduino

#include <Wire.h>          //comes with Arduino

#include <LowPower.h> //get library from: https://github.com/lowpowerlab/lowpower
                      //writeup here: http://www.rocketscream.com/blog/2011/07/04/lightweight-low-power-arduino-library/

#include "DHT.h"

#define DHTPIN 4     // what pin we're connected to
#define DHTPOWERPIN 5
#define DHTTYPE DHT22   // DHT 22  (AM2302)

//*****************************************************************************************************************************
// ADJUST THE SETTINGS BELOW DEPENDING ON YOUR HARDWARE/TRANSCEIVER SETTINGS/REQUIREMENTS
//*****************************************************************************************************************************
#define GATEWAYID   1
#define NODEID      23
#define NETWORKID   100
#define FREQUENCY     RF69_433MHZ
//#define FREQUENCY     RF69_868MHZ
//#define FREQUENCY       RF69_915MHZ //Match this with the version of your Moteino! (others: RF69_433MHZ, RF69_868MHZ)
#define ENCRYPTKEY      "encryptionkey123" //has to be same 16 characters/bytes on all nodes, not more not less!
#define IS_RFM69HW    //uncomment only for RFM69HW! Leave out if you have RFM69W!
//#define SEND_LOOPS   15 //send data this many sleep loops (15 loops of 8sec cycles = 120sec ~ 2 minutes)
#define SEND_LOOPS   30
//*********************************************************************************************
#define ENABLE_ATC    //comment out this line to disable AUTO TRANSMISSION CONTROL
#define ATC_RSSI      -75
//*********************************************************************************************
#define SLEEP_FASTEST SLEEP_15MS
#define SLEEP_FAST SLEEP_250MS
#define SLEEP_SEC SLEEP_1S
#define SLEEP_LONG SLEEP_2S
#define SLEEP_LONGER SLEEP_4S
#define SLEEP_LONGEST SLEEP_8S
period_t sleepTime = SLEEP_LONGEST; //period_t is an enum type defined in the LowPower library (LowPower.h)
//*********************************************************************************************
#define LED                  9   //pin connected to onboard LED on regular Moteinos
//#define BLINK_EN                 //uncomment to blink LED on every send
//#define SERIAL_EN                //comment out if you don't want any serial output

#ifdef SERIAL_EN
  #define SERIAL_BAUD   115200
  #define DEBUG(input)   {Serial.print(input);}
  #define DEBUGln(input) {Serial.println(input);}
  #define SERIALFLUSH() {Serial.flush();}
#else
  #define DEBUG(input);
  #define DEBUGln(input);
  #define SERIALFLUSH();
#endif
//*****************************************************************************************************************************

//global program variables

#ifdef ENABLE_ATC
  RFM69_ATC radio;
#else
  RFM69 radio;
#endif

char buffer[50];
//SPIFlash flash(8, 0xEF30); //WINDBOND 4MBIT flash chip on CS pin D8 (default for Moteino)

// Initialize DHT sensor for normal 16mhz Arduino
DHT dht(DHTPIN, DHTTYPE);
  
void setup(void)
{
#ifdef SERIAL_EN
  Serial.begin(SERIAL_BAUD);
#endif
  pinMode(LED, OUTPUT);
  pinMode(DHTPOWERPIN, OUTPUT);
  
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
  Blink(LED, 50);

  radio.encrypt(ENCRYPTKEY);

  digitalWrite(DHTPOWERPIN, HIGH);
  
  //initialize weather shield sensors  
  dht.begin();
  LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);
  digitalWrite(DHTPOWERPIN, LOW);

  initTendancyArray();

  sprintf(buffer, "WeatherMote %d - transmitting at: %d Mhz...", NODEID, FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  DEBUGln(buffer);
  
  #ifdef IS_RFM69HW
    radio.setHighPower(); //uncomment only for RFM69HW!
  #endif
  #ifdef ENABLE_ATC
    radio.enableAutoPower(ATC_RSSI);
  #endif
  
  radio.sendWithRetry(GATEWAYID, "START", 6);
  radio.sleep();
  Blink(LED, 100);Blink(LED, 100);Blink(LED, 100);
  
  SERIALFLUSH();
}

char input=0;
byte sendLoops=0;
byte sendLen;

void loop()
{
  if (sendLoops--<=0)   //send readings every SEND_LOOPS
  {
    //read values
    digitalWrite(DHTPOWERPIN, HIGH);
    LowPower.powerDown(SLEEP_SEC, ADC_OFF, BOD_OFF);
    float _t = dht.readTemperature(false, true);
    float _h = dht.readHumidity();
    digitalWrite(DHTPOWERPIN, LOW);
    if (isnan(_h) || isnan(_t)) {
      sendLoops = 0;
      DEBUGln("Failed to read from DHT sensor!");
      return;
    }
    sendLoops = SEND_LOOPS-1;

    // Tendancy management
    addTendancyValue(_t);
    float coef = calculateCoef() * 10; //Muliply by 10 to get coef by hour

    //Output generation
    snprintf(buffer, 3, "C:");
    dtostrf(_t, 3, 2, buffer + strlen(buffer));
    snprintf(buffer + strlen(buffer), 4, " H:");
    dtostrf(_h, 3, 2, buffer + strlen(buffer));
    snprintf(buffer + strlen(buffer), 5, " TD:");
    if(coef > 0 ){
      snprintf(buffer + strlen(buffer), 2, "+");
    }
    dtostrf(coef, 3, 2, buffer + strlen(buffer));
     

    //Sending message    
    sendLen = strlen(buffer);
    radio.sendWithRetry(GATEWAYID, buffer, sendLen, 1); //retry once
    radio.sleep(); //you can comment out this line if you want this node to listen for wireless programming requests
    DEBUG(buffer); DEBUG(" (packet length:"); DEBUG(sendLen); DEBUGln(")");

    #ifdef BLINK_EN
      Blink(LED, 5);
    #endif
  }
    
  SERIALFLUSH();
  LowPower.powerDown(sleepTime, ADC_OFF, BOD_OFF);
  //LowPower.idle(SLEEP_8S, ADC_OFF, TIMER2_OFF, TIMER1_OFF, 
  //            TIMER0_OFF, SPI_ON, USART0_OFF, TWI_OFF); //SPI_ON is important when receiving is activated
  #ifdef SERIAL_EN
    DEBUG(sendLoops);DEBUG(" - ");DEBUG(battReadLoops);DEBUGln(" - WAKEUP");
  #endif
}
////////////////////////////////////////////////////////////////
#define TENDANCY_MAX_SIZE 5
float storedValues[TENDANCY_MAX_SIZE];
byte storedValuesIdx = 0;

void initTendancyArray(){
  for(byte i = 0; i < TENDANCY_MAX_SIZE; i++){
    storedValues[i] = 200;
  }
}

float calculateCoef(){
  float meanTemp = 0.0f;
  float sumXY = 0.0f;
  byte nb_values = 0;
  for(byte i = 0; i < TENDANCY_MAX_SIZE; i++){
    byte a = (storedValuesIdx + i) % TENDANCY_MAX_SIZE;
    if(storedValues[a] < 200){
      sumXY += storedValues[a]*(i+1);
      meanTemp += storedValues[a];
      ++nb_values;
    }
  }
  meanTemp /= nb_values;
  return ((sumXY - (meanTemp *15)) / 10); // 15 constants as mean X = 3 and nb Elements is 5
}

void addTendancyValue(float val){
  storedValues[storedValuesIdx] = val;
  storedValuesIdx = (storedValuesIdx + 1) % TENDANCY_MAX_SIZE;

}
////////////////////////////////////////////////////////////////
void Blink(byte PIN, byte DELAY_MS)
{
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN,HIGH);
  delay(DELAY_MS/2);
  digitalWrite(PIN,LOW);
  delay(DELAY_MS/2);  
}
