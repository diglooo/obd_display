#include <AltSoftSerial.h>
#include "ELMduino.h"
#include <U8g2lib.h>
#include "logo.h"

//electrical connecitons
//HC-05 pins to arduino
//bare HC-05 module works with 3.3v power and signals
//to power the module with 5V, use two diodes in series
#define HC05_RESET_PIN 2
#define HC05_KEY_PIN 10
#define HC05_TX 8
#define HC05_RX 9

//oled screen pins to arduino
//ebay: 2.42" OLED screen SSD1309 128x64
#define OLED_CS 3
#define OLED_DC 7
#define OLED_RES 6
#define OLED_SCK 5
#define OLED_DATA 4

//use arduino pins 8-9 as additional serial port
//pin 8: receive
//pin 9: transmit
AltSoftSerial HC05Serial;
#define ELM_PORT HC05Serial

#define SERIAL_LOOPBACK
#define DEBUG_MODE
#define SIMULATION

//uncomment to talk directly yo the HC-05 module in AT mode
#undef SERIAL_LOOPBACK

//uncomment to run the application in debug mode
#undef DEBUG_MODE

//uncomment to simulate display data
//if no ELM327 is available
#undef SIMULATION


#ifndef SERIAL_LOOPBACK
ELM327 myELM327;
//U8G2 constructor set in page mode to save RAM
U8G2_SSD1309_128X64_NONAME2_2_4W_SW_SPI  u8g2(U8G2_R2, OLED_SCK, OLED_DATA, OLED_CS, OLED_DC, OLED_RES);
#endif


//Application states
#define STATE_BOOTING 0
#define STATE_CONNECTING 1
#define STATE_CONNECTED 2
#define STATE_TEST_ELM_PROTO 8
#define STATE_CONN_ERROR 3
#define STATE_RUNNING 5
#define STATE_RUNNING_ERROR 6
#define STATE_ELM_ERROR 7
#define STATE_DELAY 100
uint8_t appCurrState = 0;


char tempstr[5];
float RPM = 0;
float ECT = 0;
float INTAKETEMP = 0;
float ENGINELOAD = 0;
float VBAT = 0;


