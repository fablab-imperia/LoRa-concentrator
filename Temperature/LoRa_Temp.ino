// LoRa Temperature Sensor v0.8 by Enrico Gallesio (IZ1ZCK)
// This file is part of repository https://github.com/fablab-imperia/LoRa-concentrator
// This sketch is intended to send temperature data to nearby LoRa devices on 433Mhz ISM band
// Pin sets are optimized for "The Cheapest possible node" by Martijn Quaedvlieg
// https://www.thethingsnetwork.org/labs/story/build-the-cheapest-possible-node-yourself
// It is not intended to work with LoRaWAN... yet
//
// This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

// see also
// http://wiki.dragino.com/index.php?title=LoRa_Questions
// https://github.com/travisgoodspeed/loraham/issues/19  libraries comparison
// https://github.com/sandeepmistry/arduino-LoRa
// https://github.com/sandeepmistry/arduino-LoRa/blob/master/API.md

 
#include <SPI.h>
#include <OneWire.h>
#include <LoRa.h> 
#include <DallasTemperature.h>
#include "LowPower.h"  //removing power Led and voltage regulator, putting LoRa to sleep + powerdown allows 7,5 uA power consumption

#define ONE_WIRE_BUS 8
#define SLEEP_TIME_MINUTES 5   // low power mode duration between each tx

OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);

unsigned int sleepCounter;  
float temp_value;             // variables to handle temperature
String datastring="";
char temp_probe[10];
uint8_t dataoutgoing[10];
String custom_id = "OU1"; 
 
void setup() 
{
  while (!Serial);
  Serial.begin(9600);
  delay(1000);        //allow some time for onewire bus to initialize (D18B20 can take up to 750ms)

  Serial.println("Arduino LoRa TX Test!");
  
  LoRa.setPins(6, 5, 2);
    
  if (!LoRa.begin(43305E4)) {           //set 433.05 MHz
    Serial.println("Starting LoRa failed!");
    while (1);
    }
  Serial.println("LoRa radio init OK!");
  
  LoRa.setSyncWord(0xAA);           // ranges from 0-0xFF, default 0x34, see API docs
  Serial.println("LoRa radio sync word set!");

  LoRa.enableCrc();
  Serial.println("LoRa CRC check enabled!");
 
  sensors.begin(); 
}

void lora_tx(String outgoing)
{
  LoRa.beginPacket();
  LoRa.print(custom_id);
  LoRa.print(outgoing);
  LoRa.endPacket();
  Serial.println("Waiting for packet " + outgoing + " to complete..."); 
  delay(200); // is this necessary? TODO
}

float Vcc_probe() {                    // battery level monitoring
  signed long resultVcc;
  float resultVccFloat;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(10);                           // Wait for Vref to settle
  ADCSRA |= _BV(ADSC);                 // Convert
  while (bit_is_set(ADCSRA,ADSC));
  resultVcc = ADCL;
  resultVcc |= ADCH<<8;
  resultVcc = 1126400L / resultVcc;    // Back-calculate AVcc in mV
  resultVccFloat = (float) resultVcc / 1000.0; // Convert to Float
  return resultVccFloat;
}
 
 
void loop()
{
  Serial.println("------"); 
  Serial.print("Requesting new temperature..."); 
  sensors.requestTemperatures(); // Send command onewire global command to get temperature readings 
  temp_value = sensors.getTempCByIndex(0);
  //delay(1000);                 // Probably required after Low Power Mode to allow temperature sensor to re-initialize
  Serial.println("DONE"); 
  
  Serial.print("Temperature is: "); 
  Serial.println(temp_value);   // index because it is the first (only) sensor on onewire bus
  Serial.print("Vcc voltage level is: "); 
  Serial.println(Vcc_probe());
    
  
  datastring +=dtostrf(temp_value, 4, 2, temp_probe);  // ??????
  strcpy((char *)dataoutgoing,temp_probe);
  Serial.println("Sending to LoRa");
  String message = ";T" + String(temp_probe) + ";V" + String(Vcc_probe());
  lora_tx(message);        // Send packet - Add ACK TODO
  
  Serial.println("------"); 
  Serial.println("Enter sleep mode");
  
  LoRa.sleep();
  delay(200);
  for (sleepCounter = (SLEEP_TIME_MINUTES*60/8); sleepCounter > 0; sleepCounter--) //see https://github.com/rocketscream/Low-Power/issues/43
  {
    // example: 4 hours = 60x60x4 = 14400 s
    // 14400 s / 8 s = 1800
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);  
  }
  LoRa.idle();    //wake up lora from sleep? is this necessary? TODO
  delay(200); 
 
}
