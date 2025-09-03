#include <Wire.h>
#include <HDC2080.h>
#include <EEPROM.h>
#include "Adafruit_CCS811.h"

#define HDC2080_ADR 0x40  // Endereço do Sensor Temp & Hum
HDC2080 sensorTH(HDC2080_ADR);

#define PIN_WKE 6
#define PIN_RST 7
#define BASE_ADDR 5  
#define CCS811_DRIVE_MODE_IDLE    0
#define CCS811_DRIVE_MODE_1SEC    1
#define CCS811_DRIVE_MODE_10SEC   2
#define CCS811_DRIVE_MODE_60SEC   3
#define CCS811_DRIVE_MODE_RAW     4
uint8_t mode = 1;
uint8_t x = 0;
uint16_t CO2 = 400;
uint16_t TVOC = 0;
uint16_t CO2A = 400;
uint16_t TVOCA = 0;
float temp;
float humid;

Adafruit_CCS811 ccs;

void setMode(uint8_t modeN) {
  if (modeN > mode) {
    Serial.println("Sensor a entrar em modo de espera para diminuir a sample rate");
    ccs.setDriveMode(0);
    delay(600000);
    ccs.setDriveMode(modeN);
    mode = modeN;
    
    switch(mode) {
    case CCS811_DRIVE_MODE_10SEC:
      Serial.println("1 medicao a cada 10s");
      break;
    case CCS811_DRIVE_MODE_60SEC:
      Serial.println("1 medicao a cada 60s");
      break;
    case CCS811_DRIVE_MODE_RAW:
      Serial.println("RAW");
      break;
}
}
  else if (modeN < mode) {
    Serial.println("Sensor a aumentar a sample rate");
    ccs.setDriveMode(modeN); 
    mode = modeN;
    
    switch(mode) {
    case CCS811_DRIVE_MODE_1SEC:
      Serial.println("1 medicao a cada 1s");
      break;
    case CCS811_DRIVE_MODE_10SEC:
      Serial.println("1 medicao a cada 10s");
      break;
}   
  }
}

void startHDC2080(){
  sensorTH.begin();
  sensorTH.reset();

  sensorTH.setHighTemp(50);
  sensorTH.setLowTemp(15);
  sensorTH.setHighHumidity(100);
  sensorTH.setLowHumidity(30);

  sensorTH.setMeasurementMode(TEMP_AND_HUMID);  // Set measurements to temperature and humidity
  sensorTH.setRate(5);                     // Set measurement frequency to 1 Hz
  sensorTH.setTempRes(FOURTEEN_BIT);
  sensorTH.setHumidRes(FOURTEEN_BIT);

  sensorTH.triggerMeasurement();
}

void loadBaseline() {
  uint16_t base;
  EEPROM.get(BASE_ADDR, base);
  if (base != 0xFFFF && base != 0x0000) {
    ccs.setBaseline(base);
    Serial.print("Baseline reposta: 0x");
    Serial.println(base, HEX);
  }
}

void saveBaseline() {
  uint16_t base = ccs.getBaseline();
  EEPROM.put(BASE_ADDR, base);
  Serial.println("Baseline guardada");
}


void setup() {
  Serial.begin(9600);
  Wire.begin();
  startHDC2080();
  delay(1000);

  // Configura pinos de controle
  pinMode(PIN_WKE, OUTPUT);
  digitalWrite(PIN_WKE, LOW);  // Wake LOW = ativo

  pinMode(PIN_RST, OUTPUT);
  digitalWrite(PIN_RST, LOW);  // Reset LOW
  delay(10);                   // Aguarda
  digitalWrite(PIN_RST, HIGH); // Libera reset
  delay(100);                  

  // Inicializa o sensor
  if (!ccs.begin()) {
    Serial.println("Erro ao iniciar o sensor. Verifique as conexões!");
    while (1);
  }

  // Aguarda o sensor estar pronto
  while (!ccs.available()) {
    delay(10);
  }

  Serial.print("Sensor iniciado com sucesso! ");
  loadBaseline();
  Serial.println("A fazer por default 1 medicao por segundo");
}

void loop() {

String comando = Serial.readStringUntil('\n');
comando.trim();

temp = sensorTH.readTemp();
humid = sensorTH.readHumidity();
ccs.setEnvironmentalData(humid, temp);

  if (ccs.available()) {
    if (!ccs.readData()) {
      CO2 = ccs.geteCO2();
      TVOC = ccs.getTVOC();
      Serial.print("eCO2: ");
      Serial.print(CO2);
      Serial.print(" ppm, TVOC: ");
      Serial.print(TVOC);
      Serial.print(" ppb. ");
      Serial.print("T = ");
      Serial.print(temp);
      Serial.print("ºC | humid = ");
      Serial.print(humid);
      Serial.println("%.");
    } else {
      Serial.println("Erro de leitura");
    }
  }

  if (comando == "Calibrado") {
    saveBaseline();
}
  else if (comando == "SUJO" && x == 0) {
    Serial.println("O sensor está calibrado mas está num local sujo");
    x = 1;
  }
  
/*
    if ((CO2 > 1000 && CO2A > 900) || (TVOC > 1000 && TVOCA > 900 )) {
      setMode(1);
}
    else if ((CO2 > 1000 && CO2A < 950) || (TVOC > 1000 && TVOCA < 950 )) {
      return;
}
    else if ((CO2 > 750 && CO2A > 650) || (TVOC > 300 && TVOCA > 200)) {
      setMode(2);
}
    else if ((CO2 > 750 && CO2A < 700) || (TVOC > 300 && TVOCA < 250)) {
     return;
}
    else {
      setMode(3);
}
*/
  CO2A = CO2;
  TVOCA = TVOC;
}
