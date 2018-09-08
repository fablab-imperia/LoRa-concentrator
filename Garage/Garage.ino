/*
  LoRa Garage Module v0.2 by Enrico Gallesio (IZ1ZCK)
  This file is part of repository https://github.com/fablab-imperia/LoRa-tests
  This sketch is intended to look for car/people presence in the garage, 
  set lights on if appropriate and sending presence conditions and temperature/humidity data 
  to nearby LoRa devices on 433Mhz ISM band
  
  see also:
  https://github.com/Martinsos/arduino-lib-hc-sr04
  https://github.com/RobTillaart/Arduino/tree/master/libraries/DHTstable
  https://techtutorialsx.com/2017/12/02/esp32-arduino-interacting-with-a-ssd1306-oled-display/
*/

#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>  
#include "SSD1306.h" 
#include <HCSR04.h>
#include <dht.h>

// Delays configuration
#define OLED_WARM_UP        2000    // OLED warm-up time in Setup
#define SENSORS_WARM_UP     2000    // Additional Sensors warm-up time in Setup
#define OLED_INIT_SYNC      50      // OLED initialization syncing time in Setup. Do NOT change

#define ULTRASOUNDS_TIME    1000     // Min time to poll ultrasounds
#define PIR_TIME            2000    // Min time to poll Infrared PIR sensor
#define DHT22_TIME          5000    // Min time to poll temperature and humidity DHT22 sensor
#define OLED_TIME           1000    // Min time to update OLED display
#define LIGHT_OFF_TIME      1000    // Min time lights stay off
#define LIGHT_ON_TIME       5000    // Min time lights stay on
#define LORA_WEATHER_TIME   2000    // Min time to send LoRa weather data
#define LORA_CAR_TIME       2000    // Min time to send LoRa "Car arrived/went away" data

#define CYCLE_MIN_TIME      2000    // Minimum delay between each loop cycle

// Pins
#define LED_PIN       25               // Built in LED pin
#define OLED_PIN      16
#define LIGHT_PIN     25
#define PIR_PIN       23               // PIR input pin (for IR sensor)
#define TRIGGER_PIN   12
#define ECHO_PIN      13
#define DHT22_PIN     17

#define SCK           5    // GPIO5  -- LoRa SX127x's SCK   // LoRa module integrated in Heltec ESP32 board
#define MISO          19   // GPIO19 -- LoRa SX127x's MISO  // Do not change any of the following pins
#define MOSI          27   // GPIO27 -- LoRa SX127x's MOSI
#define SS            18   // GPIO18 -- LoRa SX127x's CS
#define RST           14   // GPIO14 -- LoRa SX127x's RESET
#define DI00          26   // GPIO26 -- LoRa SX127x's IRQ(Interrupt Request)

// LoRa
#define BAND          43305E4  //you can set band here directly,e.g. 868E6,915E6
#define PABOOST true

// Modules
  SSD1306 display(0x3c, 4, 15);
  dht DHT;
  UltraSonicDistanceSensor distanceSensor(13, 12);

// Global vars
  unsigned long currentMillis = 0;
  long previousMillis_ultrasounds = 0;
  long previousMillis_pir = 0;
  long previousMillis_dht22 = 0;
  long previousMillis_lightoff = 0;
  long previousMillis_lighton = 0;
  long previousMillis_oled = 0;
  long previousMillis_loraweather = 0;
  long previousMillis_loracar = 0;
  bool PIR_state = LOW;             // we start, assuming no motion detected
  unsigned int counter = 0;         // OLED number of updates counter
  double distance = 0;        
  bool light_state = false;
  bool light_prev = false;
  bool car_state = false;
  double temp_deg = 0;
  double humid_perc = 0;
    
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


void setup() {

    Serial.begin(9600);

    // Initializing pins
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LIGHT_PIN, HIGH);
    pinMode(LIGHT_PIN, OUTPUT);
    pinMode(PIR_PIN, INPUT);

    // Initializing OLED - Sometimes OLED does not wakeup at startup - TODO
    pinMode(OLED_PIN,OUTPUT);
    digitalWrite(OLED_PIN, LOW);    // set GPIO16 low to reset OLED
    delay(OLED_INIT_SYNC); 
    digitalWrite(OLED_PIN, HIGH); // while OLED is running, must set GPIO16 in high
    display.init();
    display.flipScreenVertically();  
    display.setFont(ArialMT_Plain_10);
    //logo();
    delay(OLED_WARM_UP);
    display.clear();

    // Initializing LoRa
    SPI.begin(SCK,MISO,MOSI,SS);
    LoRa.setPins(SS,RST,DI00);
    if (!LoRa.begin(BAND,PABOOST))
    {
      display.drawString(0, 0, "Starting LoRa failed!");
      Serial.println("Starting LoRa failed!");
      display.display();
      while (1);
    }

    delay(SENSORS_WARM_UP);
}

