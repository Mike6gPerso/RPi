#include <SPI.h>

#include <RadioHead.h>
#include <RH_ASK.h>
#include <DHT.h>
#include <LowPower.h>

#define GROUP_ID 2 // Temperature + Humidity sensor
#define NODE_ID  1

#define DHTPIN 2 // what pin we're connected to
// Uncomment whatever type you're using!
//#define DHTTYPE DHT11 // DHT 11
#define DHTTYPE DHT22 // DHT 22 (AM2302)
//#define DHTTYPE DHT21 // DHT 21 (AM2301)
// Connect pin 1 (on the left) of the sensor to +5V
// NOTE: If using a board with 3.3V logic like an Arduino Due connect pin 1
// to 3.3V instead of 5V!
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor
// Initialize DHT sensor for normal 16mhz Arduino
DHT dht(DHTPIN, DHTTYPE);
// NOTE: For working with a faster chip, like an Arduino Due or Teensy, you
// might need to increase the threshold for cycle counts considered a 1 or 0.
// You can do this by passing a 3rd parameter for this threshold. It's a bit
// of fiddling to find the right value, but in general the faster the CPU the
// higher the value. The default for a 16mhz AVR is a value of 6. For an
// Arduino Due that runs at 84mhz a value of 30 works.
// Example to initialize DHT sensor for Arduino Due:
//DHT dht(DHTPIN, DHTTYPE, 30);
const int transmit_pin = 10;
const int transmit_en_pin = 12;
const int receive_pin = 5;

uint16_t speed = 2000;

RH_ASK driver(speed);

void setup() {

  Serial.begin(9600);
  dht.begin();
  //driver.init();
  if (!driver.init())
    Serial.println("init failed");
}
void loop() {

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    // Read temperature as Celsius
    float t = dht.readTemperature();
    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t) ){
      //Serial.println("Failed to read from DHT sensor!");
      return;
    }
     char tempHum [50];
     //snprintf(tempHum, 50, "%i,TS:%lu,G:%d,N:%d,H:%d,T:%i", ++nbLoop, millis(), GROUP_ID, NODE_ID,(int)(h*100), (int)(t*100)); 
     snprintf(tempHum, 50, "*#%lu*%i*%i*%i*%i##", millis(), GROUP_ID, NODE_ID,(int)(t*100), (int)(h*100)); 
     driver.send((uint8_t *)  tempHum, strlen(tempHum));
     //driver.send(tempHum, strlen(tempHum));
     driver.waitPacketSent();

    //Serial.print("Transmitting at speed:");
    //Serial.println(speed);
    Serial.print("Sending:");
    Serial.println(tempHum);
    Serial.print("strlen:");
    Serial.println(strlen(tempHum));
    Serial.print("Humidity: ");
    Serial.print(h);
    Serial.print(" %\t");
    Serial.print("Temperature: ");
    Serial.print(t);
    Serial.println(" *C");


    delay(1000 + NODE_ID);
    //delay(4000);
    // Enter power down state for 8 s with ADC and BOD module disabled
    //for(int i = 0; i < 7; i++) //56 sec
     LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF); 

    delay(100 + NODE_ID);
}

