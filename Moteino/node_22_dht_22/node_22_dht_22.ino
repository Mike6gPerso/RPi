//Node 2 !!
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
#include <SPIFlash.h>      //get it here: http://github.com/lowpowerlab/spiflash
#include <WirelessHEX69.h> //get it here: https://github.com/LowPowerLab/WirelessProgramming
#include <SPI.h>           //comes with Arduino

#include <Wire.h>          //comes with Arduino

#include <LowPower.h> //get library from: https://github.com/lowpowerlab/lowpower
                      //writeup here: http://www.rocketscream.com/blog/2011/07/04/lightweight-low-power-arduino-library/

#include "DHT.h"

#define DHTPIN 4     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)

//*****************************************************************************************************************************
// ADJUST THE SETTINGS BELOW DEPENDING ON YOUR HARDWARE/TRANSCEIVER SETTINGS/REQUIREMENTS
//*****************************************************************************************************************************
#define GATEWAYID   1
#define NODEID      22
#define NETWORKID   100
#define FREQUENCY     RF69_433MHZ
//#define FREQUENCY     RF69_868MHZ
//#define FREQUENCY       RF69_915MHZ //Match this with the version of your Moteino! (others: RF69_433MHZ, RF69_868MHZ)
#define ENCRYPTKEY      "myencryptionkey7" //has to be same 16 characters/bytes on all nodes, not more not less!
#define IS_RFM69HW    //uncomment only for RFM69HW! Leave out if you have RFM69W!
#define SEND_LOOPS   15 //send data this many sleep loops (15 loops of 8sec cycles = 120sec ~ 2 minutes)
//*********************************************************************************************
#define SLEEP_FASTEST SLEEP_15MS
#define SLEEP_FAST SLEEP_250MS
#define SLEEP_SEC SLEEP_1S
#define SLEEP_LONG SLEEP_2S
#define SLEEP_LONGER SLEEP_4S
#define SLEEP_LONGEST SLEEP_8S
period_t sleepTime = SLEEP_LONGEST; //period_t is an enum type defined in the LowPower library (LowPower.h)
//*********************************************************************************************
#define BATT_MONITOR_EN A3 //enables battery voltage divider to get a reading from a battery, disable it to save power
#define BATT_MONITOR  A7   //through 1Meg+470Kohm and 0.1uF cap from battery VCC - this ratio divides the voltage to bring it below 3.3V where it is scaled to a readable range
#define BATT_CYCLES   2    //read and report battery voltage every this many sleep cycles (ex 30cycles * 8sec sleep = 240sec/4min). For 450 cyclesyou would get ~1 hour intervals
//#define BATT_FORMULA(reading) reading * 0.00322 * 1.475  // >>> fine tune this parameter to match your voltage when fully charged
#define BATT_FORMULA(reading) reading / 88 
#define BATT_LOW      1.6  //(volts)
#define BATT_READ_LOOPS  SEND_LOOPS*10  // read and report battery voltage every this many sleep cycles (ex 30cycles * 8sec sleep = 240sec/4min). For 450 cycles you would get ~1 hour intervals between readings
//*****************************************************************************************************************************
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
RFM69 radio;
char Pstr[10];
char buffer[50];
SPIFlash flash(8, 0xEF30); //WINDBOND 4MBIT flash chip on CS pin D8 (default for Moteino)

// Initialize DHT sensor for normal 16mhz Arduino
DHT dht(DHTPIN, DHTTYPE);
  
