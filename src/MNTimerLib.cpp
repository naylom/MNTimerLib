// 
// 
// This implements a timer class that encapsulates a microsecond timer that has functions to call at given intervals. Its set to a resolution of 2000 ticks per sec
//
// (c) Mark Naylor June 2021
// 
#include "MNTimerLib.h"
#define CALL_MEMBER_FN(object,ptrToMember)  ((object).*(ptrToMember))

#ifdef ARDUINO_ARCH_AVR					// tested with Uno
MNTimerClass::MNTimerClass ( void )
{
	m_uiCallbackCount = 0;
	m_ulTimerCount = 0UL;
	/*
	*	This code is designed for Arduino Uno only!!!!
	*
	*/
	noInterrupts ();

	//set timer2 interrupt at 4kHz
	TCCR2A = 0;				// set entire TCCR2A register to 0
	TCCR2B = 0;				// same for TCCR2B
	TCNT2 = 0;				// initialize counter value to 0

	// set compare match register for 4khz increments
	OCR2A = 113;				// = (16*10^6) / (4000*64) - 1 (must be <256)

	// turn on CTC mode
	TCCR2A |= ( 1 << WGM21 );
	// Set CS21 bit & CS20 bit for 64 prescaler
	//TCCR2B |= ( 1 << CS21 );
	// TCCR2B |= ( 1 << CS20 );
	// enable timer compare interrupt
	TIMSK2 |= ( 1 << OCIE2A );

	interrupts ();
}
#endif
#ifdef ARDUINO_ARCH_SAMD									// tested with MKR WiFi 1010
MNTimerClass::MNTimerClass ( void )
{
	m_uiCallbackCount = 0;
	m_ulTimerCount = 0UL;
	PORT->Group [ PORTA ].DIRSET.reg = PORT_PA21;			// Set D7 as a digital output

  	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN |                // Enable GCLK0 for TC4 and TC5
                      	GCLK_CLKCTRL_GEN_GCLK0 |            // Select GCLK0 at 48MHz
                      	GCLK_CLKCTRL_ID_TC4_TC5;            // Feed GCLK0 output to TC4 and TC5
  	while (GCLK->STATUS.bit.SYNCBUSY);                      // Wait for synchronization
 
  	TC4->COUNT16.CC [ 0 ].reg = 2999;                      // Set the TC4 CC0 register as the TOP value in match frequency mode
  	while ( TC4->COUNT16.STATUS.bit.SYNCBUSY );             // Wait for synchronization

  	NVIC_SetPriority(TC4_IRQn, 0);    // Set the Nested Vector Interrupt Controller (NVIC) priority for TC4 to 0 (highest)
  	NVIC_EnableIRQ(TC4_IRQn);         // Connect TC4 to Nested Vector Interrupt Controller (NVIC)

  	TC4->COUNT16.INTENSET.reg = TC_INTENSET_OVF;            // Enable TC4 overflow (OVF) interrupts
 
  	TC4->COUNT16.CTRLA.reg |= TC_CTRLA_PRESCSYNC_PRESC |    // Reset timer on the next prescaler clock
                              TC_CTRLA_PRESCALER_DIV8 |     // Set prescaler to 8, 48MHz/8 = 6MHz
                              TC_CTRLA_WAVEGEN_MFRQ |       // Put the timer TC4 into match frequency (MFRQ) mode 
                              TC_CTRLA_MODE_COUNT16;        // Set the timer to 16-bit mode      
  	while (TC4->COUNT16.STATUS.bit.SYNCBUSY);               // Wait for synchronization

  	TC4->COUNT16.CTRLA.bit.ENABLE = 1;                      // Enable the TC4 timer
  	while ( TC4->COUNT16.STATUS.bit.SYNCBUSY );             // Wait for synchronization	
}
#endif

