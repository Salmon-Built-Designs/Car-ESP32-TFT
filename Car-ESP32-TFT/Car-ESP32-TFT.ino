#include "FS.h"
#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <EEPROM.h>
#include "max6675.h"
#include "temp.h"
#include "oil.h"
#include "volts.h"
#include "music.h"
#include "settings.h"
#include "reset.h"
#include "back.h"

Adafruit_ADS1115 ads(0x48);
OneWire oneWire(25);
DallasTemperature sensors(&oneWire);
TaskHandle_t Task1;
TFT_eSPI tft = TFT_eSPI();
MAX6675 ktc(27, 16, 17); // SCK, CS, MISO
byte Thermo1[8] = { 0x28, 0xFF, 0xB5, 0x22, 0x03, 0x19, 0x8A, 0xEA };
byte Thermo2[8] = { 0x28, 0xFF, 0xD8, 0x31, 0x03, 0x19, 0x8A, 0xA7 };

#define CALIBRATION_FILE "/TouchCalData3"
#define REPEAT_CAL false
#define TFT_GREY 0x5AEB

float R1 = 110000.0, R2 = 11000.0, R3 = 110000.0, R4 = 11000.0;
float value1, value3, vout1, vout3;
float vmin1 = 20.0, vmax1 = 0.0, current_avg, vin1_avg, tempC, afr_avg, oilpressure_avg;
volatile float vin1, current, temp1, temp2, afr, oilpressure;
volatile double temp4;
volatile int avg, rpt, rptdelay, egtstate, afrstate;
int currentmax, power, powermax, screen;
int16_t adc0, adc1, adc2, adc3;
bool main_filled = false;
bool settings_filled = false;

float mv[] = { 0.0634, 0.1145, 0.1650, 0.2115, 0.2499, 0.3108, 0.3670, 0.4224, 0.4761, 0.5285, 0.5796, 0.6334, 0.6862, 0.7377, 0.7881, 0.8375, 0.8820, 0.9259, 0.9696, 1.0119, 1.0532, 1.0937, 1.1333, 1.1722, 1.2103, 1.2477, 1.2843, 1.3203, 1.3556, 1.3902, 1.4242, 1.4575, 1.4903, 1.5224, 1.5540, 1.5850, 1.6155, 1.6454, 1.6748, 1.7038, 1.7322, 1.7602, 1.7876, 1.8147, 1.8413, 1.8594, 1.8809, 1.9022, 1.9231, 1.9447, 1.9654 };
float bar[] = { 0.0, 0.2, 0.4, 0.6, 0.8, 1.0, 1.2, 1.4, 1.6, 1.8, 2.0, 2.2, 2.4, 2.6, 2.8, 3.0, 3.2, 3.4, 3.6, 3.8, 4.0, 4.2, 4.4, 4.6, 4.8, 5.0, 5.2, 5.4, 5.6, 5.8, 6.0, 6.2, 6.4, 6.6, 6.8, 7.0, 7.2, 7.4, 7.6, 7.8, 8.0, 8.2, 8.4, 8.6, 8.8, 9.0, 9.2, 9.4, 9.6, 9.8, 10.0 };

void setup()
{
	rpt = 999;
	while (!EEPROM.begin(4)) {
		true;
	}
	avg = EEPROM.read(0);
	rptdelay = EEPROM.read(1);
	egtstate = EEPROM.read(2);
	afrstate = EEPROM.read(3);
	ads.begin();
	sensors.begin();
	delay(500);
	sensors.setResolution(Thermo1, 9);
	sensors.setResolution(Thermo2, 9);
	digitalWrite(12, HIGH); // Touch controller chip select (if used)
	digitalWrite(14, HIGH); // TFT screen chip select
	tft.init();
	tft.setRotation(1);
	touch_calibrate();
	tft.setSwapBytes(true);
	xTaskCreatePinnedToCore(read_sensors, "Task1", 10000, NULL, 0, &Task1, 0);
	fill_main_screen();
}

