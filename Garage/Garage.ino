/*
  LoRa Garage Module v0.8 by Enrico Gallesio (IZ1ZCK)
  This file is part of repository https://github.com/fablab-imperia/LoRa-tests
  This sketch is intended to look for car/people presence in the garage, switch light on if appropriate 
  and sending presence conditions and temperature/humidity/telemetry data 
  to nearby LoRa devices on 433Mhz ISM band

  Sketch explaination:
  Ultrasounds distance and DHT22 sensors are always polled.
  Weather data and telemetry are automatically sent periodically (typically once every 15-30-60 mins...) via LoRa
  If distance measured is between a certain threshold, car is present. 
  In this case, switch light on in the garage, and keep it active for x minutes if car remains present. 
  If car is confirmed for some time LoRa notification is sent.
  Then switch light off, but if IR activity is detected, delay the switching off.
  If distance increases above x cm, car is gone. In this case switch light off almost immediately, then send LoRa msg.
  IR activity alone is not sufficient to switch on the light (cats roaming the garage!)
  LoRa messages contain a customized identifier that can be avoided if local address is used. 
  Each message is sent twice to avoid in freq. collision, but should be fixed with ACK receipt.
    
  see also:
  https://github.com/Martinsos/arduino-lib-hc-sr04
  https://github.com/RobTillaart/Arduino/tree/master/libraries/DHTstable
  https://techtutorialsx.com/2017/12/02/esp32-arduino-interacting-with-a-ssd1306-oled-display/

  TODO: - Put all relay switch commands in one place
        - Set lights on if only IR activity is detected, but find a way to avoid cats!
        - See all "TODO" tags within the code below
        - Set lights on remotely
  
*/

#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>  
#include "SSD1306.h" 
#include <HCSR04.h>
#include <dht.h>


// Configuration
#define VERBOSE_OUTPUT      0       // TODO TOTIDY - DEBUG mode 0: only errors - mode 1: errors + car&lights log - mode 2: verbose
#define OLED_WARM_UP        2000    // OLED warm-up time in Setup
#define SENSORS_WARM_UP     2000    // Additional Sensors warm-up time in Setup
#define OLED_INIT_SYNC      50      // OLED initialization syncing time in Setup. DO NOT CHANGE

#define ULTRASOUNDS_TIME    500     // Min time to poll ultrasounds
#define PIR_PHOTOR_TIME     500     // Min time to poll Infrared PIR sensor
#define DHT22_TIME          5000    // Min time to poll temperature and humidity DHT22 sensor
#define OLED_TIME           1000    // Min time to update OLED display
#define LED_FLASH           1000     // Duration of status led flash
#define LIGHT_OFF_TIME      1000    // Min time lights stay off TODO - erratic switching off delay
#define LIGHT_ON_CAR0_TIME  10000   // Min time lights stay on with car present
#define LIGHT_ON_CAR1_TIME  120000  // Min time lights stay on with car away
#define LORA_WEATHER_TIME   900000  // Min time to send LoRa weather data
#define LORA_CAR_TIME       5000    // Min time to consider car present/away and send LoRa "Car arrived/went away" msg
#define LORA_REPEAT         2000    // Interval between 2 messages sent

#define MIN_PHOTORES        2000    // Min or below photores value to consider night/dark condition
#define MAX_PHOTORES        2500    // Max or above photores value to consider daylight condition


// Pins
#define OLED_PIN      16
#define LIGHT_PIN     25    // Same as led pin because it helps trigging correctly the relay switch 
#define PHOTORES_PIN  12
#define PIR_PIN       23    // PIR input pin (for IR sensor)
#define TRIGGER_PIN   21
#define ECHO_PIN      13
#define DHT22_PIN     17
#define LED_PIN       33    // not builtin led


#define SCK           5     // GPIO5  -- LoRa SX127x's SCK   // LoRa module integrated in Heltec ESP32 board
#define MISO          19    // GPIO19 -- LoRa SX127x's MISO  // Do not change any of the following pins
#define MOSI          27    // GPIO27 -- LoRa SX127x's MOSI
#define SS            18    // GPIO18 -- LoRa SX127x's CS
#define RST           14    // GPIO14 -- LoRa SX127x's RESET
#define DI00          26    // GPIO26 -- LoRa SX127x's IRQ(Interrupt Request)


// LoRa
#define BAND          43305E4  //you can set band here directly,e.g. 868E6,915E6
#define SYNC_WORD     0xAA
#define PABOOST       true


// Modules
  SSD1306 display(0x3c, 4, 15);
  dht DHT;
  UltraSonicDistanceSensor distanceSensor(13, 21);


