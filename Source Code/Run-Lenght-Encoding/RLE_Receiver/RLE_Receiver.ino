#include <WiFi.h>
#include <esp_now.h>
#include <vector>

std::vector<String> rleDecompress(String compressed);

String accumulatedData = "";
bool dataComplete = false;

void setup() {
  Serial.begin(115200);
  
  // Initialize Wi-Fi
  WiFi.mode(WIFI_STA);
  
  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register callback to receive data
  esp_now_register_recv_cb(onReceiveData);

  Serial.println("Receiver initialized and waiting for data...");
}

// Updated callback to receive data
void onReceiveData(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int len) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           esp_now_info->src_addr[0], esp_now_info->src_addr[1], esp_now_info->src_addr[2],
           esp_now_info->src_addr[3], esp_now_info->src_addr[4], esp_now_info->src_addr[5]);

  String receivedChunk = String((char*)data, len);
  Serial.printf("Received chunk from: %s, Length: %d\n", macStr, len);

  // Check for end marker
  int endMarkerPos = receivedChunk.indexOf('*');
  if (endMarkerPos != -1) {
    accumulatedData += receivedChunk.substring(0, endMarkerPos);
    dataComplete = true;
  } else {
    accumulatedData += receivedChunk;
  }

  if (dataComplete) {
    processAccumulatedData();
  }
}

void processAccumulatedData() {
  Serial.println("All data received. Processing...");
  Serial.print("Total compressed data length: ");
  Serial.println(accumulatedData.length());

  // Print compressed data first
  Serial.print("Compressed Data: ");
  Serial.println(accumulatedData);

  // Decompress and store results
  std::vector<String> decompressedList = rleDecompress(accumulatedData);
  
  // Print decompressed data horizontally (side by side)
  Serial.print("Decompressed Data: ");
  String decompressedStr = "";  // To concatenate decompressed strings into one string
  for (const auto& value : decompressedList) {
    Serial.print(value);
    Serial.print(" ");
    decompressedStr += value;  // Concatenate the strings
  }
  Serial.println();

  // Calculate the byte size of the decompressed string
  size_t decompressedSize = decompressedStr.length();  // Length in bytes
  Serial.print("Ukuran Data Setelah Dekompresi: ");
  Serial.println(decompressedSize);  // Print size in bytes

  // Reset for next transmission
  accumulatedData = "";
  dataComplete = false;
}

// Function to decompress RLE
std::vector<String> rleDecompress(String compressed) {
  std::vector<String> decompressedList;
  String numBuffer = "";
  String tempBuffer = "";

  for (int i = 0; i < compressed.length(); i++) {
    char c = compressed[i];

    if (c == '@') {
      i++;
      while (i < compressed.length() && isDigit(compressed[i])) {
        numBuffer += compressed[i];
        i++;
      }
      i--; // To balance the increment in the loop
      int count = numBuffer.toInt();
      for (int j = 0; j < count; j++) {
        decompressedList.push_back(tempBuffer);
      }
      numBuffer = "";
      tempBuffer = "";
    } else if (c == '|') {
      if (!tempBuffer.isEmpty()) {
        decompressedList.push_back(tempBuffer);
        tempBuffer = "";
      }
    } else {
      tempBuffer += c;
    }
  }

  return decompressedList;
}

void loop() {
  // No additional logic here
  delay(10);
}