void loop()
{
	uint16_t x, y;

	if (screen == 0)
	{
		if (main_filled)
		{
			rpt++;
			main_screen();
		}
		else
		{
			fill_main_screen();
		}
		if (tft.getTouch(&x, &y))
		{
			if ((x > 10) && (x < (10 + 32)))
			{
				if ((y > 198) && (y <= (198 + 32)))
				{
					//tft.fillCircle(x, y, 2, TFT_WHITE);
					//ESP.restart();
					vmin1 = 20.0; vmax1 = 0.0; currentmax = 0; powermax = 0; rpt = 999;
					main_screen();
				}
			}
			else if ((x > 278) && (x < (278 + 32)))
			{
				if ((y > 198) && (y <= (198 + 32)))
				{
					main_filled = false;
					screen = 1;
					return;
				}
			}
		}

	}
	else if (screen == 1)
	{
		if (!settings_filled)
		{
			fill_settings_screen();
		}
		if (tft.getTouch(&x, &y))
		{
			if ((x > 5) && (x < (5 + 38)))
			{
				if ((y > 5) && (y <= (5 + 32)))
				{
					settings_filled = false;
					rpt = 999;
					screen = 0;
					return;
				}
			}
			else if ((x > 170) && (x < (170 + 40)))
			{
				if ((y > 100) && (y <= (100 + 40)))
				{
					if (avg > 1)
					{
						avg--;
						redraw_avg();
					}
				}
				else if ((y > 54) && (y <= (54 + 40)))
				{
					if (rptdelay > 0)
					{
						rptdelay--;
						redraw_delay();
					}
				}
			}
			else if ((x > 270) && (x < (270 + 40)))
			{
				if ((y > 192) && (y <= (192 + 40)))
				{
					if (egtstate == 1)
					{
						egtstate = 0;
						tft.drawString("OFF",279,211,2);
					}
					else
					{
						egtstate = 1;
						tft.drawString("ON  ",279,211,2);
					}
					delay(200);
				}
				else if ((y > 146) && (y <= (146 + 40)))
				{
					if (afrstate == 1)
					{
						afrstate = 0;
						tft.drawString("OFF",279,166,2);
					}
					else
					{
						afrstate = 1;
						tft.drawString("ON  ",279,166,2);
					}
					delay(200);
				}
				else if ((y > 100) && (y <= (100 + 40)))
				{
					if (avg < 99)
					{
						avg++;
						redraw_avg();
					}
				}
				else if ((y > 54) && (y <= (54 + 40)))
				{
					if (rptdelay < 99)
					{
						rptdelay++;
						redraw_delay();
					}
				}
			}
			if ((x > 240) && (x < (240 + 70)))
			{
				if ((y > 4) && (y <= (4 + 35)))
				{
					tft.setTextColor(TFT_GREEN, TFT_BLACK);
					tft.drawString("ZAPISZ",253,22,2);
					EEPROM.write(0, avg);
					EEPROM.write(1, rptdelay);
					EEPROM.write(2, egtstate);
					EEPROM.write(3, afrstate);
					EEPROM.commit();
					delay(500);
					tft.setTextColor(TFT_CYAN, TFT_BLACK);
					tft.drawString("ZAPISZ",253,22,2);
				}
			}
		}
	}
}

void read_sensors(void * parameter) {
	for(;;)
	{
		sensors.requestTemperatures();
		
		temp1 = sensorValue(Thermo1);
		temp2 = sensorValue(Thermo2);
		temp1 = round(temp1);
		temp2 = round(temp2);
	}
}

void redraw_avg()
{
	tft.setTextPadding(tft.textWidth("88", 4));
	tft.drawNumber(avg,226,120,4);
	tft.setTextPadding(0);
	delay(50);
}

void redraw_delay()
{
	tft.setTextPadding(tft.textWidth("88", 4));
	tft.drawNumber(rptdelay,226,75,4);
	tft.setTextPadding(0);
	delay(50);
}

void fill_settings_screen()
{
	tft.fillScreen(TFT_BLACK);
	tft.pushImage(5,5,38,32,back);
	tft.setTextColor(TFT_CYAN, TFT_BLACK);
	tft.drawString("USTAWIENIA",64,25,4);

	tft.drawLine(0,44,320,44,TFT_GREY);
	tft.drawString("Opoznienie",10,75,4);
	tft.drawRoundRect(170,54,40,40,10,TFT_WHITE);
	tft.drawString("-",186,75,4);
	tft.setTextPadding(tft.textWidth("88", 4));
	tft.drawNumber(rptdelay,226,75,4);
	tft.setTextPadding(0);
	tft.drawRoundRect(270,54,40,40,10,TFT_WHITE);
	tft.drawString("+",284,75,4);

	tft.drawString("Srednia",10,120,4);
	tft.drawRoundRect(170,100,40,40,10,TFT_WHITE);
	tft.drawString("-",186,120,4);
	tft.setTextPadding(tft.textWidth("88", 4));
	tft.drawNumber(avg,226,120,4);
	tft.setTextPadding(0);
	tft.drawRoundRect(270,100,40,40,10,TFT_WHITE);
	tft.drawString("+",284,120,4);

	tft.drawString("AFR",10,165,4);
	tft.drawRoundRect(270,146,40,40,10,TFT_WHITE);
	if (afrstate == 1)
	{
		tft.drawString("ON",279,166,2);
	}
	else
	{
		tft.drawString("OFF",279,166,2);
	}
	
	tft.drawString("EGT",10,210,4);
	tft.drawRoundRect(270,192,40,40,10,TFT_WHITE);
	if (egtstate == 1)
	{
		tft.drawString("ON",279,211,2);
	}
	else
	{
		tft.drawString("OFF",279,211,2);
	}
	
	tft.drawRoundRect(240,4,70,35,10,TFT_WHITE);
	tft.drawString("ZAPISZ",253,22,2);

	settings_filled = true;
}

