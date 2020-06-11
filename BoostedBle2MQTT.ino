#include "BLEDevice.h"
#include "WiFi.h"
#include "PubSubClient.h"

const char* ssid = "xxxxxxxx";
const char* password = "xxxxxxxxxxxx";
const char* mqtt_server = "broker.mqtt-dashboard.com";

// The remote service we wish to connect to.
static BLEUUID batteryServiceUUID("65a8eaa8-c61f-11e5-9912-ba0be0483c18");
// The characteristic of the remote service we are interested in.
static BLEUUID    batteryUUID("65a8eeae-c61f-11e5-9912-ba0be0483c18");

static BLEAddress macAddress = BLEAddress("xx:xx:xx:xx:xx:xx");

static BLEAddress *pServerAddress;
static boolean doConnect = false;
static boolean connected = false;
static BLERemoteCharacteristic* notifyChar;

WiFiClient espClient;
PubSubClient client(espClient);

String result;

void mqttconnect() {
  /* Loop until reconnected */
  while (!client.connected()) {
    Serial.print("MQTT connecting ...");
    /* client ID */
    String clientId = "ESP32Client";
    /* connect now */
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
    } else {
      Serial.print("failed, status code =");
      Serial.print(client.state());
      Serial.println("try again in 5 seconds");
      /* Wait 5 seconds before retrying */
      delay(5000);
    }
  }
}

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  Serial.print("Notify callback for characteristic ");
  Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
  Serial.print(" of data length ");
  Serial.println(length);
}

void connectToServer(BLEAddress pAddress) {
  Serial.print("Forming a connection to ");
  Serial.println(pAddress.toString().c_str());

  BLEClient*  pClient  = BLEDevice::createClient();
  Serial.println(" - Created client");

  // Connect to the remove BLE Server.
  pClient->connect(pAddress);
  Serial.println(" - Connected to server");

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(batteryServiceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to get service: ");
    Serial.println(batteryServiceUUID.toString().c_str());
    return;
  }

  // Obtain a reference to the characteristic in the service of the remote BLE server.
  notifyChar = pRemoteService->getCharacteristic(batteryUUID);

  if (notifyChar == nullptr) {
    Serial.print("Failed to get BATTERY: ");
    Serial.println(batteryUUID.toString().c_str());
    return;
  }
  
      // Read the value of the characteristic.
      std::string value = notifyChar->readValue();
      Serial.print("Battery: ");
      Serial.println(value.c_str()[0], DEC);
      Serial.println("%");

  

  result = (value.c_str()[0],DEC);
  
  notifyChar->registerForNotify(notifyCallback);
  Serial.println("Done connecting");
  delay(400);
}
/**
   Scan for BLE servers and find the first one that advertises the address we are looking for.
*/
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    /**
        Called for each advertising BLE server.
    */
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      Serial.print("BLE Advertised Device found: ");
      Serial.println(advertisedDevice.toString().c_str());

     if (advertisedDevice.getAddress().equals(macAddress)) {
   // if (advertisedDevice.haveServiceUUID() && advertisedDevice.getServiceUUID().equals(batteryServiceUUID)) {
        advertisedDevice.getScan()->stop();

        Serial.print("Found our device!  address: ");
        Serial.println(advertisedDevice.getAddress().toString().c_str());

        pServerAddress = new BLEAddress(advertisedDevice.getAddress());
        doConnect = true;

        /*
          MyClient* pMyClient = new MyClient();
          pMyClient->setStackSize(18000);
          pMyClient->start(new BLEAddress(*advertisedDevice.getAddress().getNative()));
        */
      } // Found our server
    } // onResult
}; // MyAdvertisedDeviceCallbacks

void setup()
{
  Serial.begin(115200);
  while (!Serial);

  Serial.println("Starting ...");
  BLEDevice::init("");
  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(30);

  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address : ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, 1883);
  
}

void loop()
{
  // put your main code here, to run repeatedly:
  if (doConnect == true) {
    connectToServer(*pServerAddress);
    doConnect = false;
    connected = true;
  }
  if (connected) {
    Serial.println(result);
    client.publish("ESPert/ilzam", result.c_str());
  }

  if (!client.connected()) {
    mqttconnect();
  }
  
  delay(1000);
}