void setup(void)
{
#ifdef SERIAL_EN
  Serial.begin(SERIAL_BAUD);
#endif
  pinMode(LED, OUTPUT);
  
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
#ifdef IS_RFM69HW
  radio.setHighPower(); //uncomment only for RFM69HW!
#endif
  radio.encrypt(ENCRYPTKEY);

  sprintf(buffer, "WeatherMote - transmitting at: %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  DEBUGln(buffer);

  //initialize weather shield sensors  
  dht.begin();
  
  radio.sendWithRetry(GATEWAYID, "START", 6);
  Blink(LED, 100);Blink(LED, 100);Blink(LED, 100);
  
  SERIALFLUSH();
  readBattery();
}

unsigned long doorPulseCount = 0;
char input=0;
double P;
byte sendLoops=0;
byte battReadLoops=0;
float batteryVolts = 5;
char* BATstr="BAT:5.00v"; //longest battery voltage reading message = 9chars
byte sendLen;
char bufferT[10];
char bufferH[10];

void loop()
{
  if (radio.receiveDone())
  {
    //byte newStatus=STATUS;
    DEBUG('[');DEBUG(radio.SENDERID);DEBUG("] ");
    for (byte i = 0; i < radio.DATALEN; i++)
      DEBUG((char)radio.DATA[i]);
    if(radio.DATALEN == 1){
      if(radio.DATA[0] == 'B'){
        Blink(LED, 200);
      }
    }
    if (radio.ACKRequested())
    {
      radio.sendACK();
      DEBUG(" - ACK sent.");
    }
  }
  
  if (battReadLoops--<=0) //only read battery every BATT_READ_LOOPS cycles
  {
    readBattery();
    battReadLoops = BATT_READ_LOOPS-1;
  }
  if (sendLoops--<=0)   //send readings every SEND_LOOPS
  {
    float _t = dht.readTemperature(false, true);
    float _h = dht.readHumidity();
    if (isnan(_h) || isnan(_t)) {
      DEBUGln("Failed to read from DHT sensor!");
      return;
    }
    sendLoops = SEND_LOOPS-1;

    dtostrf(_h,5,2,bufferH);
    dtostrf(_t,5,2,bufferT); // sprintf does not support floating point: http://forum.arduino.cc/index.php?topic=125946.0
    sprintf(buffer, "BAT:%sv C:%s H:%s", BATstr, bufferT, bufferH);
    
    //sprintf(buffer, "C:%s H:%s", bufferT, bufferH);
    sendLen = strlen(buffer);
    radio.sendWithRetry(GATEWAYID, buffer, sendLen, 2); //retry two time
    DEBUG(buffer); DEBUG(" (packet length:"); DEBUG(sendLen); DEBUGln(")");

    #ifdef BLINK_EN
      Blink(LED, 5);
    #endif
  }
    
  SERIALFLUSH();
  //radio.sleep(); //you can comment out this line if you want this node to listen for wireless programming requests
  //LowPower.powerDown(sleepTime, ADC_OFF, BOD_OFF);
  LowPower.idle(SLEEP_8S, ADC_OFF, TIMER2_OFF, TIMER1_OFF, 
              TIMER0_OFF, SPI_ON, USART0_OFF, TWI_OFF); //SPI_ON is important when receiving is activated
  //delay(8000);
  DEBUGln("WAKEUP");
}

void readBattery()
{
  unsigned int readings=0;
  
  //enable battery monitor on WeatherShield (via mosfet controlled by A3)
  pinMode(BATT_MONITOR_EN, OUTPUT);
  digitalWrite(BATT_MONITOR_EN, LOW);

  for (byte i=0; i<5; i++) //take several samples, and average
    readings+=analogRead(BATT_MONITOR);
  
  //disable battery monitor
  pinMode(BATT_MONITOR_EN, INPUT); //highZ mode will allow p-mosfet to be pulled high and disconnect the voltage divider on the weather shield
  //  DEBUGln(readings);
  batteryVolts = BATT_FORMULA(readings / 5.0);
  dtostrf(batteryVolts,3,2, BATstr); //update the BATStr which gets sent every BATT_CYCLES or along with the MOTION message
  if (batteryVolts <= BATT_LOW) BATstr = "LOW";
}

void Blink(byte PIN, byte DELAY_MS)
{
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN,HIGH);
  delay(DELAY_MS/2);
  digitalWrite(PIN,LOW);
  delay(DELAY_MS/2);  
}
