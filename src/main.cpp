/*

~/.platformio/packages/framework-arduinoespressif32/tools/partitions/max.csv
~/.platformio/packages/framework-arduinoespressif32/variants/esp32max/pins_arduino.h
~/.platformio/platforms/espressif32/boards/esp32max.json

*/

#define SCAN_TIME 5      // seconds
#define PRESENCE_TIME 20 // seconds
#define ACTION_ON 2000   // useconds
#define SLEEP_TIME 0     // seconds

#ifdef BOARD1
// PIN3-J2
#define D15 15
#endif

#include <Arduino.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include "BLEBeacon.h"

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

BLEScan *pBLEScan;
int presence_counter = 0;
bool presence_fired = false;

void doActionOn()
{
  presence_fired = true;
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(D15, LOW);
  delay(ACTION_ON);
  digitalWrite(D15, HIGH);
}

void doActionOff()
{
  presence_fired = false;
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(D15, HIGH);
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice d)
  {

    if (d.haveManufacturerData())
    {
      std::string strManufacturerData = d.getManufacturerData();

      uint8_t cManufacturerData[100];
      strManufacturerData.copy((char *)cManufacturerData, strManufacturerData.length(), 0);

      if (strManufacturerData.length() == 25 && cManufacturerData[0] == 0x4C && cManufacturerData[1] == 0x00)
      {
        BLEBeacon oBeacon = BLEBeacon();
        oBeacon.setData(strManufacturerData);
        Serial.printf("iBeacon: %d04X major:%d minor:%d UUID:%s power:%d rssi:%i\n",
                      oBeacon.getManufacturerId(), oBeacon.getMajor(), oBeacon.getMinor(),
                      oBeacon.getProximityUUID().toString().c_str(), oBeacon.getSignalPower(),
                      d.getRSSI());
        presence_counter = PRESENCE_TIME;
        if (not presence_fired)
          doActionOn();
      } else {
        Serial.printf("device with no manufacter data: %s rssi:%i\n", d.toString().c_str(), d.getRSSI());  
      }
    } else {
      Serial.printf("generic device: %s rssi:%i\n", d.toString().c_str(), d.getRSSI());
    }
  }
};

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(D15, OUTPUT);
  doActionOff();

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
  
  Serial.begin(115200);
  Serial.println("ESP32 BLE Scanner");

  BLEDevice::init("blescanner");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
}

void loop()
{
  BLEScanResults foundDevices = pBLEScan->start(SCAN_TIME);

  presence_counter = presence_counter - SCAN_TIME;
  if (presence_counter <= 0)
    doActionOff();

  delay(SLEEP_TIME * 1000);
}