void fill_main_screen()
{
	tft.fillScreen(TFT_BLACK);
	tft.pushImage(2,56,39,26,volts);
	tft.pushImage(5,5,32,30,temp);
	tft.pushImage(146,7,61,24,oil);
	tft.pushImage(5,101,30,32,music);
	tft.pushImage(10,198,32,32,reset);
	tft.pushImage(278,198,32,32,settings);
	tft.drawLine(0,46,320,46,TFT_GREY);
	tft.drawLine(0,93,320,93,TFT_GREY);
	tft.drawLine(0,141,320,141,TFT_GREY);
	tft.drawLine(0,187,320,187,TFT_GREY);
	main_filled = true;
}

void main_screen()
{
	if (rpt > rptdelay)
	{
		rpt = 0;
		
		for (int i=0; i < avg; i++)
		{
			adc0 = ads.readADC_SingleEnded(0);
			adc1 = ads.readADC_SingleEnded(1);
			adc2 = ads.readADC_SingleEnded(2);
			adc3 = ads.readADC_SingleEnded(3);
			value1 = adc0;
			value3 = adc1;
			
			if (afrstate == 1)
			{
				afr = ((adc3 * 0.1875)/1000) * 1.764 + 10.29;
			}
			
			vout1 = (value1 * 0.1875)/1000;
			vin1 = vout1 / (R2/(R1+R2));
			if (vin1 < 0.1) { vin1 = 0.0; }

			vout3 = (value3 * 0.1875)/1000;
			current = (vout3 - 2.528) / 0.02;
			if (current < 0.1) { current = 0; }
			
			oilpressure = 5.057 - (adc2 * 0.1875)/1000;
			
			if (afrstate == 1)
			{
				afr_avg = afr_avg + afr;
			}
			vin1_avg = vin1_avg + vin1;
			current_avg = current_avg + current;
			oilpressure_avg = oilpressure_avg + oilpressure;
		}

		if (afrstate == 1)
		{
			afr = afr_avg/avg;
		}
		vin1 = vin1_avg/avg;
		current = current_avg/avg;
		oilpressure = oilpressure_avg/avg;
		oilpressure = FmultiMap(oilpressure, mv, bar, 51);
				
		if (afrstate == 1)
		{
			afr_avg = 0.0;
		}
		vin1_avg = 0.0;
		current_avg = 0;
		oilpressure_avg = 0.0;

		if (vin1 < vmin1) { vmin1 = vin1; }
		if (vin1 > vmax1) { vmax1 = vin1; }
		if (current > currentmax) { currentmax = current; }
		power = vin1 * current / 2;
		if (power > powermax) { powermax = power; }
		
		if (egtstate == 1)
		{
			temp4 = ktc.readCelsius();
		}
		
		tft.setTextColor(TFT_CYAN, TFT_BLACK);

		tft.setTextDatum(MR_DATUM);
		tft.setTextPadding(tft.textWidth("88.88", 2));
		tft.setTextFont(2);
		tft.drawFloat(vmin1,2,100,70,2);
		tft.setTextPadding(0);
		tft.setTextDatum(ML_DATUM);
		tft.drawString("V",100,70,2);

		tft.setTextDatum(MR_DATUM);
		tft.setTextPadding(tft.textWidth("88.88", 4));
		tft.drawFloat(vin1,2,200,73,4);
		tft.setTextPadding(0);
		tft.setTextDatum(ML_DATUM);
		tft.drawString("V",200,73,4);

		tft.setTextDatum(MR_DATUM);
		tft.setTextPadding(tft.textWidth("88.88", 2));
		tft.drawFloat(vmax1,2,300,70,2);
		tft.setTextPadding(0);
		tft.setTextDatum(ML_DATUM);
		tft.drawString("V",300,70,2);

		tft.setTextDatum(MR_DATUM);
		tft.setTextPadding(tft.textWidth("88", 4));
		tft.drawNumber(current,100,120,4);
		tft.setTextPadding(0);
		tft.setTextDatum(ML_DATUM);
		tft.drawString("A",100,120,4);

		tft.setTextDatum(MR_DATUM);
		tft.setTextPadding(tft.textWidth("88", 2));
		tft.drawNumber(currentmax,160,118,2);
		tft.setTextPadding(0);
		tft.setTextDatum(ML_DATUM);
		tft.drawString("A",160,118,2);

		tft.setTextDatum(MR_DATUM);
		tft.setTextPadding(tft.textWidth("888", 4));
		tft.drawNumber(power,234,120,4);
		tft.setTextPadding(0);
		tft.setTextDatum(ML_DATUM);
		tft.drawString("W",234,120,4);

		tft.setTextDatum(MR_DATUM);
		tft.setTextPadding(tft.textWidth("888", 2));
		tft.drawNumber(powermax,295,118,2);
		tft.setTextPadding(0);
		tft.setTextDatum(ML_DATUM);
		tft.drawString("W",295,118,2);

		tft.setTextDatum(MR_DATUM);
		tft.setTextPadding(tft.textWidth("888", 4));
		tft.drawNumber(temp1,90,23,4);
		tft.setTextPadding(0);
		tft.setTextDatum(ML_DATUM);
		tft.drawString("`C",90,23,4);

		tft.setTextDatum(MR_DATUM);
		tft.setTextPadding(tft.textWidth("888", 4));
		tft.drawNumber(temp2,265,18,4);
		tft.setTextPadding(0);
		tft.setTextDatum(ML_DATUM);
		tft.drawString("`C",265,18,4);
		
		tft.setTextDatum(MR_DATUM);
		tft.setTextPadding(tft.textWidth("8.8888", 2));
		tft.drawFloat(oilpressure,1,262,36,2);
		tft.setTextPadding(0);
		tft.setTextDatum(ML_DATUM);
		tft.drawString(" BAR",262,36,2);

		if (afrstate == 1)
		{
			tft.drawString("AFR",1,167,4);
			tft.setTextDatum(MR_DATUM);
			tft.setTextPadding(tft.textWidth("88.8", 4));
			tft.drawFloat(afr,1,115,167,4);
			tft.setTextPadding(0);
			tft.setTextDatum(ML_DATUM);
		}
		
		if (egtstate == 1)
		{
			tft.drawString("EGT",180,167,4);
			tft.setTextDatum(MR_DATUM);
			tft.setTextPadding(tft.textWidth("888", 4));
			tft.drawNumber(temp4,290,167,4);
			tft.setTextPadding(0);
			tft.setTextDatum(ML_DATUM);
			tft.drawString("`C",290,167,4);
		}
	}
}

