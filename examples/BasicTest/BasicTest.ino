/*
	Test that supplied funtion runs once per second
*/
#include <MNTimerLib.h>

unsigned int uiCount = 0;
unsigned long ulLastTime;
unsigned long ulStartTime;

#define BAUD_RATE 115200 

void setup() 
{

	Serial.begin (BAUD_RATE);
	while ( !Serial );

	TheTimer.AddCallBack ( MyTimerCode, 2000 );
  	
	ulStartTime = millis();
	ulLastTime = ulStartTime;
}

void loop() 
{
  if ( millis() - ulLastTime > 1000 )
  {
	  ulLastTime = millis();
	  Serial.print ( "Timer code has run : " );
	  Serial.print ( uiCount );
	  Serial.print ( " times in " );
	  Serial.print (  ( ulLastTime - ulStartTime ) / 1000.0 );
	  Serial.println ( " seconds" );
  }
}

void MyTimerCode ()
{
	uiCount++;
}
