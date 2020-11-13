# ECE-6110-Security-System

 This project creates a proximity-based security system that includes live temperature and
 proximity readings via an HTTP server, which can be accessed by connecting to
 the IP address of the STM32 board while on the same network as the board. The live
 readings work by auto-refreshing the control panel web page every 5 seconds. Auto-refreshes
 faster than this make changing the established proximity fence almost impossible. As a result,
 the live values experience a maximum 5-second view lag from the "real" world values.

 This project also includes an adjustable alarm that is triggered when the on-board
 proximity sensor detects any object that is within the established proximity fence.
 When triggered, the live control panel web page alerts any viewers that an alarm has been
 triggered. This alarm also blinks an on-board LED using timer 16 to trigger an interrupt,
 for additional visibility. Additionally, the only way to silence the alarm is to press the
 alarm reset button (blue button on-board). This button triggers an interrupt which resets the alarm.

 Lastly, the established proximity fence can be changed through the control panel by
 using a text input to enter the desired distance at which any object at or within that boundary
 will trigger the alarm.

 For accessing the control panel web page, supported web browsers are:

	Google Chrome - Version 86.0.4240.193 (Official Build) (64-bit)