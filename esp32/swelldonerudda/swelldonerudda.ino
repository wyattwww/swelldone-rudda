/*
 * Swelldone Rudda BLE controller 
 * 
 * Hook up 4 SPST switches a ESP32 board, on pins 22 (left), 21 (right), 18 (Button 1), 17 (Button 2)
 * Tie the other pole of the SPSTs to 3V3 (high)
 * Standard settings for ESP32 are used for compile & download
 * 
 * Written on Arduino IDE 1.8.12
 *
 * Tested using Swelldone on iOS, Mac, and Android.
 *
 * v0.1 April 2021 Wyatt Wong
 *   
 * Licensed under GNU GPL-3
 *
 */

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <WiFi.h>

#define RUDDA_SERVICE_UUID "15358001-7635-408b-8918-8ff3949ce592"
#define CHAR14_UUID         "15358014-7635-408b-8918-8ff3949ce592"
#define CHAR30_UUID         "15358030-7635-408b-8918-8ff3949ce592"
#define CHAR31_UUID         "15358031-7635-408b-8918-8ff3949ce592"
#define CHAR32_UUID         "15358032-7635-408b-8918-8ff3949ce592"

bool deviceConnected = false;
BLECharacteristic *pChar32;
BLECharacteristic *pChar30;
BLE2902 *p2902Char14;
BLE2902 *p2902Char30;
BLE2902 *p2902Char32;
int pinLEFT = 22;
int pinRIGHT = 21;
int pinButton1 = 18;
int pinButton2 = 17;


bool challengeOK = false;
bool ntf32On = false;
bool button1 = false;
bool button2 = false;
bool buttonLeft = false;
bool buttonRight = false;

void printHex(uint8_t num) {
  char hexCar[2];
  sprintf(hexCar, "%02X", num);
  Serial.print(hexCar);
  Serial.print(" ");
}

static void bleNotifyControls() {
  if ((deviceConnected) && (challengeOK)) { 
    unsigned int value = 0;
    if( buttonLeft ) {
      value |= ( 1 << 0);
    }
    if( buttonRight ) {
      value |= ( 1 << 1);
    }
    if( button1 ) {
      value |= ( 1 << 2);
      button1 = false;
    }
    if( button2 ) {
      value |= ( 1 << 3);
      button2 = false;
    }
    Serial.print("Sending ");
    printHex(value);
    Serial.println();
    pChar30->setValue((uint8_t *)&value,4);
    pChar30->notify();
  }
}

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Connected to server");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      challengeOK = false;
      Serial.println("Disconnected from server");

      resetConnection();
    }
};

class char12Callbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    Serial.print("char12EventHandler : ");
    Serial.println(pCharacteristic->getValue().c_str());
  }
};

// when app writes to 0x310, we send a challenge on char32
class char31Callbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    std::string val = pCharacteristic->getValue();
    Serial.print("char31EventHandler : ");
    for (int i=0;i<4;i++){
      Serial.print(val[i],HEX);
      Serial.print('-');
    }
    Serial.println();
    
    if ((val[0] == 0x03) && (val[1] == 0x10)) {
      uint8_t challenge[] = {0x03,0x10,0x4a,0x89};//{0x03,0x10,0x12,0x34};
      Serial.println("char31EventHandler : got 0x310!");
      pChar32->setValue(challenge,4);
      pChar32->indicate();
    }
    else if ((val[0] == 0x03) && (val[1] == 0x11)) {
      unsigned char fakeData[] = {0x03,0x11,0xff, 0xff};
      Serial.println("char31EventHandler : got 0x311!");
      challengeOK = true;
      pChar32->setValue(fakeData,4);
      pChar32->indicate();
    }
  } // onWrite
};

