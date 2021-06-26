#include <M5Stack.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

//Humidity service and characteristics variables.
#define SERVICE_UUID "58453eb4-b9d7-11eb-8529-0242ac130003"
#define CHARACTERISTIC_UUID "2a6f"

//variables used for duty cycling
#define microSecondToSecond 1000000
#define SleepDuration 5
#define powerOnTime 10000 //this is in milliseconds

//device associated with temeperature
BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;

//variable changed when device is connected or not
bool deviceConnected = false;

//callback called when this device is connected or disconnected to the gateway.
class MyServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
        M5.Lcd.println("connected to gateway! :D ");
        deviceConnected = true;
    };

    void onDisconnect(BLEServer *pServer)
    {
        M5.Lcd.println("disconnected from gateway -_-");
        deviceConnected = false;
    }
};

//callback telling the device a characteristic has been accessed (read or write) by another BLE device

class MyCallbacks : public BLECharacteristicCallbacks
{
    void onRead(BLECharacteristic *pCharacteristic)
    {
        M5.Lcd.println("Humidity value accessed:");
    }

    void onWrite(BLECharacteristic *pCharacteristic)
    {
    }
};

//function run everytime the device is started
void setup()
{
    Serial.begin(115200);
    M5.begin();
    M5.setWakeupButton(BUTTON_A_PIN);
    M5.Lcd.println("BLE Humidity Node Started");
    m5.Speaker.mute();

    // Initialize BLE on this device
    BLEDevice::init("m5-Hum-Node");

    //makes this device a BLE server
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    //create the ble service on this device
    BLEService *pService = pServer->createService(SERVICE_UUID);

    //create the characteristic on this device
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_NOTIFY |
            BLECharacteristic::PROPERTY_INDICATE);
    pCharacteristic->setCallbacks(new MyCallbacks());
    pCharacteristic->addDescriptor(new BLE2902());

    // starts the service
    pService->start();

    //starts intialises and starts the sending of BLE advertising packets
    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->start();

    //   //sets the initial value of the humidity. Makes sure that 0 is not sent. In reality this value would be constantly updated by a sensor.
    pCharacteristic->setValue("30");

    //gives random number from 0 to 30 as temperature
    int hum = random(0, 100);
    char buf[5];
    itoa(hum, buf, 10);
    M5.Lcd.printf("The Humidity has been set to %s \n", buf);

    //set the characteristic value with the temperature
    pCharacteristic->setValue(buf);
    pCharacteristic->notify();

    //function which keeps the power on
    delay(powerOnTime);
    //functions which put device to sleep
    esp_sleep_enable_timer_wakeup(microSecondToSecond * SleepDuration);
    esp_deep_sleep_start();
}

void loop()
{
}
