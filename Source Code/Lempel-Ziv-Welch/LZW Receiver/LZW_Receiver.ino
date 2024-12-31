#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <vector>
#include <map>
#include <string>
#include <sstream>

std::string receivedDataString; // Store received data as string
bool isTransmissionComplete = false;

// Improved LZW decoding function
std::string lzw_decode(const std::vector<int>& input) {
    if (input.empty()) return "";

    std::map<int, std::string> dictionary;
    int dictSize = 256;

    // Initialize dictionary with single character strings
    for (int i = 0; i < dictSize; i++) {
        dictionary[i] = std::string(1, char(i));
    }

    std::string result;
    std::string current = dictionary[input[0]];
    result += current;

    for (size_t i = 1; i < input.size(); i++) {
        std::string entry;
        int code = input[i];

        if (dictionary.count(code)) {
            entry = dictionary[code];
        } else if (code == dictSize) {
            entry = current + current[0];
        } else {
            Serial.printf("Invalid dictionary code: %d\n", code);
            break;
        }

        result += entry;

        // Add new string to dictionary safely
        if (dictSize < 4096) {
            dictionary[dictSize++] = current + entry[0];
        }
        current = entry;
    }

    return result;
}

// Callback function for receiving data
void onDataReceive(const esp_now_recv_info* info, const uint8_t* data, int len) {
    if (len <= 0) return;

    // Convert received bytes to string, excluding the terminator
    std::string receivedChunk(reinterpret_cast<const char*>(data), len);

    // Check for end of transmission marker
    if (receivedChunk.back() == '*') {
        receivedChunk.pop_back(); // Remove the marker
        isTransmissionComplete = true;
    }

    // Append the chunk to the full data string
    receivedDataString += receivedChunk;

    Serial.printf("Received chunk (length: %d)\n", len);
}

// Convert space-separated string to vector of integers
std::vector<int> stringToIntVector(const std::string& str) {
    std::vector<int> result;
    std::istringstream iss(str);
    int num;

    while (iss >> num) {
        result.push_back(num);
    }

    return result;
}

void setup() {
    delay(2000);
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        while (1);
    }

    esp_now_register_recv_cb(onDataReceive);
    Serial.println("ESP-NOW Receiver Initialized");
}

void loop() {
    if (isTransmissionComplete) {
        // Reset transmission complete flag
        isTransmissionComplete = false;

        Serial.printf("Total received data length: %zu bytes\n", receivedDataString.length());

        // Convert received string to integers
        std::vector<int> compressedData = stringToIntVector(receivedDataString);

        // Decode the data
        std::string decodedData = lzw_decode(compressedData);

        // Output decoded data horizontally
        Serial.print("Decoded Data: ");
        Serial.println(decodedData.c_str());


        // Print compressed and decompressed data sizes
        Serial.printf("Compressed Data Size: %zu bytes, ", receivedDataString.length());
        Serial.printf("Decoded Data Size: %zu bytes\n", decodedData.length());

        // Clear received data for next transmission
        receivedDataString.clear();
    }

    // Small delay to prevent busy-waiting
    delay(100);
}