// Global vars                          // TODO look for unused vars
  unsigned long currentMillis = 0;
  unsigned int counter = 0;             // progressive number on OLED
  long previousMillis_ultrasounds = 0;  // vars to compute elapsed time
  long previousMillis_pir = 0;
  long previousMillis_dht22 = 0;
  long previousMillis_lightoff = 0;
  long previousMillis_lightoncar0 = 0;
  long previousMillis_lightoncar1 = 0;
  long previousMillis_oled = 0;
  long previousMillis_loraweather = 0;
  long last_valid_dht22Millis = 0;
  long last_car_changeMillis = 0;
  double distance = 0;  
  double temp_deg = 0;
  double humid_perc = 0; 
  int photores_val = 0;                 // photoresistor light reading
  String custom_id = "GA1";             // set a define? TODO
  bool PIR_state = false;               // we start, assuming no motion detected
  bool light_state = false;
  bool light_prev = false;
  bool car_state = false;
  bool car_prev = false;
  bool car_lora_state = false;
  bool car_lora_prev = false;
  bool light_inhibit = false;
  bool night_mode = false;
    
  struct                      // DHT22 errors counter
    {
        uint32_t total;
        uint32_t ok;
        uint32_t crc_error;
        uint32_t time_out;
        uint32_t connect;
        uint32_t ack_l;
        uint32_t ack_h;
        uint32_t unknown;
    } stat = { 0,0,0,0,0,0,0,0};


//Functions

bool inRange(int val, int minimum, int maximum)
{
  return ((minimum <= val) && (val <= maximum));
}

