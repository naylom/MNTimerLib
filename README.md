# MNTimerLib

Arduino library to provide a timer service

Tested on MKr WiFI 1010 and Uno baords

Allows user to set upto 8 timers concurrently

Each timer has a callback routine and a count which sets how often the callback is invoked. The count is in increments 1/2000 of a second 
i.e. if set to 2000 it will callback once a second.

The user is responsible to ensuring the callback is short and conforms with the restrictions of code run on an interrupt.