class char32NtfCallbacks:public BLEDescriptorCallbacks {
  void onWrite(BLEDescriptor* pDescriptor) {
    ntf32On = true;
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting setup...");
  pinMode(pinLEFT,INPUT_PULLDOWN);
  pinMode(pinRIGHT,INPUT_PULLDOWN);
  pinMode(pinButton1,INPUT_PULLDOWN);
  pinMode(pinButton2,INPUT_PULLDOWN);

  startServices();
}

void resetConnection() 
{
  Serial.println("Resetting Rudda...");
  
  startAdvertising();
}

void startServices() 
{
  String macAddress = WiFi.macAddress();
  macAddress.replace(String(":"),String(""));
  macAddress = macAddress.substring(0,8);

  String deviceName = "Swelldone Rudda " + macAddress;
  Serial.println("Starting " + deviceName + "...");

  BLEDevice::init(deviceName.c_str());
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pSvcSterzo = pServer->createService(RUDDA_SERVICE_UUID);
  BLECharacteristic *pChar14 = pSvcSterzo->createCharacteristic(CHAR14_UUID,BLECharacteristic::PROPERTY_NOTIFY);
  pChar30 = pSvcSterzo->createCharacteristic(CHAR30_UUID,BLECharacteristic::PROPERTY_NOTIFY); 
  BLECharacteristic *pChar31 = pSvcSterzo->createCharacteristic(CHAR31_UUID,BLECharacteristic::PROPERTY_WRITE); 
  pChar32 = pSvcSterzo->createCharacteristic(CHAR32_UUID,BLECharacteristic::PROPERTY_INDICATE); 
  
  p2902Char14 = new BLE2902();
  p2902Char30 = new BLE2902();
  p2902Char32 = new BLE2902();
  p2902Char32->setCallbacks(new char32NtfCallbacks());
  pChar14->addDescriptor(p2902Char14);
  pChar30->addDescriptor(p2902Char30);
  pChar32->addDescriptor(p2902Char32);

  // initial values
  uint8_t defaultValue[4] = {0x0,0x0,0x0,0x0};
  pChar30->setValue(defaultValue,4); // default angle = 0
  defaultValue[0] = 0xFF; // fill other characteristics with a default 0xFF
  pChar14->setValue(defaultValue,1);
  pChar31->setValue(defaultValue,1);
  uint8_t challenge[] = {0x03,0x10,0x4a, 0x89};
  pChar32->setValue(challenge,4);

  pChar31->setCallbacks(new char31Callbacks());
  pChar32->setCallbacks(new char31Callbacks());

  pSvcSterzo->start();

  startAdvertising();
}

void startAdvertising() 
{  
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(RUDDA_SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Rudda ready!");
}

uint32_t bleNotifyMillis = 0;
bool debounceButton1 = false;
bool debounceButton2 = false;
bool debounceTime = 10;
uint32_t debounceButton1Time = 0;
uint32_t debounceButton2Time = 0;

void loop() {
  bool shouldNotify = false;
  buttonLeft = digitalRead(pinLEFT);
  buttonRight = digitalRead(pinRIGHT);

  bool b1 = digitalRead(pinButton1);
  if( b1 && !debounceButton1 )
  {
    button1 = true;
    debounceButton1 = true;
  }
  else if( !b1 && debounceButton1 && debounceButton1Time == 0 )
  {
    debounceButton1Time = millis();
  }
  bool b2 = digitalRead(pinButton2);
  if( b2 && !debounceButton2 )
  {
    button2 = true;
    debounceButton2 = true;
  }
  else if( !b2 && debounceButton2 && debounceButton2Time == 0 )
  {
    debounceButton2Time = millis();
  }

  if( debounceButton1 &&
     debounceButton1Time > 0 &&
    (millis() - debounceButton1Time) > debounceTime ) 
  {
      debounceButton1Time = 0;
      debounceButton1 = false;
  }
  
  if( debounceButton2 &&
     debounceButton2Time > 0 &&
    (millis() - debounceButton2Time) > debounceTime ) 
  {
      debounceButton2Time = 0;
      debounceButton2 = false;
  }
  
  // Send the initial challenge on char32
  if ((ntf32On) && (deviceConnected)) {
    ntf32On = false;
    uint8_t challenge[] = {0x03,0x10,0x4a, 0x89};
    pChar32->setValue(challenge,4);
    pChar32->indicate();
  }
  
  if (deviceConnected) {
    // notify steering angle every half second
    if (millis() - bleNotifyMillis > 200) {
      bleNotifyControls ();
      bleNotifyMillis = millis();
    }
  }
} // loop
