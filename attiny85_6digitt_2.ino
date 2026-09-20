//butoane de reglaj
//optimizat cu gemini AI

#include <TM1637TinyDisplay6.h> //https://github.com/jasonacox/TM1637TinyDisplay/tree/master
#include <EEPROM.h>
#include <math.h>

#define CLK 3 //pin2
#define DIO 4 //pin3
#define RELAY_PIN 1 //pin6
#define THERMISTOR_PIN A1 //pin7
#define BUTTONS_PIN A0  //pin1  
//pin5 referinta externa

TM1637TinyDisplay6 display(CLK, DIO);

float targetTemp = 35.0; 
const float HYSTERESIS = 2.0;
const int EEPROM_ADDR = 0; 

unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 300; 

bool isAdjusting = false;
unsigned long adjustTimer = 0;
const unsigned long adjustDuration = 1000; // 1 secunda

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  analogReference(EXTERNAL); 

  display.begin();
  display.setBrightness(BRIGHT_HIGH);

  float savedTemp = 0;
  EEPROM.get(EEPROM_ADDR, savedTemp);
  if (!isnan(savedTemp) && savedTemp >= 10.0 && savedTemp <= 90.0) {
    targetTemp = savedTemp;
  }
}

void loop() {
  unsigned long currentMillis = millis();

  // --- 1. CITIRE BUTOANE CU PULL-UP LA 5V (Tensiuni care scad la apăsare) ---
  int btnVal = analogRead(BUTTONS_PIN);
  
  if (currentMillis - lastButtonPress > debounceDelay) {
    // Prag pentru ~3V (de exemplu între 550 și 700 în ADC) - Exemplu: Decrementare
    if (btnVal >= 550 && btnVal <= 700) { 
      targetTemp -= 1.0; 
      if (targetTemp < 10.0) targetTemp = 10.0;
      EEPROM.put(EEPROM_ADDR, targetTemp); 
      lastButtonPress = currentMillis;
      isAdjusting = true;
      adjustTimer = currentMillis;
    }
    // Prag pentru ~4V (de exemplu între 750 și 900 în ADC) - Exemplu: Incrementare
    else if (btnVal >= 750 && btnVal <= 900) { 
      targetTemp += 1.0; 
      if (targetTemp > 90.0) targetTemp = 90.0;
      EEPROM.put(EEPROM_ADDR, targetTemp); 
      lastButtonPress = currentMillis;
      isAdjusting = true;
      adjustTimer = currentMillis;
    }
  }

  if (isAdjusting && (currentMillis - adjustTimer > adjustDuration)) {
    isAdjusting = false;
  }

  // --- 2. CITIRE TEMPERATURĂ ȘI AFIȘAJ ---
  int rawADC = analogRead(THERMISTOR_PIN);
  
  if (rawADC > 0 && rawADC < 1023) {
    float currentTemp = calculeazaTemperatura(rawADC); 

    if (isAdjusting) {
      display.showNumber(targetTemp, 1, 4, 0); 
      uint8_t segments_degA[] = {
        SEG_A | SEG_B | SEG_F | SEG_G,                  
        SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G   
      };
      display.setSegments(segments_degA, 2, 4);
    } 
    else {
      display.showNumber(currentTemp, 1, 4, 0); 
      uint8_t segments_degC[] = {
        SEG_A | SEG_B | SEG_F | SEG_G, 
        SEG_A | SEG_D | SEG_E | SEG_F  
      };
      display.setSegments(segments_degC, 2, 4);
    }

    // --- 3. LOGICA DE HISTEREZIS ---
    if (currentTemp < (targetTemp - (HYSTERESIS / 2.0))) {
      digitalWrite(RELAY_PIN, HIGH); //pentru racire inlocuieste HIGH cu LOW
    } 
    else if (currentTemp > (targetTemp + (HYSTERESIS / 2.0))) {
      digitalWrite(RELAY_PIN, LOW);  // pentru racire inlocuieste LOW cu HIGH
    }
  } else {
    display.showString("Err"); 
  }

  delay(100); 
}

//presupunem un coeficient Beta B = 3950 și o rezistență de pull-up de 10K legată la cei 5V referinta

float calculeazaTemperatura(int raw) {
  float rezistentaTermistor = 10000.0 * (float)raw / (1023.0 - (float)raw);
  float steinhart = log(rezistentaTermistor / 10000.0) / 3950.0 + (1.0 / 298.15); //10000 se inlocuieste cu valoarea masurata a rezistorului serie cu termistorul
  return (1.0 / steinhart) - 273.15;
}