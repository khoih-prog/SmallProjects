# Small Projects to demonstrate the usage of ISR-based timers for ESP8266, ESP32 and Arduino Mega, Nano, etc.

## ISR-based Fire Smoke Alarm demonstrate how to use ESP8266TimerInterrupt, ESP32TimerInterrupt and TimerInterrupt Library

These are examples how to use, design and convert the code from normal `software timer` to `ISR-based timer`.

**Why do we need this `Hardware Timer Interrupt`?**

Imagine you have a system with a `mission-critical` function, measuring water level and control the sump pump or doing something much more important using extensive GUI, such as medical equipments, security and/or fire-smoke alarm, etc. You normally use a `software timer to poll`, or even place the function in loop(). But what if another function is blocking the `loop()` or `setup()`.

So your function might not be executed, and the result would be disastrous.

You'd prefer to have your function called, no matter what happening with other functions (busy loop, bug, etc.).

The correct choice is to use a `Hardware Timer with Interrupt` in cooperation with an `Input Pin Interrupt` to call your function.

These hardware timers, using interrupt, still work even if other functions are blocking. Moreover, they are much more precise (certainly depending on clock frequency accuracy) than other software timers using `millis()` or `micros()`. That's necessary if you need to measure some data requiring better accuracy.

Functions using normal software timers, relying on loop() and calling millis(), won't work if the loop() or setup() is `blocked` by certain operation. For example, certain function is blocking while it's connecting to WiFi or some services.

The catch is your function is now part of an `ISR (Interrupt Service Routine)`, and must be `lean / mean`, and follow certain rules. More to read on:

[Attach Interrupt](https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/)

**Important Notes:**
1. Inside the attached function, delay() won’t work and the value returned by millis() will not increment. Serial data received while in the function may be lost. You should declare as volatile any variables that you modify within the attached function.

2. Typically global variables are used to pass data between an ISR and the main program. To make sure variables shared between an ISR and the main program are updated correctly, declare them as volatile.

## Design principles of ISR-based Fire and Smoke Alarm

The design principles are as follows:

1. Fire or Smoke measurement and alarm activation are considered `mission-critical`, and must not be interfered or blocked by any other bad tasks, `intentionally or unintentionally`. This principle can be applied to any of your projects. Please check the way ISR-based are designed ( very lean and mean ), no delay() and no unnecessary baggage.
2. The sound alarm is also considered critical. Alarm without sound has no meaning in life-threatening dangerous situation.
3. Blynk is considered just a `Graphical-User-Interface (GUI)`. Being connected or not must not interfere with the alarm detection / warning.

Certainly, with Blynk GUI, we can achieve many more great features, such as `remote check and control, configurable test case and value` , etc. when possible.

This can be applied in many projects requiring reliable system control, where `good, bad, or no connection has no effect on the operation of the system`.

## Contributing
If you want to contribute to this project:
- Report bugs and errors
- Ask for enhancements
- Create issues and pull requests
- Tell other people about this library

## Copyright
Copyright 2019- Khoi Hoang
