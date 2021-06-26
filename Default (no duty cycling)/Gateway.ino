
#include <M5Stack.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <coap-simple.h>
#include "time.h"

//scanning and connection status variables
static boolean doScan = false;
static boolean tempConnected = false;
static boolean humidityConnected = false;
static boolean TryTempConnecting = false;
static boolean TryHumidityConnecting = false;

//Temperature service and characteristics variables
static BLEUUID serviceUUIDTemperature("1bcf5e80-1aed-4167-b900-b406460009eb");
static BLEUUID charUUIDTemperature("2a1c");
//device associated with temeperature
static BLERemoteCharacteristic *pRemoteCharacteristicTemperature;
static BLEAdvertisedDevice *myDeviceTemperature;
std::string tem = charUUIDTemperature.toString().c_str();

//Humidity service and characteristics variables.
static BLEUUID serviceUUIDHumidity("58453eb4-b9d7-11eb-8529-0242ac130003");
static BLEUUID charUUIDHumidity("2a6f");
//device associated with humidity
static BLERemoteCharacteristic *pRemoteCharacteristicHumidity;
static BLEAdvertisedDevice *myDeviceHumidity;
std::string hum = charUUIDHumidity.toString().c_str();

//variables related to the sensor characteristics
std::string tempValue;
std::string humidityValue;
std::string CoAPoutputTemp;
std::string CoAPoutputHumidity;
std::string degC = "°C";
std::string humidityP = "%";
std::string space = " ";

//wifi network details you want to connect to
const char *WIFI_SSID = "davidrhode";
const char *WIFI_PASSWORD = "rocksrule";
// const char *WIFI_SSID = "Wakachangi-EXT";
// const char *WIFI_PASSWORD = "Greatunclekenny";

//time sync related variables used once wifi is connected
const char *ntpServer1 = "oceania.pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";
const long gmtOffset_sec = (13) * 60 * 60;
const int daylightOffset_sec = 3600;

//Unix timestamp. Once gateway is time synced, you can use this timestamp
RTC_DATA_ATTR time_t timestamp = 0;

//BLE scanner object
BLEScan *scanner;

// IP address, UDP and Coap instances
IPAddress wifi_ip;
WiFiUDP udp;
Coap coap(udp);

//function that is called everytime a CoAP request is received for the temperature value.
void callback_temp(CoapPacket &packet, IPAddress ip, int port)
{
    M5.Lcd.println("CoAP temperature query received");

    CoAPoutputTemp = String(timestamp).c_str() + space + tempValue.c_str() + degC;
    const char *payload = CoAPoutputTemp.c_str();
    int ret = coap.sendResponse(ip, port, packet.messageid, payload, strlen(payload),
                                COAP_CONTENT, COAP_TEXT_PLAIN, packet.token, packet.tokenlen);

    if (ret)
    {
        M5.Lcd.println("Sent TEMP via COAP");
    }
    else
    {
        M5.Lcd.println("COAP TEMP failed");
    }
}

//function that is called everytime a CoAP request is received for the humidity value.
void callback_humidity(CoapPacket &packet, IPAddress ip, int port)
{
    M5.Lcd.println("CoAP humidity query received");
    
    CoAPoutputHumidity = String(timestamp).c_str() + space + humidityValue.c_str() + humidityP;
    const char *payload = CoAPoutputHumidity.c_str();
    int ret = coap.sendResponse(ip, port, packet.messageid, payload, strlen(payload),
                                COAP_CONTENT, COAP_TEXT_PLAIN, packet.token, packet.tokenlen);

    if (ret)
    {
        M5.Lcd.println("Sent HUMIDITY via COAP");
    }
    else
    {
        M5.Lcd.println("COAP HUMIDITY failed");
    }
}

