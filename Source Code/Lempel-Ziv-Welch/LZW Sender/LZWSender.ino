#include <WiFi.h>
#include <esp_now.h>
#include <vector>
#include <map>
#include <string>
#include <SD.h>
#include <SPI.h>

#define SD_CS 5
const uint8_t RECEIVER_MAC[] = {0xFC, 0xE8, 0xC0, 0x78, 0xD1, 0xD4};

std::string sdData;  // Global variable to store data read from SD card
std::string encodedString;  // Global variable to store compressed data
bool sdReadComplete = false; // Flag to ensure SD reading is complete

// Function for LZW encoding
std::vector<int> lzw_encode(const std::string& input) {
    if (input.empty()) return {};

    std::map<std::string, int> dictionary;
    for (int i = 0; i < 256; ++i) {
        dictionary[std::string(1, char(i))] = i;
    }

    std::vector<int> result;
    std::string current;
    int dictSize = 256;

    for (char c : input) {
        std::string next = current + c;
        if (dictionary.count(next)) {
            current = next;
        } else {
            result.push_back(dictionary[current]);
            if (dictSize < 4096) { // Prevent dictionary from growing too large
                dictionary[next] = dictSize++;
            }
            current = std::string(1, c);
        }
    }

    if (!current.empty()) {
        result.push_back(dictionary[current]);
    }

    return result;
}

// Function to send encoded data as a string with spaces
void sendData(const String& compressedData, const uint8_t* receiverAddress) {
    String message = compressedData + "*"; // Add '*' at the end to signify the last packet
    const size_t maxPacketSize = 250;
    size_t start = 0;

    while (start < message.length()) {
        size_t length = std::min(maxPacketSize, message.length() - start);
        String chunk = message.substring(start, start + length);

        Serial.print("Mengirim data: ");
        Serial.println(chunk.c_str());

        esp_now_send(receiverAddress, (const uint8_t*)chunk.c_str(), chunk.length());
        start += length;
    }
}

// Function to read from SD card
std::string readFromSD(const char* filename) {
    File file = SD.open(filename);
    if (!file) {
        Serial.println("Failed to open file for reading");
        return "";
    }

    std::string content;
    while (file.available()) {
        String line = file.readStringUntil('\n'); // Read until newline
        line.trim();  // Removes any leading and trailing whitespace, including newlines
        content += line.c_str(); // Add to content without newline
    }
    file.close();
    return content;
}

// Function to convert encoded data to space-separated string
std::string encodeToString(const std::vector<int>& encoded) {
    std::string result;
    for (size_t i = 0; i < encoded.size(); ++i) {
        result += std::to_string(encoded[i]);
        if (i < encoded.size() - 1) {
            result += " "; // Add space here
        }
    }
    return result;
}

void setup() {
    delay(2000);  // Initial 2-second delay
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);

    if (!SD.begin(SD_CS)) {
        Serial.println("SD Card initialization failed!");
        return;
    }
    Serial.println("SD Card initialized successfully.");

    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    esp_now_peer_info_t peerInfo;
    memcpy(peerInfo.peer_addr, RECEIVER_MAC, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;
    }

    // Read data from SD card once
    sdData = readFromSD("/dht22_data_suhu.txt");
    if (sdData.empty()) {
        Serial.println("Failed to read data from SD card or no valid data found");
        return;
    }

    // Cetak data dari SD card
    Serial.println("Data read from SD card:");
    Serial.println(sdData.c_str());

    // Cetak ukuran data asli
    Serial.printf("Size of original data (in bytes): %zu\n", sdData.length());


    sdReadComplete = true; // Set flag to indicate completion
}

void loop() {
    if (!sdReadComplete) {
        return; // Exit loop until SD card reading is complete
    }

    // Perform LZW compression on the data read from SD
    int64_t start_time = esp_timer_get_time();
    std::vector<int> encoded = lzw_encode(sdData);  
    int64_t end_time = esp_timer_get_time();

    float compression_time = (end_time - start_time) / 1000.0; // Convert to milliseconds
    Serial.printf("Waktu kompresi: %.3f ms\n", compression_time);

    encodedString = encodeToString(encoded);

    Serial.print("Compressed data: ");
    Serial.println(encodedString.c_str());
    Serial.print("Size of compressed data (in bytes): ");
    Serial.println(encodedString.length());

    delay(3000);  // Delay 3 seconds before sending data

    // Send compressed data
    start_time = esp_timer_get_time();
    sendData(String(encodedString.c_str()), RECEIVER_MAC);
    end_time = esp_timer_get_time();

    float transmission_time = (end_time - start_time) / 1000.0; // Convert to milliseconds
    Serial.printf("Waktu pengiriman: %.3f ms\n", transmission_time);

    delay(2000);  // Delay 2 seconds before compression
}