void touch_calibrate()
{
  uint16_t calData[5];
  uint8_t calDataOK = 0;

  // check file system exists
  if (!SPIFFS.begin()) {
    Serial.println("Formating file system");
    SPIFFS.format();
    SPIFFS.begin();
  }

  // check if calibration file exists and size is correct
  if (SPIFFS.exists(CALIBRATION_FILE)) {
    if (REPEAT_CAL)
    {
      // Delete if we want to re-calibrate
      SPIFFS.remove(CALIBRATION_FILE);
    }
    else
    {
      File f = SPIFFS.open(CALIBRATION_FILE, "r");
      if (f) {
        if (f.readBytes((char *)calData, 14) == 14)
          calDataOK = 1;
        f.close();
      }
    }
  }

  if (calDataOK && !REPEAT_CAL) {
    // calibration data valid
    tft.setTouch(calData);
  } else {
    // data not valid so recalibrate
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 0);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    tft.println("Touch corners as indicated");

    tft.setTextFont(1);
    tft.println();

    if (REPEAT_CAL) {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.println("Set REPEAT_CAL to false to stop this running again!");
    }

    tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println("Calibration complete!");

    // store data
    File f = SPIFFS.open(CALIBRATION_FILE, "w");
    if (f) {
      f.write((const unsigned char *)calData, 14);
      f.close();
    }
  }
}

float sensorValue (byte deviceAddress[])
{
	tempC = sensors.getTempC (deviceAddress);
	return tempC;
}

float FmultiMap(float val, float * _in, float * _out, uint8_t size)
{
	if (val <= _in[0]) return _out[0];
	if (val >= _in[size-1]) return _out[size-1];

	uint8_t pos = 1;
	while(val > _in[pos]) pos++;

	if (val == _in[pos]) return _out[pos];

	return (val - _in[pos-1]) * (_out[pos] - _out[pos-1]) / (_in[pos] - _in[pos-1]) + _out[pos-1];
}