//runs everytime a notification is received from a server node
void notifyCallback(
    BLERemoteCharacteristic *pBLERemoteCharacteristic,
    uint8_t *pData,
    size_t length,
    bool isNotify)
{
    std::string rem = pBLERemoteCharacteristic->getUUID().toString().c_str();

    //converts the characteristic data into a string (this took me 1 day to find and implement...sigh)
    int val = 0;
    val = (uint16_t)(pData[1] << 8 | pData[0]);
    //checks which notification is received and outputs the according string and value
    if (rem == hum)
    {
        M5.Lcd.print("New Humidity Value Received of ");
        M5.Lcd.println(val);
        tempValue = val;
    }
    else
    {
        M5.Lcd.print("New Temperature Value Received of ");
        M5.Lcd.println(val);
        humidityValue = val;
    }
}

//runs everytime a node is connected and disconnected. Main purpose in this usecase is to tell this client to start searching again once a device disconnects
class MyClientCallback : public BLEClientCallbacks
{
    void onConnect(BLEClient *pclient)
    {
    }
    void onDisconnect(BLEClient *pclient)
    {
        BLERemoteService *checkTemp = pclient->getService(serviceUUIDTemperature);
        BLERemoteService *checkHumidity = pclient->getService(serviceUUIDHumidity);
        if (checkTemp == nullptr)
        {
            M5.Lcd.println("Disconnected: Temp Node");
            tempConnected = false;
            doScan = true;
        }
        else
        {
            M5.Lcd.println("Disconnected: Humidity Node");
            humidityConnected = false;
            doScan = true;
        }
    }
};

//function used to connect to the temperature device and setups of the gateway to receive notifications from the device
bool connectToTemp()
{
    BLEClient *pClientTemp = BLEDevice::createClient();

    // Connect to the BLE Server.
    pClientTemp->setClientCallbacks(new MyClientCallback());
    pClientTemp->connect(myDeviceTemperature);

    // Obtain a reference to the service we are after in the remote BLE server,  if it doesnt have it then disconnect from device
    BLERemoteService *pRemoteServiceTemp = pClientTemp->getService(serviceUUIDTemperature);
    if (pRemoteServiceTemp == nullptr)
    {
        pClientTemp->disconnect();
        return false;
    }

    // Obtain a reference to the characteristic in the service of the remote BLE server, if it doesnt have it then disconnect from device
    pRemoteCharacteristicTemperature = pRemoteServiceTemp->getCharacteristic(charUUIDTemperature);
    if (pRemoteCharacteristicTemperature == nullptr)
    {
        pClientTemp->disconnect();
        return false;
    }

    // Read the value and display characteristic value
    if (pRemoteCharacteristicTemperature->canRead())
    {
        tempValue = pRemoteServiceTemp->getValue(charUUIDTemperature);
        M5.Lcd.print("New Temperature Value Received of ");
        M5.Lcd.println(tempValue.c_str());
    }
    //allows for client to detect changes on node/server
    if (pRemoteCharacteristicTemperature->canNotify())
    {
        pRemoteCharacteristicTemperature->registerForNotify(notifyCallback);
        tempConnected = true;
        TryTempConnecting = false;
    }
}

//function used to connect to the humidity device and setups of the gateway to receive notifications from the device
bool connectToHumidity()
{

    BLEClient *pClientHumidity = BLEDevice::createClient();

    // Connect to the BLE Server.
    pClientHumidity->setClientCallbacks(new MyClientCallback());
    pClientHumidity->connect(myDeviceHumidity);

    // Obtain a reference to the service we are after in the remote BLE server,  if it doesnt have it then disconnect from device
    BLERemoteService *pRemoteServiceHumidity = pClientHumidity->getService(serviceUUIDHumidity);
    if (pRemoteServiceHumidity == nullptr)
    {
        pClientHumidity->disconnect();
        return false;
    }

    // Obtain a reference to the characteristic in the service of the remote BLE server,  if it doesnt have it then disconnect from device
    pRemoteCharacteristicHumidity = pRemoteServiceHumidity->getCharacteristic(charUUIDHumidity);
    if (pRemoteCharacteristicHumidity == nullptr)
    {
        pClientHumidity->disconnect();
        return false;
    }

    // Read the value and display characteristic value
    if (pRemoteCharacteristicHumidity->canRead())
    {
        humidityValue = pRemoteServiceHumidity->getValue(charUUIDHumidity);
        M5.Lcd.print("New Humidity Value Received of ");
        M5.Lcd.println(humidityValue.c_str());
    }

    //allows for client to detect changes on node/server
    if (pRemoteCharacteristicHumidity->canNotify())
    {
        pRemoteCharacteristicHumidity->registerForNotify(notifyCallback);
        humidityConnected = true;
        TryHumidityConnecting = false;
    }
}