void loop(){

  currentMillis = millis(); //elapsed time since reboot in milliseconds. used to avoid delays

  // PIR polling
  if(currentMillis - previousMillis_pir > PIR_TIME) {
    previousMillis_pir = currentMillis;
    PIR_state = digitalRead(PIR_PIN);  // read input value
    Serial.print("PIR polled! Condition is: ");
    Serial.println(PIR_state);
  }

  // Ultrasounds polling
  if(currentMillis - previousMillis_ultrasounds > ULTRASOUNDS_TIME) {
    previousMillis_ultrasounds = currentMillis;
    distance = distanceSensor.measureDistanceCm();
    Serial.print("Ultrasounds polled! Distance is: ");
    Serial.print(distance);
    Serial.println(" cm");
  }

  // Time to switch light OFF?
  if(currentMillis - previousMillis_lighton > LIGHT_ON_TIME) {
    previousMillis_lighton = currentMillis;
    if (light_state != light_prev) {
      Serial.print("Light was ON! Now setting: ");
      Serial.println(light_state);
      digitalWrite(LIGHT_PIN, light_state);
      light_prev = light_state;
    }
  }

  // Time to switch light ON?
  if(currentMillis - previousMillis_lightoff > LIGHT_OFF_TIME) {
    previousMillis_lightoff = currentMillis;
    if (light_state != light_prev) {
      Serial.print("Light was OFF! Now setting: ");
      Serial.println(light_state);
      digitalWrite(LIGHT_PIN, light_state);
      light_prev = light_state;
    }
  }
    
  // DHT22 polling
  if(currentMillis - previousMillis_dht22 > DHT22_TIME) {
    previousMillis_dht22 = currentMillis;
    
    uint32_t start = micros();              //DHT22 error check procedure
    int dht22_chk = DHT.read22(DHT22_PIN);
    uint32_t stop = micros();
    Serial.print("DHT22 polled! State is: ");
    
    switch (dht22_chk)
    {
      case DHTLIB_OK:
          stat.ok++;
          Serial.println("OK\t");          // remove tabs? - TODO
          break;
      case DHTLIB_ERROR_CHECKSUM:
          stat.crc_error++;
          Serial.println("Checksum error\t");
          break;
      case DHTLIB_ERROR_TIMEOUT:
          stat.time_out++;
          Serial.println("Time out error\t");
          break;
      default:
          stat.unknown++;
          Serial.println("Unknown error\t");
          break;
    }

    temp_deg = (DHT.temperature);
    humid_perc = (DHT.humidity);
    Serial.print("Temperature is: ");
    Serial.print(temp_deg);
    Serial.println("°C");
    Serial.print("Humidity is: ");
    Serial.print(humid_perc);
    Serial.println("%");
  }
  
  // Print to OLED
  if(currentMillis - previousMillis_oled > OLED_TIME) {
    previousMillis_oled = currentMillis;
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    
    display.drawString(0,0, "PIR: " + String(PIR_state));
    display.drawString(0,10, "Distance: " + String(distance));
    display.drawString(0,20, "Temp: " + String(temp_deg));
    display.drawString(0,30, "Hum: " + String(humid_perc));
    display.drawString(0,40, "Rele: " + String(light_state));
    display.drawString(0,50, "Counter: " + String(counter));
    display.display();
    counter++;
  }


  // Conditions to set light (lights)  

  /* TO REPLACE COMPLETELY - TODO

    if (distance > 5) {
      if (distance < 100) 
      {
        car_state = true;
  
            if (PIR_state) {
              light_state = true;
                    digitalWrite(LIGHT_PIN, LOW);    // turn the LED off by making the voltage LOW
                    delay(500); 
                    digitalWrite(LIGHT_PIN, HIGH);    // turn the LED off by making the voltage LOW
              light_state = false;      
            }
      }
      else
      {
        car_state = false;
        light_state = false; 
        digitalWrite(LIGHT_PIN, HIGH);    // turn the LED off by making the voltage LOW
      }
    car_state = false;
    light_state = false; 
    digitalWrite(LIGHT_PIN, HIGH);    // turn the LED off by making the voltage LOW
    }
    else
    {
      car_state = false;
      light_state = false;
      digitalWrite(LIGHT_PIN, HIGH);    // turn the LED off by making the voltage LOW
    }
*/


      
  /*
  PIR_state = digitalRead(PIR_PIN);  // read input value
  if (PIR_state == HIGH) {            // check if the input is HIGH
    digitalWrite(LED_PIN, HIGH);  // turn LED ON
    Serial.println("Motion detected!");
  } 
  else 
  {
    digitalWrite(LED_PIN, LOW); // turn LED OFF
    Serial.println("Motion ended!");
  }
  */

  // Send LoRa packet -- TODO
      

}
  
