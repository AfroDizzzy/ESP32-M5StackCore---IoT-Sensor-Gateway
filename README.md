# ESP32-M5StackCore---IoT-Sensor-Gateway

This project created an IoT sensor network consistening of two sensor nodes and one central gateway. All devices used the Bluetooth Low Energy protocol with approciate service and characteristic UUID's. 

Retieving the node values from the gateway was done via CoAP. 

Battery saving functionality included duty cycling via ESP deep sleep at 10 second intervals. Further power saving can be done (as well as speed increases) by switching off the LCD screen of the M5Stack and all related calls to write text on the screen. 

The following repositories and codebases were used: 
1. https://github.com/naoki-sawada/m5stack-ble
2. https://github.com/nkolban/ESP32_BLE_Arduino for client device
3. https://github.com/hirotakaster/CoAP-simple-library
4. https://github.com/avency/coap-cli
5. https://community.m5stack.com/topic/2505/light-deep-sleep-example/8

The IDE used was the Ardiuno IDE, the appropriate libraries can be found in the above links. 
