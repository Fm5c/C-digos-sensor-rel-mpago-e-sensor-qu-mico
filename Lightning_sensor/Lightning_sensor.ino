/*******************************************************************
 ____  __    ____  ___  ____  ____   __   __ _  ____   __  
(  __)(  )  (  __)/ __)(_  _)(  _ \ /  \ (  ( \(__  ) / _\ 
 ) _) / (_/\ ) _)( (__   )(   )   /(  O )/    / / _/ /    \
(____)\____/(____)\___) (__) (__\_) \__/ \_)__)(____)\_/\_/
Project name: AS3935 lightning detection using Arduino Uno
Project page: https://electronza.com/as3935-lightning-detection-arduino-uno/
Description : Demo code for AS3935 lightning sensor
********************************************************************/

/* This Arduino sketch requires the AS3935 library from https://github.com/raivisr/AS3935-Arduino-Library
  Based on the original library and code by:
  LightningDetector.pde - AS3935 Franklin Lightning Sensor™ IC by AMS library demo code
  Copyright (c) 2012 Raivis Rengelis (raivis [at] rrkb.lv). All rights reserved.
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 3 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <SPI.h>
#include <AS3935.h>

void printAS3935Registers();

byte SPItransfer(byte sendByte);
int tunecap;
volatile bool novoEvento = false;
volatile int tipoEvento = 0;
volatile int dist = 0;
unsigned long tempoBloqueio = 0;
bool bloqueado = false;

void AS3935Irq();

AS3935 AS3935(SPItransfer,10,2);

void setup()
{
  Serial.begin(9600);
  SPI.begin();
  SPI.setDataMode(SPI_MODE1);
  SPI.setClockDivider(SPI_CLOCK_DIV16);
  SPI.setBitOrder(MSBFIRST);
  AS3935.reset();
  delay(10);
  AS3935.setIndoors();
  AS3935.registerWrite(AS3935_NF_LEV,2);
  if(!AS3935.calibrate())
    Serial.println("Tuning out of range, verificar as ligações");
  tunecap=AS3935.registerRead(AS3935_TUN_CAP);
  Serial.print("Tuning cap register is ");
  Serial.println(tunecap);

  AS3935.enableDisturbers();
  printAS3935Registers();

  attachInterrupt(0,AS3935Irq,RISING);
}

void loop()
{
    if (bloqueado && millis() < tempoBloqueio) {
      return;
    } else {
        bloqueado = false;
    }
    
    String comando = Serial.readStringUntil('\n');
    comando.trim();
    
    if (comando == "STOP")
{
      tempoBloqueio = millis() + 10000;
      bloqueado = true;
      novoEvento = false;
}
    else if (comando == "WAIT")
{
      tempoBloqueio = millis() + 4000;
      bloqueado = true;
      novoEvento = false;
}
    else if (novoEvento == true)
{
      if (tipoEvento == 1)
        Serial.println("Muito ruido");
      else if (tipoEvento == 2)
        Serial.println("Disturbio detectado");
      else if (tipoEvento == 3)
{
        if (dist == 1)
          Serial.println("Relampago a menos de 1km");
        if (dist == 63)
          Serial.println("Relampago fora de alcance");
        if (dist < 63 && dist > 1)
{
        Serial.print("Relampago detetado a cerca de ");
        Serial.print(dist,DEC);
        Serial.println(" km de distância");
}
}
      novoEvento = false;
      tipoEvento = 0;
}
    delay(1000);
    Serial.println("Em espera...");
}

void printAS3935Registers()
{
  int noiseFloor = 6; //Outdoor:5; Indoor:6; Sem Osciloscópio:3
  int spikeRejection = 6; //Outdoor:2; Indoor:8; Sem Osciloscópio:6
  int watchdogThreshold = 5; //Outdor:1; Indoor:6; Sem Osciloscópio:5
  AS3935.setNoiseFloor(noiseFloor);
  AS3935.setSpikeRejection(spikeRejection);
  AS3935.setWatchdogThreshold(watchdogThreshold);
  Serial.print("Noise floor em: ");
  Serial.println(noiseFloor,DEC);
  Serial.print("Spike rejection em: ");
  Serial.println(spikeRejection,DEC);
  Serial.print("Watchdog threshold em: ");
  Serial.println(watchdogThreshold,DEC);
}

byte SPItransfer(byte sendByte)
{
  return SPI.transfer(sendByte);
}

void AS3935Irq()
{
  int irqSource = AS3935.interruptSource();
  if (irqSource & 0b0001) tipoEvento = 1;
  else if (irqSource & 0b0100) tipoEvento = 2;
  else if (irqSource & 0b1000) tipoEvento = 3;
  dist = AS3935.lightningDistanceKm();
  novoEvento = true;
}
