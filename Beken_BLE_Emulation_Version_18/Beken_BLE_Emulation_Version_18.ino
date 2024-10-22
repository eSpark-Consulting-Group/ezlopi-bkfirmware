
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <BLE2901.h>
#include <ArduinoJson.h>
#include "arduino_base64.hpp"

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristicProvisioningInfo = NULL;
BLECharacteristic *pCharacteristicWifiCredentials = NULL;
BLECharacteristic *pCharacteristicReset = NULL;
BLECharacteristic *pCharacteristicFullDeviceInfo = NULL;
BLE2901 *descriptor_2901 = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;

String encodedData;
String encodedCompData;
String decodedDataString;
String accumulatedData;
String CurrentValue;

// UUIDs for Provisioning/Wifi Service
#define PROVISIONING_SERVICE_UUID "00E3"
#define PROVISIONING_INFO_UUID "E301"
#define WIFI_CREDENTIALS_UUID "E302"
#define RESET_UUID "E303"

// UUIDs for Common Device Info Service
#define COMMON_DEVICE_INFO_SERVICE_UUID "00E4"
#define FULL_DEVICE_INFO_UUID "E401"

String jsonData = "{\
  \"serial\": 100004961,\
  \"provision_server\": \"https://req-disp-at0m.mios.com/\",\
  \"cloud_server\": \"https://cloud.ezlo.com:7000\",\
  \"provision_token\": \"c25415a5f1de9e5084dab5b0c92a28d2f5f64bb0079a9ffcc312eb005ff1d2b9bb896a097a565eb79fb15334f7c296e8ca67596cde8a87255d44e2507face3c288aed92624e355207700b07071365353acca8a1c3e6ac2e5dcce519021d2bc72cc7f5c4e85ebd0e75cc84c8b56a8a378672bcd9ac6975b511b77482001ab9a35\",\
  \"ssl_private_key\": \"-----BEGIN PRIVATE KEY-----\nMIGEAgEAMBAGByqGSM49AgEGBSuBBAAKBG0wawIBAQQgpADG/nseS5R6XDv15FcM\ntU9zNVVal8X9qzESKrLzWDehRANCAATPmLXLTDUTxBvNIm6BFhzmGirTmW+wBE8B\nAEJJb1c9zuSdSMdMMUeb89KGb3scQUZotkiH/ki/VR0+3Lvorg8v\n-----END PRIVATE KEY-----\n\",\
  \"ssl_public_key\": \"-----BEGIN PUBLIC KEY-----\nMFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAEz5i1y0w1E8QbzSJugRYc5hoq05lvsARP\nAQBCSW9XPc7knUjHTDFHm/PShm97HEFGaLZIh/5Iv1UdPty76K4PLw==\n-----END PUBLIC KEY-----\",\
  \"ssl_shared_key\": \"-----BEGIN CERTIFICATE-----\nMIICDTCCAbKgAwIBAgIDAzZDMAoGCCqGSM49BAMCMIGQMQswCQYDVQQGEwJVUzEU\nMBIGA1UECAwLIE5ldyBKZXJzZXkxEDANBgNVBAcMB0NsaWZ0b24xDzANBgNVBAoM\nBklUIE9wczEPMA0GA1UECwwGSVQgT3BzMRQwEgYDVQQDDEltWkxPIExURCBDQTEh\nMB8GCSqGSIb3DQEJARYSc3lzYWRtaW5zQGV6bG8uY29tMCAXDTIxMTIwMjA4NDYx\nMFoYDzIyOTUwOTE2MDg0NjEwWjCBjDELMAkGA1UEBhMCVVMxEzARBgNVBAgMCk5l\ndyBKZXJzZXkxEDAOBgNVBAcMB0NsaWZ0b24xEzARBgNVBAoMCmNvbnRyb2xsZXIx\nLTArBgNVBAsMJDRjMGQ5MDYwLTUzNGMtMTFlYy1iMmQ2LThmMjYwZjUyODdmYTES\nMBAGA1UEAwwJMTAwMDA0OTYxMFYwEAYHKoZIzj0CAQYFK4EEAAoDQgAEz5i1y0w1\nE8QbzSJugRYc5hoq05lvsARPAQBCSW9XPc7knUjHTDFHm/PShm97HEFGaLZIh/5I\nv1UdPty76K4PLzAKBggqhkjOPQQDAgNJADBGAiEAseX9mwtQ2DN60jCplOIIPd2S\n+6bdgpQGiCeYNpY/sBcCIQDwduNJYOIlBGmQgH1KAk9nW5JxAzA2MsPKovjTUSFB\nyg==\n-----END CERTIFICATE-----\n\",\
  \"uuid\": \"4c0d9060-534c-11ec-b2d6-8f260f5287fa\",\
  \"user_id\": \"Jon\",\
  \"device_name\": \"My Device\",\
  \"brand\": \"doe\",\
  \"manufacturer_name\": \"foo\",\
  \"model_number\": \"063DEX524\",\
  \"mac\": \"11:22:33:44:55:66\",\
  \"uuid_provisioning\": \"b558c598-a35a-4fdc-a2cd-5462ff11455a\",\
  \"device_type_ezlopi\": \"ezlopi_generic\"\
}";

