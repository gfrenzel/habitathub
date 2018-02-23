# Habitat Hub Arduino ARV Code

This code is for the Arduino Yun AVR side that monitors the sensors and sends the data to the middle ware application on the Linux side.  Reason I choose to use the middle ware and not communicate directly to Amazon Web Services was to allow the unit to be fully functional if there was no access to the Internet.

The program does the following in the loop:

    Reads sensors
    Reads menu buttons
    Reads clean water button
        Send date and command to save configuration
    Reads clean cage button
        Send date and command to save configuration
    Rotates through sensor data on LCD every 15 seconds
    Access configuration menu via select button
        Change configuration values Up/Down buttons

The following libraries will be needed to build.
    BridgeClient
    OneWire
    DallasTemperature
    DHT
    Adafruit_RGBLCDShield

