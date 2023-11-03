/*************************************************************************
 * Code for a morbidostat adjusting temperature increase according to exhaust CO2 levels
 * Copyright (C) 2023 Kerem Bora
 * borakerem@hotmail.com
 *
 * Libraries used:
 * Melexis MLX90615 infra-red temperature sensor by Sergey Kiselev --> https://github.com/skiselev/MLX90615/blob/master/mlx90615.cpp
 * MH-Z19 Carbon dioxide sensor by Jonathan Dempsey --> https://github.com/WifWaf/MH-Z19
 * SevenSegmentTM1637 displayer library by Bram Harmsen --> https://github.com/bremme/arduino-tm1637
 * Arduion Wire library --> http://www.arduino.cc/en/Reference/Wire
 * Arduino SoftwareSerial library --> http://www.arduino.cc/en/Reference/SoftwareSerial
 *
 * This code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *************************************************************************/

// CO2
#include <MHZ19.h>
#include <SoftwareSerial.h>
#define RX_PIN 2                          // Rx pin of the MHZ19 (Tx pin should be attached to D2)
#define TX_PIN 3                          // Tx pin of the MHZ19 (Rx pin should attached to D3)
#define BAUDRATE 9600                     // Device to MH-Z19 Serial baudrate
MHZ19 myMHZ19;                            // Constructor for library
SoftwareSerial mySerial(RX_PIN, TX_PIN);  // serial for MH-Z19

// Display
#include "SevenSegmentTM1637.h"

//Temp
#include <Wire.h>
#include <mlx90615.h>
MLX90615 mlx = MLX90615();


const byte PIN_CLK = 7;                   // CLK pin for TM1637 display
const byte PIN_DIO = 6;                   // DIO pin for TM1637 display
SevenSegmentTM1637    display(PIN_CLK, PIN_DIO);
int Temp;
float Tset = 37.00;

int CO2Value;                             // variable to store the value coming from the CO2 sensor
int CO2Value2;                            // for the second measurement
int sensorPin = A1;                       // input pin for LDR
int sensorValue = 0;                      // variable to store the value coming from the light sensor
float ODValue;                            // variable to store the calculated OD value                          
float average;                            // for the calculation of the average CO2 value

int Duration = 0;
int i = 0;
int j = 0;
int a = 0;

void setup()
{
  Serial.begin(9600);                     // serial monitor display (to record experimental data on computer)
  // Start Co2
  display.begin();                        // initializes TM1637 display
  display.setBacklight(100);              // set the brightness to 100 %
  mySerial.begin(BAUDRATE);               // device to MH-Z19 serial start
  myMHZ19.begin(mySerial);                // Serial(Stream) refence must be passed to library begin().
  myMHZ19.autoCalibration();              // Turn auto calibration ON
  delay(100);                
  
  pinMode(13, OUTPUT);                    // LED
  pinMode (8, OUTPUT);                    // Heater
  pinMode (10, OUTPUT);                   // Air pump
  
  delay(100);
  
  pinMode(4, OUTPUT);                     //Waste  
  digitalWrite(4, LOW);
  pinMode(5, OUTPUT);                     //Feed
  digitalWrite(5, LOW);
  
  mlx.begin();
  delay(100);

}

void loop()
{
  display.begin();                                 // initializes TM1637 display
  display.setBacklight(100);                       // set the brightness to 100 %
  digitalWrite(13, HIGH);                          // turn the LED ON
  delay(500);                             
  sensorValue = analogRead(sensorPin) - 130;       // read the light transmission value from the sensor. Note: 130 is the transmission value when LED is OFF This number may change for your system  
  ODValue = 2 -log10(sensorValue / 9.7);           // calculating OD from % transmission.  Note: 970 is the maximum transmission value when there is only water in the bioreactor. Dividing it by 100 gives % transmission. This number may change for your system 
  delay(900);
  digitalWrite(13, LOW);                  // turn the LED off by making the voltage LOW
  delay (100);
  digitalWrite(10, HIGH);                 // air ventillation starts
  delay (19000);
  digitalWrite(10, LOW);                  // air ventillation stops
  tempcontrol();                          // This process takes 10 seconds and in my design I can expect that CO2 molecules will reach the sensor after this period
  CO2Value = myMHZ19.getCO2();            // read the CO2 value from the sensor
  delay(5000); 
  CO2Value2 = myMHZ19.getCO2();           // read the CO2 value from the sensor (For double checking that the population is doing well before increase the temperature)
  delay(200);
   
 if (CO2Value > 800 and CO2Value2 > 800) 
  {
    Tset = Tset + 0.02;                   // Temperature increases 0.02 C per hour, if detected CO2Value is above 800 ppm
  }

  average = (CO2Value + CO2Value2) /2;
  CO2Value = average;
  
 if (CO2Value < 550) {
  CO2Value = 9999;                        //ALARM --> CO2 measurement is not working or microbes are probably in danger..
  }
  
  Temp = int(100 * mlx.get_object_temp());
  
  delay(300);

  Serial.print(j);
  Serial.print(".hour,");
  Serial.print(CO2Value);
  Serial.print(","); 
  Serial.print(ODValue); 
  Serial.print(",");
  Serial.print(Temp); 
  Serial.print(","); 
  Temp = int(100 * Tset);
  Serial.println(Temp);


 
  while (i != 36)                         // timer for the hourly set temperature change (36 x 99 seconds in the cycles below + 36 seconds at the start of each loop)
  {
    i++;

    tempcontrol();                        // This process takes 10 seconds
    
    while (Duration != 1000)              // feed pump is on for 2 seconds
    {

      digitalWrite(4, HIGH);

      delay(1);

      digitalWrite(4, LOW);

      delay(1);

      Duration = Duration + 1;
    }

    delay(2000);
    
    Duration = 0;

    tempcontrol();                        // This process takes 10 seconds

    while (Duration != 2000)              //waste pump is on for 4 seconds
    {

      digitalWrite(5, HIGH);

      delay(1);

      digitalWrite(5, LOW);

      delay(1);

      Duration = Duration + 1;
    }

    delay(1000);
    
    Duration = 0;

    while (a != 7)                        // This cycle takes 7 x 10 = 70 seconds
    {
      a++;
      tempcontrol();
    }
    a = 0;

  }
  i = 0;
  j++;

}

void tempcontrol()                        // This process takes 10 seconds
{
  
  if (Tset > mlx.get_object_temp())       // Checks whether heating is necessary..
  {
    digitalWrite(8, HIGH);
    display.print("ON");
    delay(1000);
  }
  else
  {
    digitalWrite(8, LOW);
    display.print("OFF");
    delay(1000);
  }
  display.clear();                        // I needed the following data to monitor the experiment
  display.print("CO2");
  delay(500);  
  display.clear(); 
  display.print(CO2Value);                //displays the CO2 value on TM1637
  delay(1000);                      
  display.clear(); 
  display.print("OD");
  delay(500);
  display.clear();
  display.print(ODValue);                 //displays the OD value on TM1637
  delay(1000);
  display.clear();
  Temp = int(100 * mlx.get_object_temp());
  delay(1000);
  display.print("TEMP");                  //displays the TEMP value on TM1637
  delay(1000);
  display.clear();
  display.print(Temp);
  delay(1000);
  display.clear();
  Temp = int(100 * Tset);
  display.print(Temp);                    //displays the Tset value on TM1637
  delay(1000);
  display.clear();
  display.print(j);
  delay(1000);
  display.clear();
  display.print(i);
  delay(1000);
  display.clear();
}