/// <summary>
/// Called by timer ISR to increement count of ticks
/// </summary>
/// <returns>Nothing</returns>
void MNTimerClass::IncTickCount ()
{
	m_ulTimerCount++;
}
/// <summary>
/// Called by timer ISR to invoke callback if ready
/// <param name="iCallbackNum">number of callback to be checked</param>
/// </summary>
/// <returns>Nothing</returns>
void MNTimerClass::CheckReady ( uint8_t iCallbackNum )
{
	if ( iCallbackNum < m_uiCallbackCount )
	{
		if ( ( m_ulTimerCount - m_Data [ iCallbackNum ].TimerOffset ) % m_Data [ iCallbackNum ].Ticks == 0 )
		{
			if ( m_Data [ iCallbackNum ].aMFunction == nullptr )
			{
			 	m_Data [ iCallbackNum ].aFunction();
			}
			else
			{
				CALL_MEMBER_FN ( *m_Data [ iCallbackNum ].pClass, m_Data [ iCallbackNum ].aMFunction)();
			}
		}
	}
}
/// <summary>
/// adds a callback routine to be called at specified interval
/// </summary>
/// <param name="Routine">address of callback routine with signature of void func ( void )</param>
/// <param name="ulInterval">number of 1/2000 sec ticks after which callback should be invoked</param>
/// <returns>true if max number of callbacks not exceeded and this callback is not alreasy registered. else false</returns>
bool MNTimerClass::AddCallBack ( TimerCallback Routine, uint32_t ulInterval )
{
	bool bResult = false;

	if ( m_uiCallbackCount < MAX_CALLBACKS )
	{
		// check callback not already registered
		for ( uint8_t i = 0; i < m_uiCallbackCount; i++ )
		{
			// look for match
			if ( m_Data [ i ].aFunction == Routine )
			{
				// already here, so quit
				return bResult;
			}
		}
		m_Data [ m_uiCallbackCount ].pClass = nullptr;
		m_Data [ m_uiCallbackCount ].aMFunction = nullptr;
		m_Data [ m_uiCallbackCount ].aFunction = Routine;
		m_Data [ m_uiCallbackCount ].Ticks = ulInterval;
		m_Data [ m_uiCallbackCount ].TimerOffset = 0UL;
		m_uiCallbackCount++;

		bResult = true;
	}
	return bResult;
}
/// <summary>
/// adds a callback routine to be called at specified interval
/// </summary>
/// <param name="pClass">address class instance/param>
/// <param name="aMFunction">address of callback member function/param>
/// <param name="ulInterval">number of 1/2000 sec ticks after which callback should be invoked</param>
/// <returns>true if max number of callbacks not exceeded and this callback is not alreasy registered. else false</returns>
bool MNTimerClass::AddCallBack ( MNTimerClass * pClass, aMemberFunction aMFunction, uint32_t ulInterval )
{
	bool bResult = false;
	noInterrupts();
	if ( m_uiCallbackCount < MAX_CALLBACKS )
	{
		// check callback not already registered

		for ( uint8_t i = 0; i < m_uiCallbackCount; i++ )
		{
			// look for match
			if ( m_Data [ i ].aMFunction == aMFunction && m_Data [ m_uiCallbackCount ].pClass == pClass )
			{
				// already here, so quit
				interrupts();
				return bResult;
			}
		}
		
		m_Data [ m_uiCallbackCount ].pClass = pClass;
		m_Data [ m_uiCallbackCount ].aMFunction = aMFunction;
		m_Data [ m_uiCallbackCount ].aFunction = nullptr;
		m_Data [ m_uiCallbackCount ].Ticks = ulInterval;
		m_Data [ m_uiCallbackCount ].TimerOffset = 0UL;
		m_uiCallbackCount++;

		bResult = true;
	}
	interrupts();	
	return bResult;
}

