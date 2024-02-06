# Arduino-Project
**Description:** Uses an Arduino Due and an ethernet shield to communicate with DUTs. Works as an instrument with a server that waits for client connections. 
Uses a 4-byte packet structure that is sent using a TCP connection.

## How to run python driver
Instanciate the Arduino class, passing in the IP Address and TCP port your Arduino is listening to.

Example: 
```python
self.driver = Arduino(IP_address = ip, TCP_port = port)
```
You can use this driver by simply calling your instantiated class and the method you want to use. To connect to the Arduino you can use the following.
```python
self.driver.connect()
self.driver.enable_debug(True)
self.I2C_Write(56, bytearray([3, 207]))
...
And so on.
```

## How to compile and upload the Arduino code 
I will be using VSCode for this example. The same procedure would be used if running the Arduino IDE.
Install the PlatformIO IDE extension in VSCode. 

Open the project from the Platform IO extension. Just need to select this folder you just cloned. If you have a separate Arduino or other device you can change it in the platform.ini file.
If you wish to change boards you can to go project tab in the Platform IO extension and select which board you use. Keep in mind this code was developed for the Due.

<img src="images\Platform_IO_Project_Settings.png" alt="Project Settings" height="400"/>

At the bottom of VSCode it should show PlatformIO functions. If you want to build and upload to your microcontroller you can just press upload or build then upload.

<img src="images\functions.png" alt="PlatformIO Settings"/>

If you have enabled debug through the python driver you can open the serial monitor shown obove (AND gate button (or electrical plug)). You may have to go into the settings if your microcontroller is connected on a different COM port that the one defaulted.

With the serial port open and also the command prompt for the python script, you can run a little test. If you want to go crazy you can run the python driver file itself (/driver/arduino_driver.py). It has a built in test program to ensure that all functions are working.

## Arduino Packet Structure