double mapLimit(double x, double in_min, double in_max, double out_min, double out_max)
{
	if (x > in_max) return out_max;
	if (x < in_min) return out_min;
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void setup()
{
  pinMode(HC05_RESET_PIN, OUTPUT);
  pinMode(HC05_KEY_PIN, OUTPUT);

  //reset HC-05 module on power-up
  digitalWrite(HC05_KEY_PIN, LOW);
  digitalWrite(HC05_RESET_PIN, LOW);

  Serial.begin(115200);
  ELM_PORT.begin(38400);
  
#ifdef SERIAL_LOOPBACK
  ELM_PORT.println("AT+RESET");
  Serial.println("AT+RESET");
  delay(1000);
  ELM_PORT.println("AT+ROLE=1"); 
  Serial.println("AT+ROLE=1");
  delay(1000);
  ELM_PORT.println("AT+CMODE=0");
  Serial.println("AT+CMODE=0");
  delay(1000);
  ELM_PORT.println("AT+INIT");
  Serial.println("AT+INIT");
  delay(1000);
  ELM_PORT.println("AT+PSWD=1234");
  Serial.println("AT+PSWD=1234");
  delay(1000);
  ELM_PORT.println("AT+BIND=AB90,78,563412");
  Serial.println("AT+BIND=AB90,78,563412");
  delay(1000);
  ELM_PORT.println("AT+PAIR=AB90,78,563412,20");
  Serial.println("AT+PAIR=AB90,78,563412,20");
  delay(5000);
  ELM_PORT.println("AT+LINK=AB90,78,563412");
  Serial.println("AT+LINK=AB90,78,563412");
  delay(5000);
  while (1)
  {
	  while (Serial.available()) ELM_PORT.write(Serial.read());
	  while (ELM_PORT.available()) Serial.write(ELM_PORT.read());
  }
#else
  delay(100);
  u8g2.begin();
  u8g2.setFont(u8g2_font_helvB08_tf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);
#endif
#ifdef SIMULATION
  appCurrState = STATE_RUNNING;
#else
  appCurrState = STATE_BOOTING;
#endif
}


#ifndef SERIAL_LOOPBACK
void loop()
{
	if (appCurrState == STATE_BOOTING)
	{
#ifdef DEBUG_MODE
		u8g2.firstPage();
		do
		{
			u8g2.drawStr(8, 0, "BOOTING");
		} while (u8g2.nextPage());
#else
		u8g2.firstPage();
		do
		{
			u8g2.drawXBMP(0, 0, 128, 64, kymco_logo);
		} while (u8g2.nextPage());
#endif
		//reset HC-05
		digitalWrite(HC05_RESET_PIN, LOW);
		delay(1000);
		appCurrState = STATE_CONNECTING;
	}

	if (appCurrState == STATE_CONNECTING)
	{
#ifdef DEBUG_MODE
		u8g2.firstPage();
		do
		{
		u8g2.drawStr(8, 0,  "CONNECTING");
		} while (u8g2.nextPage());
#endif
		//boot HC-05
		digitalWrite(HC05_RESET_PIN, HIGH);
		delay(3000);

		//initiate communication with ELM327 module
		if (myELM327.begin(ELM_PORT, false, 5000, ISO_15765_11_BIT_500_KBAUD))
			appCurrState = STATE_CONNECTED;
		else 
			appCurrState = STATE_ELM_ERROR;
	}
		
	if (appCurrState == STATE_CONNECTED)
	{
#ifdef DEBUG_MODE
		u8g2.firstPage();do
		{
		u8g2.drawStr(8, 0, "CONNECTED");
		} while (u8g2.nextPage());
#endif
		delay(1000);
		appCurrState = STATE_TEST_ELM_PROTO;
	}

	if (appCurrState == STATE_TEST_ELM_PROTO)
	{
#ifdef DEBUG_MODE
		u8g2.firstPage(); 
		do
		{
		u8g2.drawStr(8, 0, "TEST_ELM_PROTO");
		} while (u8g2.nextPage());
#endif
		delay(1000);
		if (myELM327.connected)
		{
			appCurrState = STATE_RUNNING;
#ifdef DEBUG_MODE
			u8g2.firstPage();
			do
			{
				u8g2.drawStr(8, 0, "RUNNING");
			} while (u8g2.nextPage());
#endif
		}
		else appCurrState = STATE_ELM_ERROR;
	}
	
	if (appCurrState == STATE_RUNNING)
	{	

#ifdef SIMULATION
		RPM = 4000 + random(-1000, 4000);// myELM327.rpm();-
		ECT = 100 + random(-5, 2);// myELM327.engineCoolantTemp();
		VBAT = 12.7 + random(-2, 2) / 10.2;// myELM327.ctrlModVoltage
		INTAKETEMP = 27.1 + random(-2, 2) / 10.2;
		ENGINELOAD = 50 + random(-10, 10);
#else
		RPM = abs(myELM327.rpm());
		if (myELM327.status == ELM_SUCCESS)
		{
			ECT = myELM327.engineCoolantTemp();
			INTAKETEMP = myELM327.intakeAirTemp();
			ENGINELOAD = mapLimit(abs(myELM327.engineLoad()),11,82,0,100);
		}
		VBAT = myELM327.ELMVoltage();
#endif

#ifdef SIMULATION
		if (1)
#else
		if (myELM327.status == ELM_SUCCESS || myELM327.status == ELM_NO_DATA)
#endif	
		{			
			u8g2.firstPage();
			do
			{			   
			sprintf(tempstr, "%03d",int(ECT));
			u8g2.setFont(u8g2_font_helvB24_tn);
			u8g2.drawStr(3, 1, tempstr);

			u8g2.setFont(u8g2_font_helvB08_tf);
			u8g2.drawStr(55, 0, "\260C");
			
			u8g2.setFont(u8g2_font_helvB14_tf);
			dtostrf(INTAKETEMP, 2, 1, tempstr);
			u8g2.drawStr(80-5, -1, tempstr);
			u8g2.setFont(u8g2_font_helvB08_tf);
			u8g2.drawStr(116-5, 2, "\260C");
			
			dtostrf(VBAT, 2, 1, tempstr);
			u8g2.setFont(u8g2_font_helvB14_tf);
			u8g2.drawStr(80-5, 16-2, tempstr);
			u8g2.setFont(u8g2_font_helvB08_tf);
			u8g2.drawStr(118-3, 20-2, "V");
			
			long barWidth = map(RPM, 0, 8000, 0, 128);
			u8g2.drawBox(2, 32-1, barWidth, 16);
			u8g2.setFontMode(1);
			u8g2.setDrawColor(2);
			sprintf(tempstr, "%04d", int(RPM));
			u8g2.setFont(u8g2_font_helvB14_tf);
			u8g2.drawStr(5, 32-1, tempstr);
			u8g2.setFont(u8g2_font_helvB08_tf);
			u8g2.drawStr(50, 32+2, "RPM");
			u8g2.setDrawColor(1);

			barWidth = map(ENGINELOAD, 0, 100, 0, 128);
			u8g2.drawBox(2, 48, barWidth, 16);
			u8g2.setFontMode(1);
			u8g2.setDrawColor(2);
			sprintf(tempstr, "%03d", int(ENGINELOAD));
			u8g2.setFont(u8g2_font_helvB14_tf);
			u8g2.drawStr(5, 48, tempstr);
			u8g2.setFont(u8g2_font_helvB08_tf);
			u8g2.drawStr(50, 48+3, "%");
			u8g2.setDrawColor(1);
				
			} while (u8g2.nextPage());

			appCurrState = STATE_RUNNING;
		}
		else
		{
			appCurrState = STATE_ELM_ERROR;
		}
	}

	if (appCurrState == STATE_ELM_ERROR)
	{
#ifdef DEBUG_MODE
		u8g2.firstPage();
		do
		{
			u8g2.drawStr(8, 0, "ERROR");
			if (myELM327.status == ELM_SUCCESS)	u8g2.drawStr(8, 16, "ELM_SUCCESS");
			if (myELM327.status == ELM_NO_RESPONSE)	u8g2.drawStr(8, 16, "ELM_NO_RESP");
			if (myELM327.status == ELM_BUFFER_OVERFLOW) u8g2.drawStr(8, 16, "ELM_BUF_OVF");
			if (myELM327.status == ELM_UNABLE_TO_CONNECT) u8g2.drawStr(8, 16, "ELM_UNAB_TO_CON");
			if (myELM327.status == ELM_NO_DATA)	u8g2.drawStr(8, 16, "ELM_NO_DATA");
			if (myELM327.status == ELM_STOPPED) u8g2.drawStr(8, 16, "ELM_STOPPED");
			if (myELM327.status == ELM_TIMEOUT)	u8g2.drawStr(8, 16, "ELM_TIMEOUT");
			if (myELM327.status == ELM_TIMEOUT) u8g2.drawStr(8, 16, "ELM_GEN_ERR");
	} while (u8g2.nextPage());
#else
		u8g2.firstPage();
		do
		{
			u8g2.drawStr(32, 24, "Errore");
		} while (u8g2.nextPage());
#endif
			delay(3000);
			appCurrState = STATE_BOOTING;
	}
}
#else
void loop() {}
#endif