String wifiCred =
  "{"
  "\"wifi_ssid\":\"WiFi SSID\","
  "\"wifi_password\":\"password\","
  "\"user_id\":\"Jon\""
  "}";

String reset =
  "{"
  "\"cmd\":0,"
  "\"sub_cmd\":0"
  "}";

String readjsonData = "{"
                      "\"device_info\":{\"firmware_version\":\"1.0.0\",\"firmware_build\":169,\"chip\":\"bk7231n\",\"uptime\":113,\"build_date\":1690548178,\"provisioned_status\":true,\"mac\":\"11:22:33:44:55:66\",\"serial\":100004961,\"ezlopi_device_type\":\"ezlopi_generic\",\"wifi_ssid\":\"eSpark Wifi\",\"wifi-connection_status\":1,\"internet_status\":1},"
                      "\"api_version_info\":{\"api\":\"1.0.0\"},"
                      "\"ezlopi_firmware_info\":{\"version\":\"3.4.7\",\"build\":185,\"build_date\":\"2024-02-29T18:46:33+545\"},"
                      "\"mcu_chip_info\":{\"type\":\"BEKEN\",\"version\":\"1.00\"},"
                      "\"mcu_halsdk_info\":{\"name\":\"TuyaOS\",\"version\":\"v3.9.0\"},"
                      "\"ezlopi_current_device_status\":{\"uptime\":\"0d 0h 1m 51s\",\"boot_count\":13,\"boot_reason\":\"Power ON\",\"flash_size\":\"2MB\",\"wifi_mac\":\"EC:62:60:A7:1D:98\",\"ble_mac\":\"EC:62:60:A7:1D:9A\"},"
                      "\"Serial_port_config\":{\"baud\":115200,\"parity\":\"N\",\"start_bits\":0,\"stop_bits\":1,\"frame_size\":8,\"flow_control\":\"DISABLED\"},"
                      "\"ezlopi_cloud_info\":{\"provisioned\":true,\"uuid\":\"1b576400-8392-11ee-8889-0f56091e7639\",\"uuid_prov\":\"c2ea484b-6b31-446e-b42e-8ab7b5c36ce4\",\"type\":\"ezlopi_generic\",\"serial\":102002167,\"mac\":\"a2:f8:75:1c:3a:4c\"},"
                      "\"oem_info\":{\"brand\":\"\",\"manufacturer\":\"\",\"model\":\"\"},"
                      "\"network_info\":{\"ssid\":\"eSpark Wifi\",\"wifi_mode\":\"STA\",\"ip_sta\":\"192.168.18.194\",\"ip_nmask\":\"255.255.255.0\",\"ip_gw\":\"192.168.18.1\"}"
                      "}";

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    Serial.print("Characteristic written: ");
    Serial.println(pCharacteristic->getUUID().toString().c_str());
  }
};

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
  };

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
  }
};

