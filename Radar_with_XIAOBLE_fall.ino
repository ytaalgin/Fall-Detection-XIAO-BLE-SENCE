#include <Wire.h>
#include <ArduinoBLE.h>
#include <60ghzfalldetection.h>
#include <ChainableLED.h>

ChainableLED leds(4, 5, 1);

FallDetection_60GHz radar = FallDetection_60GHz(&Serial1);
BLEService radarService("19B10000-E8F2-537E-4F6C-D104768A1214"); // Bluetooth® Low Energy LED Service

// Bluetooth® Low Energy LED Switch Characteristic - custom 128-bit UUID, read and writable by central
BLEStringCharacteristic switchCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify, 20);

bool sentNotification = false; // Bildirim gönderildi mi?

unsigned long previousNotificationTime = 0;
const unsigned long notificationInterval = 1000; // 1 saniye

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200);
  delay(200);
  while (!Serial);

  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting XIAO BLE with 60GHz radar sensor demo failed!");
    while (1);
  }

  // set advertised local name and service UUID:
  BLE.setLocalName("60Ghz Fall Module");
  BLE.setAdvertisedService(radarService);

  // add the characteristic to the service
  radarService.addCharacteristic(switchCharacteristic);

  // add service
  BLE.addService(radarService);

  // start advertising
  BLE.advertise();

  Serial.println("Fall Module active, waiting for connections...");

  // led settings
  leds.init();
}

void loop() {
  // listen for Bluetooth® Low Energy peripherals to connect:
  BLEDevice central = BLE.central();

  // if a central is connected to peripheral:
  if (central) {
    Serial.print("Connected to central: ");
    // print the central's MAC address:
    Serial.println(central.address());

    leds.setColorRGB(0, 255, 0, 255); // Mor

    String message;
    
    // while the central is still connected to peripheral:
    while (central.connected()){
      radar.Fall_Detection();           //Receive radar data and start processing
      if(radar.sensor_report != 0x00){
        switch(radar.sensor_report){
            case NOFALL: {
                message = "Problem yok.";
                leds.setColorRGB(0, 0, 255, 0); // Yeşil
                Serial.println("The sensor detects this movement is not a fall.");
                sendNotification(message);
                break;
            }
            case FALL: {
                message = "Düşme algılandı!";
                leds.setColorRGB(0, 255, 0, 0); // Kırmızı
                Serial.println("The sensor detects a fall.");
                sendNotification(message);
                break;
            }
            case NORESIDENT: {
                Serial.println("The sensors did not detect anyone staying in place.");
                break;
            }
            case RESIDENCY: {
                Serial.println("The sensor detects someone staying in place.");
                break;
            }
        }
       }
      }

    // when the central disconnects, print it out:
      Serial.print(F("Disconnected from central: "));
      Serial.println(central.address());
  }
}

void sendNotification(String message) {
  unsigned long currentMillis = millis();
  if (currentMillis - previousNotificationTime >= notificationInterval) {
    previousNotificationTime = currentMillis;
    switchCharacteristic.writeValue(message.c_str());
  }
}