/// <summary>
/// Removes specified callback from configured list
/// </summary>
/// <param name="Routine">address of callback routine with signature of void func ( void )</param>
/// <returns>true if callback removed successfully, else false</returns>
bool MNTimerClass::RemoveCallBack ( TimerCallback Routine )
{
	bool bResult = false;
	noInterrupts ();
	for ( uint8_t i = 0 ; i < m_uiCallbackCount; i++ )
	{
		// look for match
		if ( m_Data [ i ].aFunction == Routine )
		{
			// match found
			// overwrite with last entry
			
			m_Data [ i ] = m_Data [ m_uiCallbackCount - 1];
			m_uiCallbackCount--;
			bResult = true;
			break;
		}
	}
	interrupts ();	
	return bResult;
}
/// <summary>
/// Removes specified callback from configured list
/// </summary>
/// <param name="pClass">address class instance/param>
/// <param name="aMFunction">address of member function</param>
/// <returns>true if callback removed successfully, else false</returns>
bool MNTimerClass::RemoveCallBack ( MNTimerClass * pClass, aMemberFunction aMFunction )
{
	bool bResult = false;
	noInterrupts ();	
	for ( uint8_t i = 0 ; i < m_uiCallbackCount; i++ )
	{
		// look for match
		if ( m_Data [ i ].aMFunction == aMFunction && m_Data [ i ].pClass == pClass )
		{
			// match found
			// overwrite with last entry

			m_Data [ i ] = m_Data [ m_uiCallbackCount - 1];
			m_Data [ m_uiCallbackCount - 1 ].pClass = 0;
			m_Data [ m_uiCallbackCount - 1 ].aFunction = 0;
			m_uiCallbackCount--;

			bResult = true;
			break;
		}
	}
	interrupts ();	
	return bResult;
}

/// <summary>
/// Gets the number of 1/2000 sec ticks required to pass before the callback whose index is provided is invoked
/// </summary>
/// <param name="uiIndex">zero based index of callback list entry required</param>
/// <returns>number of 1/4000 sec ticks</returns>
uint32_t MNTimerClass::GetInterval ( uint8_t uiIndex )
{
	return m_Data [ uiIndex % MAX_CALLBACKS  ].Ticks;
}

/// <summary>
/// Get the address of the callback whose index is provided
/// </summary>
/// <param name="uiIndex">zero based index of callback list entry required</param>
/// <returns>callback function address</returns>
TimerCallback	MNTimerClass::GetCallback ( uint8_t uiIndex )
{
	return m_Data [ uiIndex % MAX_CALLBACKS ].aFunction;
}

/// <summary>
/// Clears callback list
/// </summary>
/// <param name="">none</param>
void MNTimerClass::ClearAllCallBacks ( void )
{
	m_uiCallbackCount = 0;
}

/// <summary>
/// Gets the number of callbacks that have been configured
/// </summary>
/// <param name="">none</param>
/// <returns>number of callbacks in list</returns>
uint8_t MNTimerClass::GetNumCallbacks ( void )
{
	return m_uiCallbackCount;
}

// Interrupt routine called by system timer
// common interrupt code called by either architecture
void TimerInterruptCode ()
{
	uint8_t uiNumCallbacks = TheTimer.GetNumCallbacks ();
	
	TheTimer.IncTickCount();
	
	for ( uint8_t i = 0 ; i < uiNumCallbacks ; i++ )
	{
		TheTimer.CheckReady ( i );
	}

}
#ifdef ARDUINO_ARCH_AVR					// tested with Uno

/// <summary>
/// hardware Timer2 interrupt - called every 1/4000 second. Checks if any configured callback is due to be called and invokes if necessary
/// </summary>
/// <param name="">none</param>
ISR ( TIMER2_COMPA_vect )
{

	TimerInterruptCode();
	TCNT2 = 0;		// Shouldn't be necessary!
}
#endif
#ifdef  ARDUINO_ARCH_SAMD									// tested with MKR WiFi 1010
void TC4_Handler()                                         // Interrupt Service Routine (ISR) for timer TC4
{     
	TimerInterruptCode(); 
    PORT->Group[PORTA].OUTTGL.reg = PORT_PA21;             // Toggle the D7 output
    TC4->COUNT16.INTFLAG.reg = TC_INTFLAG_OVF;             // Clear the OVF interrupt flag
}
#endif

MNTimerClass TheTimer;