void ledFlash()
{
  digitalWrite(LED_PIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(LED_FLASH);                       // wait for a second
  digitalWrite(LED_PIN, LOW);    // turn the LED off by making the voltage LOW
}

void sendMessage(String outgoing)       // Repeated twice to avoid conflicts in freq. TODO remove fix with ACK!
{
  LoRa.beginPacket();                   // start packet
  LoRa.print(custom_id);                // add customized sender identifier
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  Serial.print("LoRa Packet Sent: ");
  ledFlash();
  delay(LORA_REPEAT);
  LoRa.beginPacket();                   // start packet
  LoRa.print(custom_id);                // add customized sender identifier
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  Serial.print("LoRa Packet Sent (second try): ");
  ledFlash();
  Serial.println(custom_id + outgoing);
}


void setup() {

    Serial.begin(9600);
    Serial.println("-- GARAGE LoRa Module --");
    Serial.println("");
    
    
    // Initializing pins
    digitalWrite(LIGHT_PIN, HIGH);
    pinMode(LIGHT_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    pinMode(PIR_PIN, INPUT);
    // pinMode(LED_PIN, OUTPUT);
    
    
    // Initializing OLED - Sometimes OLED does not wakeup at startup - TODO
    pinMode(OLED_PIN,OUTPUT);
    digitalWrite(OLED_PIN, LOW);    // set GPIO16 low to reset OLED
    delay(OLED_INIT_SYNC); 
    digitalWrite(OLED_PIN, HIGH); // while OLED is running, must set GPIO16 in high
    display.init();
    display.flipScreenVertically();  
    display.setFont(ArialMT_Plain_10);
    //logo(); TODO?
    delay(OLED_WARM_UP);
    display.clear();
    display.drawString(0, 0, "Setup ongoing");
    display.display();
    

    // Initializing LoRa
    SPI.begin(SCK,MISO,MOSI,SS);
    LoRa.setPins(SS,RST,DI00);
    if (!LoRa.begin(BAND,PABOOST))
    {
      display.drawString(0, 10, "Starting LoRa failed!");
      Serial.println("Starting LoRa failed! Crashing here");
      display.display();
      while (1);
    }
    LoRa.setSyncWord(SYNC_WORD);           // ranges from 0-0xFF, default 0x34, see API docs

    delay(SENSORS_WARM_UP);
    
}

void loop(){

  currentMillis = millis();         // current elapsed time since reboot in milliseconds. used to avoid delays

  if (!night_mode) {                // are we in dark or light conditions?
    if (photores_val < MIN_PHOTORES)
      {
        night_mode = true;
        Serial.println("NIGHT MODE ON!");
      }
  }
  else
  {
    if (photores_val > MAX_PHOTORES)
      {
        night_mode = false;
        Serial.println("NIGHT MODE OFF!");
      }
  }


  // PIR & PHOTORESISTOR polling
  if(currentMillis - previousMillis_pir > PIR_PHOTOR_TIME) {
    previousMillis_pir = currentMillis;
    PIR_state = digitalRead(PIR_PIN);  // read input value
    photores_val = analogRead(PHOTORES_PIN);
    if (VERBOSE_OUTPUT > 1) {
      //Serial.print("PIR polled! Condition is: ");             //DEBUG
      //Serial.println(PIR_state);
      Serial.print("Photores value: ");
      Serial.println(photores_val);
    }
  }


  // ULTRASOUNDS distance polling
  if(currentMillis - previousMillis_ultrasounds > ULTRASOUNDS_TIME) {
    previousMillis_ultrasounds = currentMillis;
    distance = distanceSensor.measureDistanceCm();
    if (VERBOSE_OUTPUT > 1) {
      Serial.print("Ultrasounds polled! Distance is: ");
      Serial.print(distance);
      Serial.println(" cm");
    }
  }


  // Time to SWITCH LIGHT OFF with car GONE?
  if(currentMillis - previousMillis_lightoncar0 > LIGHT_ON_CAR0_TIME) {
    previousMillis_lightoncar0 = currentMillis;

    if (!PIR_state)
    {
      if (!car_state) {
        light_state = false;
        if (light_state != light_prev) {
            //if (VERBOSE_OUTPUT) {
              Serial.print("Light is ON, car is gone. Switching OFF: ");
              Serial.println(light_state);
            //}
              if (night_mode) {
                 digitalWrite(LIGHT_PIN, !light_state);    // Relay is activated with LOW
                }
                else
                {
                  Serial.println("It's daytime! Skipping switch off");
                }
            light_prev = light_state;
        }
      }
    }
    else
    {
      //if (VERBOSE_OUTPUT) 
      Serial.println("PIR still active, switch off skipped...");
    }
    
  }


   // Time to SWITCH LIGHT OFF with car PRESENT?
  if(currentMillis - previousMillis_lightoncar1 > LIGHT_ON_CAR1_TIME) {
    previousMillis_lightoncar1 = currentMillis;

    if (!PIR_state)
    {
      if (car_state) {
        light_state = false;
        if (light_state != light_prev) {
            //if (VERBOSE_OUTPUT) {
              Serial.print("Light is ON! Max timout elapsed. Now switching OFF: ");
              Serial.println(light_state);
            //}
             if (night_mode) {
                 digitalWrite(LIGHT_PIN, !light_state);    // Relay is activated with LOW
                }
                else
                {
                  Serial.println("It's daytime! Skipping switch off");
                }
        
            light_prev = light_state;
        }
      }
    }
    else
    {
      //if (VERBOSE_OUTPUT) 
      Serial.println("PIR still active, switch off skipped...");
    }
    
  }


  // Time to SWITCH LIGHT ON?
  if(currentMillis - previousMillis_lightoff > LIGHT_OFF_TIME) {
    previousMillis_lightoff = currentMillis;

    if (car_state)
    {
      light_state = true;
      if (light_state != light_prev) {
        //if (VERBOSE_OUTPUT) {
        Serial.print("Light is OFF, car is here! Now switching ON: ");
        Serial.println(light_state);
        //}
        if (night_mode) {
        digitalWrite(LIGHT_PIN, false);      // Relay is activated with LOW
        }
        else
        {
          Serial.println("It's daytime! Skipping switch on");
        }
        light_prev = light_state;
      }
    }  
   }
   
    
  // DHT22 polling and Status led blink
  if(currentMillis - previousMillis_dht22 > DHT22_TIME) {
    previousMillis_dht22 = currentMillis;
    
    uint32_t start = micros();              //DHT22 error check procedure
    int dht22_chk = DHT.read22(DHT22_PIN);
    uint32_t stop = micros();
    if (VERBOSE_OUTPUT > 1) Serial.print("DHT22 polled! State is: ");
    
    switch (dht22_chk)
    {
      case DHTLIB_OK:
          stat.ok++;
          last_valid_dht22Millis = currentMillis;
          temp_deg = (DHT.temperature);
          humid_perc = (DHT.humidity); 
          if (VERBOSE_OUTPUT > 1) Serial.println("OK\t");
          break;
      case DHTLIB_ERROR_CHECKSUM:
          stat.crc_error++;
          Serial.println("DHT22 Checksum error\t");
          break;
      case DHTLIB_ERROR_TIMEOUT:
          stat.time_out++;
          Serial.println("DHT22 Time out error\t");
          break;
      default:
          stat.unknown++;
          Serial.println("DHT22 Unknown error\t");
          break;
    }
    
    if (stat.total % 20 == 0)
    {
        if (VERBOSE_OUTPUT) {
        Serial.println("DHT22 Errors stats");
        Serial.println("\nTOT\tOK\tCRC\tTO\tUNK");
        Serial.print(stat.total);
        Serial.print("\t");
        Serial.print(stat.ok);
        Serial.print("\t");
        Serial.print(stat.crc_error);
        Serial.print("\t");
        Serial.print(stat.time_out);
        Serial.print("\t");
        Serial.print(stat.connect);
        Serial.print("\t");
        Serial.print(stat.ack_l);
        Serial.print("\t");
        Serial.print(stat.ack_h);
        Serial.print("\t");
        Serial.print(stat.unknown);
        Serial.println("\n");
        }
    }

    if (VERBOSE_OUTPUT) {
      Serial.print("Temperature is: ");
      Serial.print(temp_deg);
      Serial.println("°C");
      Serial.print("Humidity is: ");
      Serial.print(humid_perc);
      Serial.println("%");
      Serial.print("Last valid DHT22 reading was: ");
      Serial.print((currentMillis - last_valid_dht22Millis)/1000);
      Serial.println(" secs ago.");
    }
    
    ledFlash();
  }

  
  // Print sensors condition to OLED
  if(currentMillis - previousMillis_oled > OLED_TIME) {
    previousMillis_oled = currentMillis;
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    
    display.drawString(0,0, "PIR: " + String(PIR_state) + " Rele: " + String(light_state));
    display.drawString(0,10, "Distance: " + String(distance));
    display.drawString(0,20, "Temp: " + String(temp_deg));
    display.drawString(0,30, "Hum: " + String(humid_perc));
    display.drawString(0,40, "Lumin: " + String(photores_val));
    display.drawString(0,50, "Counter: " + String(counter));
    display.display();
    counter++;
  }


 // Look for the car
    if (inRange(distance, 5, 100))
    {
      car_state = true;
      if (car_prev != car_state) {
        Serial.print("Car is present! :D - distance is: ");
        Serial.println(distance);
        last_car_changeMillis = currentMillis;
      }
      car_prev = car_state;
    }
    else if (distance < 0) {}
    else
    {
      car_state = false;
        if (car_prev != car_state) {
        Serial.print("Car gone away... :( - distance is: ");
        Serial.println(distance);
      }
      car_prev = car_state;
    }
  
  
  // Time to send WEATHER via LoRa?
  if (currentMillis - previousMillis_loraweather > LORA_WEATHER_TIME) {
    previousMillis_loraweather = currentMillis;
    Serial.println("Time to send LoRa Weather!");
    if (VERBOSE_OUTPUT > 1) {
      Serial.print("Temperature is: ");
      Serial.print(temp_deg);
      Serial.println("°C");
      Serial.print("Humidity is: ");
      Serial.print(humid_perc);
      Serial.println("%");
      Serial.print("Last valid DHT22 reading was: ");
      Serial.print((currentMillis - last_valid_dht22Millis)/1000);
      Serial.println(" secs ago.");
    }
    // PACKET SEND WEATHER + telemetry
      String message = ";T" + String(temp_deg) + ";H" + String(humid_perc) + ";V" + String((currentMillis - last_valid_dht22Millis)/1000) + ";C" + String(car_state) + ";D" + String(distance) + ";N" + String(night_mode) + ";L" + String(photores_val);
      sendMessage(message);
  }
 
  
  
  // Time to send car present notification?
  if (car_state) {
    if (currentMillis-last_car_changeMillis > LORA_CAR_TIME) { // to get a more stable detection of car, allow some ultrasound cycles to perform
      last_car_changeMillis = currentMillis;
      car_lora_state = true;
      if (car_lora_state != car_lora_prev) {
        Serial.print("Car appears arrived since a while... Sending LoRa packet! "); 
        Serial.println(car_state);
                
        // PACKET SEND CAR STATE - TODO 
          String message = ";C1";
          sendMessage(message);
          
        car_lora_prev = car_lora_state;
      }
    }
  }
  else
  {
    if (currentMillis-last_car_changeMillis > LORA_CAR_TIME) {  // to get a more stable detection of car, allow some ultrasound cycles to perform 
        last_car_changeMillis = currentMillis;
        car_lora_state = false;
        if (car_lora_state != car_lora_prev) {
          Serial.print("Car appears gone away since a while... Sending LoRa packet! ");
          Serial.println(car_state);
          
          // PACKET SEND CAR STATE
          String message = ";C0";
          sendMessage(message);
          
          car_lora_prev = car_lora_state;
        }
    }
  }

} // end of loop
  
