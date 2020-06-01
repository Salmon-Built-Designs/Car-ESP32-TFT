#include <Wire.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <Adafruit_ADS1015.h>
//#include "max6675.h"

#include "temp.h"
#include "oil.h"
#include "volts.h"
#include "music.h"

Adafruit_ADS1115 ads(0x48);
Adafruit_ADS1115 ads2(0x4A);
TFT_eSPI tft = TFT_eSPI();
//MAX6675 ktc(13, 10, 12);

float R1 = 110000.0, R2 = 11000.0, R3 = 110000.0, R4 = 11000.0;
float value1, value2, value3, value4, value5, value6, vout1, vout2, vout3;
float vin1, vin2, vmin1 = 20.0, vmin2 = 20.0, vmax1 = 0.0, vmax2 = 0.0, temp1, temp2, temp3;
double temp4;
int avg = 10, current, currentmax, power, powermax;

void setup()
{
  ads.begin();
  ads2.begin();
  digitalWrite(12, HIGH); // Touch controller chip select (if used)
  digitalWrite(14, HIGH); // TFT screen chip select
  digitalWrite(5, HIGH); // SD card chips select, must use GPIO 5 (ESP32 SS)
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(true);
  tft.pushImage(0,57,45,30,volts);
  tft.pushImage(5,5,32,30,temp);
  tft.pushImage(148,5,66,26,oil);
  tft.pushImage(5,110,30,32,music);
}

void loop()
{
  int16_t adc0, adc1, adc2, adc3, adc20, adc21, adc22, adc23;
  adc0 = ads.readADC_SingleEnded(0);
  adc1 = ads.readADC_SingleEnded(1);
  //adc1 = ads.readADC_SingleEnded(2);
  adc20 = ads2.readADC_SingleEnded(0);
  adc21 = ads2.readADC_SingleEnded(1);
  adc22 = ads2.readADC_SingleEnded(2);
  value1 = adc0;
  //value2 = adc1;
  value3 = adc1;
  temp1 = (adc20 * 0.1875)/10;
  temp2 = (adc21 * 0.1875)/10;
  temp3 = (adc22 * 0.1875)/10;
  //temp4 = ktc.readCelsius();

/*
  int16_t average1, average2, average3, average4, average5, average6;

  for (int i=0; i < avg; i++) {
    average1 = average1 + ads.readADC_SingleEnded(0);
    average2 = average2 + ads.readADC_SingleEnded(1);
    average3 = average3 + ads.readADC_SingleEnded(2);
    average4 = average4 + ads2.readADC_SingleEnded(0);
    average5 = average5 + ads2.readADC_SingleEnded(1);
	average6 = average5 + ads2.readADC_SingleEnded(2);
  }

  value1 = average1/avg; value2 = average3/avg; value3 = average2/avg; value4 = average4/avg; value5 = average5/avg; value6 = average6/avg;
  temp1 = (value4 * 0.1875)/10; temp2 = (value5 * 0.1875)/10; temp3 = (value6 * 0.1875)/10;
*/

  vout1 = (value1 * 0.1875)/1000;
  vin1 = vout1 / (R2/(R1+R2));
  if (vin1 < 0.09) { vin1 = 0.0; }
  if (vin1 < vmin1) { vmin1 = vin1; }
  if (vin1 > vmax1) { vmax1 = vin1; }

/*
  vout2 = (value2 * 0.1875)/1000;
  vin2 = vout2 / (R4/(R3+R4));
  if (vin2 < 0.09) { vin2 = 0.0; }
  if (vin2 < vmin2) { vmin2 = vin2; }
  if (vin2 > vmax2) { vmax2 = vin2; }
*/
  
  vout3 = (value3 * 0.1875)/1000;
  current = (vout3 - 2.528) / 0.02;
  if (current < 0.1) { current = 0; }
  if (current > currentmax) { currentmax = current; }
  power = vin1 * current / 2;
  if (power > powermax) { powermax = power; }
    
  tft.setTextDatum(MR_DATUM);
  tft.setTextPadding(tft.textWidth("88.88", 2));
  tft.setTextFont(2);
  tft.drawFloat(vmin1,2,100,72,2);
  tft.setTextPadding(0);
  tft.setTextDatum(ML_DATUM);
  tft.drawString("V",100,72,2);
  
  tft.setTextDatum(MR_DATUM);
  tft.setTextPadding(tft.textWidth("88.88", 4));
  tft.drawFloat(vin1,2,200,72,4);
  tft.setTextPadding(0);
  tft.setTextDatum(ML_DATUM);
  tft.drawString("V",200,72,4);
  
  tft.setTextDatum(MR_DATUM);
  tft.setTextPadding(tft.textWidth("88.88", 2));
  tft.drawFloat(vmax1,2,300,72,2);
  tft.setTextPadding(0);
  tft.setTextDatum(ML_DATUM);
  tft.drawString("V",300,72,2);
  
  tft.setTextDatum(MR_DATUM);
  tft.setTextPadding(tft.textWidth("88", 4));
  tft.drawNumber(current,100,125,4);
  tft.setTextPadding(0);
  tft.setTextDatum(ML_DATUM);
  tft.drawString("A",100,125,4);
  
  tft.setTextDatum(MR_DATUM);
  tft.setTextPadding(tft.textWidth("88", 2));
  tft.drawNumber(currentmax,160,125,2);
  tft.setTextPadding(0);
  tft.setTextDatum(ML_DATUM);
  tft.drawString("A",160,125,2);

  tft.setTextDatum(MR_DATUM);
  tft.setTextPadding(tft.textWidth("888", 4));
  tft.drawNumber(power,235,125,4);
  tft.setTextPadding(0);
  tft.setTextDatum(ML_DATUM);
  tft.drawString("W",235,125,4);
  
  tft.setTextDatum(MR_DATUM);
  tft.setTextPadding(tft.textWidth("888", 2));
  tft.drawNumber(powermax,295,125,2);
  tft.setTextPadding(0);
  tft.setTextDatum(ML_DATUM);
  tft.drawString("W",295,125,2);

  tft.setTextDatum(MR_DATUM);
  tft.setTextPadding(tft.textWidth("888.8", 4));
  tft.drawFloat(temp1,1,105,22,4);
  tft.setTextPadding(0);
  tft.setTextDatum(ML_DATUM);
  tft.drawString("`C",105,22,4);
    
  tft.setTextDatum(MR_DATUM);
  tft.setTextPadding(tft.textWidth("888.8", 4));
  tft.drawFloat(temp2,1,280,22,4);
  tft.setTextPadding(0);
  tft.setTextDatum(ML_DATUM);
  tft.drawString("`C",280,22,4);
  
  tft.setTextDatum(MR_DATUM);
  tft.setTextPadding(tft.textWidth("888.8", 4));
  tft.drawFloat(temp3,1,280,200,4);
  tft.setTextPadding(0);
  tft.setTextDatum(ML_DATUM);
  tft.drawString("`C",280,200,4);

  delay(300);
}