//ran every time a BLE device is detected
class MyCallbacks : public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
        //initialises the temp node on the client
        if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUIDTemperature))
        {
            M5.Lcd.println("Temp node found!");
            myDeviceTemperature = new BLEAdvertisedDevice(advertisedDevice);
            TryTempConnecting = true;
            doScan = true;
        }

        //initialises the humidity node on the client
        if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUIDHumidity))
        {
            M5.Lcd.println("Humidity node found!");
            myDeviceHumidity = new BLEAdvertisedDevice(advertisedDevice);
            TryHumidityConnecting = true;
            doScan = true;
        }

        //stops the scan if both devices are found
        if (TryTempConnecting && TryHumidityConnecting)
        {
            BLEDevice::getScan()->stop();
            doScan = false;
        }
    }
};

//this is ran only when the device is started up for the first time.
void setup()
{

    // Initialize the M5Stack device
    M5.begin();
    M5.Power.begin();
    M5.Lcd.clear();
    M5.Lcd.setBrightness(100);
    M5.Lcd.println("Gateway node starting...");

    // Initialize WIFI. Will not process past this unless the wifi is connected.
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
        delay(1000);
    M5.Lcd.print("Connected to WIFI with IP address ");
    wifi_ip = WiFi.localIP();
    M5.Lcd.println(wifi_ip);

    // Do time sync. Make sure gateway can connect to
    // Internet (i.e., your WiFi AP must be connected to Internet)
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
    delay(2000);
    time(&timestamp);
    M5.Lcd.println("Current time is " + String(timestamp));

    // Initialize BLE on device
    BLEDevice::init("Gateway");

    // Creates BLE scanner and sets scanner parameters
    M5.Lcd.println("scanning for BLE devices...");
    BLEScan *scanner = BLEDevice::getScan();
    scanner->setAdvertisedDeviceCallbacks(new MyCallbacks());
    scanner->setInterval(1349);
    scanner->setWindow(449);
    scanner->setActiveScan(true);
    scanner->start(5, false);

    // Initialize CoAP and sets callbacks and the corresponding phrases that trigger them
    coap.server(callback_temp, "temperature");
    coap.server(callback_humidity, "humidity");

    // start coap server/client
    coap.start();
}

//sets of operations that run per delay cycle
void loop()
{
    //makes the previous lcd print statements black (does not remove them surprisingly)
    if (M5.BtnC.wasPressed())
    {
        M5.Lcd.clear(BLACK);
    }

    //connects the device to the temperature node if it is found
    if (TryTempConnecting == true)
    {
        if (connectToTemp())
        {
            M5.Lcd.println("Connected to Temperature Node");
            TryTempConnecting = false;
        }
    }

    //connects the device to the humidity node if it is found
    if (TryHumidityConnecting == true)
    {
        if (connectToHumidity())
        {
            M5.Lcd.println("Connected to Humidity Node");
            TryHumidityConnecting = false;
        }
    }

    //stops scanning for nearby devices if both are found and connnected. Will continue to scan if both devices arent connected.
    if (tempConnected && humidityConnected)
    {
        doScan = false;
    }
    else if (doScan)
    {
        BLEDevice::getScan()->start(5, false);
    }

    //delay(1000); use this to set the delay for when this loop is run again

    M5.update();
    coap.loop();
}
