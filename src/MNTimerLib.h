// MNTimerLib.h
//
// This defines a timer class that encapsulates a microsecond timer that has functions to call at given intervals
//
// (c) Mark Naylor November 2022
//
#pragma once

#include <Arduino.h>
#include "MNTimerLib.h"

#define MAX_CALLBACKS	8
#define RESOLUTION		2000		// ticks per sec

extern class MNTimerClass TheTimer;

typedef void ( *TimerCallback )();
typedef void ( MNTimerClass::*aMemberFunction)(void);

class MNTimerClass
{
	typedef struct CallbackData 
	{
		TimerCallback	aFunction;			// non member function to be called to process action
		aMemberFunction	aMFunction;
		MNTimerClass*	pClass;
		uint32_t		Ticks;				// number of timer ticks after which aFunction called
		uint32_t		TimerOffset;		// timer count value when callback added
	} CALLBACKDATA, *PCALLBACKDATA;	
public:
					MNTimerClass ( void );
	bool			AddCallBack ( TimerCallback Routine, uint32_t uiInterval );
	bool			AddCallBack ( MNTimerClass * pClass, aMemberFunction, uint32_t uiInterval );
	bool			RemoveCallBack ( TimerCallback Routine );
	bool			RemoveCallBack ( MNTimerClass * pClass, aMemberFunction );
	uint32_t		GetInterval ( uint8_t uiIndex );
	TimerCallback 	GetCallback ( uint8_t uiIndex );
	void			ClearAllCallBacks ( void );
	uint8_t			GetNumCallbacks ( void );
	void			CheckReady ( uint8_t iCallbackNum );
	void 			IncTickCount();	
protected:
	CALLBACKDATA		m_Data [ MAX_CALLBACKS ];
	volatile uint8_t	m_uiCallbackCount;
	uint32_t			m_ulTimerCount;
};