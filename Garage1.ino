/*
  
  
  https://github.com/RobTillaart/Arduino/tree/master/libraries/DHTstable
  https://techtutorialsx.com/2017/12/02/esp32-arduino-interacting-with-a-ssd1306-oled-display/
*/
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>  
#include "SSD1306.h" 
#include <HCSR04.h>
#include <dht.h>

#define SCK     5    // GPIO5  -- SX127x's SCK
#define MISO    19   // GPIO19 -- SX127x's MISO
#define MOSI    27   // GPIO27 -- SX127x's MOSI
#define SS      18   // GPIO18 -- SX127x's CS
#define RST     14   // GPIO14 -- SX127x's RESET
#define DI00    26   // GPIO26 -- SX127x's IRQ(Interrupt Request)

#define BAND    433E6  //you can set band here directly,e.g. 868E6,915E6
#define PABOOST true

#define LED_PIN 25               // Built in LED pin
#define RELE_PIN 33
#define PIR_PIN 23               // PIR input pin (for IR sensor)
#define TRIGGER_PIN 12
#define ECHO_PIN 13

#define DHT22_PIN 17

dht DHT;

UltraSonicDistanceSensor distanceSensor(13, 12);

bool PIR_state = LOW;             // we start, assuming no motion detected

unsigned int counter = 0;
double distance = 0;

SSD1306 display(0x3c, 4, 15);

void setup() {
 
    Serial.begin(9600);

    // Initializing OLED
    
    pinMode(16,OUTPUT);
    digitalWrite(16, LOW);    // set GPIO16 low to reset OLED
    delay(50); 
    digitalWrite(16, HIGH); // while OLED is running, must set GPIO16 in high

    display.init();
    display.flipScreenVertically();  
    display.setFont(ArialMT_Plain_10);
    //logo();
    delay(1000);
    display.clear();

    // Initializing LoRa

    SPI.begin(SCK,MISO,MOSI,SS);
    LoRa.setPins(SS,RST,DI00);

    if (!LoRa.begin(BAND,PABOOST))
    {
      display.drawString(0, 0, "Starting LoRa failed!");
      display.display();
      while (1);
    }

    // Initializing pins
    
    pinMode(LED_PIN, OUTPUT);  // initialize digital pin LED 25 as an output.
    pinMode(RELE_PIN, OUTPUT);
    pinMode(PIR_PIN, INPUT);
    digitalWrite(RELE_PIN, LOW);

    delay(1000);
}

struct      // DHT22 errors counter
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


void loop(){
  delay(2000);
  digitalWrite(RELE_PIN, LOW);
    // PIR reading
    
    PIR_state = digitalRead(PIR_PIN);  // read input value

    // Ultrasounds reading
    
    double distance = distanceSensor.measureDistanceCm();
    
    // DHT22 reading
    
    Serial.print("DHT22, \t");

    uint32_t start = micros();
    int chk = DHT.read22(DHT22_PIN);
    uint32_t stop = micros();

    switch (chk)
    {
    case DHTLIB_OK:
        stat.ok++;
        Serial.print("OK,\t");
        break;
    case DHTLIB_ERROR_CHECKSUM:
        stat.crc_error++;
        Serial.print("Checksum error,\t");
        break;
    case DHTLIB_ERROR_TIMEOUT:
        stat.time_out++;
        Serial.print("Time out error,\t");
        break;
    default:
        stat.unknown++;
        Serial.print("Unknown error,\t");
        break;
    }

    // Print to serial monitor

    Serial.println(DHT.temperature, 1);
    Serial.println(DHT.humidity, 1);
    Serial.println(distance);
    Serial.println(PIR_state);

    // Print to OLED
    
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    
    display.drawString(0,0, "PIR: " + String(PIR_state));
    display.drawString(0,10, "Distance: " + String(distance));
    display.drawString(0,20, "Temp: " + String(DHT.temperature));
    display.drawString(0,30, "Hum: " + String(DHT.humidity));
    display.drawString(0,40, "Rele: ");
    display.drawString(0,50, "Counter: " + String(counter));
    display.display();
  

  
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
/*
    delay(500);*/

  // Send LoRa packet -- TODO
      
  counter++;
}
  