void setup() {
  Serial.begin(115200);

  /*For Common Device Info Services JSON coverting to base62 encode */
  StaticJsonDocument<4096> note;
  deserializeJson(note, readjsonData);
  String minifiedJson;
  serializeJson(note, minifiedJson);
  size_t inputLength = minifiedJson.length();
  char encodedString[base64::encodeLength(inputLength)];
  base64::encode(reinterpret_cast<const uint8_t *>(minifiedJson.c_str()), inputLength, encodedString);
  encodedCompData = String(encodedString);
  // Create the BLE Device
  BLEDevice::init("ESP32");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create BLE characteristics
  BLEService *pProvisioningService = pServer->createService(PROVISIONING_SERVICE_UUID);
  pCharacteristicProvisioningInfo = pProvisioningService->createCharacteristic(PROVISIONING_INFO_UUID, BLECharacteristic::PROPERTY_INDICATE | BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  pCharacteristicWifiCredentials = pProvisioningService->createCharacteristic(WIFI_CREDENTIALS_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
  pCharacteristicReset = pProvisioningService->createCharacteristic(RESET_UUID, BLECharacteristic::PROPERTY_WRITE);
  BLEService *pinfodeviceService = pServer->createService(COMMON_DEVICE_INFO_SERVICE_UUID);
  pCharacteristicFullDeviceInfo = pinfodeviceService->createCharacteristic(FULL_DEVICE_INFO_UUID, BLECharacteristic::PROPERTY_READ);
  // Creates BLE Descriptor 0x2902: Client Characteristic Configuration Descriptor (CCCD)
  pCharacteristicProvisioningInfo->addDescriptor(new BLE2902());
  pCharacteristicWifiCredentials->addDescriptor(new BLE2902());
  pCharacteristicReset->addDescriptor(new BLE2902());

  // Adds also the Characteristic User Description - 0x2901 descriptor
  descriptor_2901 = new BLE2901();
  descriptor_2901->setDescription("PROVISIONING INFO UUID.");
  descriptor_2901->setAccessPermissions(ESP_GATT_PERM_READ);
  pCharacteristicProvisioningInfo->addDescriptor(descriptor_2901);

  descriptor_2901 = new BLE2901();
  descriptor_2901->setDescription("WIFI CREDENTIALS.");
  descriptor_2901->setAccessPermissions(ESP_GATT_PERM_READ);
  pCharacteristicWifiCredentials->addDescriptor(descriptor_2901);

  descriptor_2901 = new BLE2901();
  descriptor_2901->setDescription("RESET.");
  descriptor_2901->setAccessPermissions(ESP_GATT_PERM_READ);
  pCharacteristicReset->addDescriptor(descriptor_2901);

  /*COMMON_DEVICE_INFO_SERVICE_UUID*/
  pCharacteristicFullDeviceInfo->addDescriptor(new BLE2902());
  descriptor_2901->setDescription("COMMON DEVICE INFO SERVICE UUID.");
  descriptor_2901->setAccessPermissions(ESP_GATT_PERM_READ);
  pCharacteristicFullDeviceInfo->addDescriptor(descriptor_2901);
  // Start the service
  pProvisioningService->start();
  pinfodeviceService->start();
  // Start advertising
  /*For Provision Services */
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(PROVISIONING_SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);

  /*For Common Device Info Services */
  BLEAdvertising *pAdvertisingCommonDevice = BLEDevice::getAdvertising();
  pAdvertisingCommonDevice->addServiceUUID(COMMON_DEVICE_INFO_SERVICE_UUID);
  pAdvertisingCommonDevice->setScanResponse(false);
  pAdvertisingCommonDevice->setMinPreferred(0x0);

  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {

  if (deviceConnected) {

    size_t chunkLen = 180;
    size_t totalLen = encodedCompData.length();
    size_t sequence = 0;

    /*for writting the info common device service that can view on device */
    while (totalLen > 0) {
      int lenToSend = min(chunkLen, totalLen);
      String chunk = "{\"total_len\": " + String(totalLen) + ", \"len\": " + String(lenToSend) + ", \"sequence\": " + String(sequence) + ", \"data\": \"";

      chunk += encodedCompData.substring(sequence * chunkLen, (sequence * chunkLen) + lenToSend);
      chunk += "\"}";

      // Serial.println(chunk);
      pCharacteristicFullDeviceInfo->setValue(chunk);

      totalLen -= lenToSend;
      sequence++;
      delay(500);
    }

    /*for getting provision data from the device*/
    String currentWifiCred = pCharacteristicWifiCredentials->getValue().c_str();
    if (currentWifiCred.length() > 0) {  // Compare current value with last received value
      wifiCred = currentWifiCred;        // Update the stored value
      Serial.println("Received new Wi-Fi credentials: " + wifiCred);
      // pCharacteristicWifiCredentials->setValue(wifiCred);
    }

    String updateReset = pCharacteristicReset->getValue().c_str();
    if (updateReset.length() > 0) {
      reset = updateReset;
      Serial.println("Received new reset command: " + reset);
    }

    String ProvisioningInfo = pCharacteristicProvisioningInfo->getValue();
    if (ProvisioningInfo.length() > 0) {
      StaticJsonDocument<2048> jsonnoc;  // Adjust size if necessary
      DeserializationError error = deserializeJson(jsonnoc, ProvisioningInfo);

      if (error) {
        Serial.print("Failed to parse JSON: ");
        Serial.println(error.f_str());
        return;
      }

      String EncodedValue = jsonnoc["data"];
      if (ProvisioningInfo != CurrentValue) {
        Serial.println("EncodedValue is: ");
        Serial.println(EncodedValue);
        accumulatedData += EncodedValue;
        Serial.println("Length of Accumulator is: ");
        Serial.println(accumulatedData.length());
      }

      if (accumulatedData.length() >= 3872) {
        Serial.println("Accumulated data reached the required length. Decoding...");
        Serial.println(accumulatedData);

        const char *encodedDataCStr = accumulatedData.c_str();


        size_t decodedLength = base64::decodeLength(encodedDataCStr);
        uint8_t decodedData[decodedLength];


        base64::decode(encodedDataCStr, decodedData);
        String decodedDataString = String((const char *)decodedData);

        Serial.println("Decoded Data:");
        Serial.println(decodedDataString);

        // Now try to parse the decoded string as JSON
        StaticJsonDocument<2048> decodedJsonDoc;
        DeserializationError decodedError = deserializeJson(decodedJsonDoc, decodedDataString);

        if (decodedError) {
          Serial.print("Failed to parse decoded JSON: ");
          Serial.println(decodedError.f_str());
          return;  // Exit if parsing failed
        }


        serializeJsonPretty(decodedJsonDoc, Serial);
        Serial.println();
        accumulatedData = "";
      }

      CurrentValue = ProvisioningInfo;
      ProvisioningInfo = "";
    }
  }


  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);                   // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising();  // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}
