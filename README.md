# arduino-timer-lib

A simple Timer library for Arduino that allows executing a functor (funtion or member) at a specified delay and/or period.
See the following methods in **Timer** class:
- **schedOneTime** for an unique execution given a delay.
- **schedRepeat** for a repeated execution given a period and an optional delay.

## License
Distributed under BOOST license. See *LICENSE_1_0.txt*.

## Installation notes
Copy this folder in *libraries* folder of your sketchbooks.

## Dependencies
- **arduino-srutil**
- **arduino-util-lib**

## Usage
Include *TimerLib.h* in your sketch (file .ino) so Arduino add this library at compilation. Additionally, include dependencies *SRUtilLib.h* and *UtilLib.h*.

Add *SoftwareTimer.h* to use the software timer in your application.

## Version History
- 1.0 Initial version (15 March 2013).
	Complete software implementation for Arduino.

## TODO
- Create a **HardwareTimer** that uses hardware timers. (In progress)
- Allow removing scheduled executions.

