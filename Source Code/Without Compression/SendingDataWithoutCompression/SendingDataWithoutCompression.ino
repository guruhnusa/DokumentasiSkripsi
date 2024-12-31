#include <WiFi.h>
#include <esp_now.h>
#include <SD.h>
#include <vector>
#include "esp_timer.h"  // Include for the hardware timer

#define SD_CS 5  // Adjust this to your SD Card CS pin
uint8_t receiverAddress[] = {0xfc, 0xe8, 0xc0, 0x78, 0xd1, 0xd4}; // Receiver's MAC address
std::vector<float> suhuData; // Holds the temperature data as floats
String suhuDataStr; // String to hold the converted temperature data

// Callback function for ESP-NOW message delivery status
void onSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.print("\r\nLast Packet Send Status: ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// Function to read data from SD card as floats
std::vector<float> readDataFromSD(const char* filename) {
    std::vector<float> data;
    File file = SD.open(filename);
    if (!file) {
        Serial.println("Failed to open file for reading");
        return data;
    }

    // Read the file line by line
    while (file.available()) {
        String line = file.readStringUntil('\n');  
        float temperature = line.toFloat();  // Convert the whole line to float
        if (temperature != 0.0 || line == "0") {  // Ensure valid temperature value
            data.push_back(temperature);
        }
    }

    file.close();
    return data;
}
// Convert the vector of floats into a string
String convertFloatVectorToString(const std::vector<float>& data) {
    String result = "";
    for (float value : data) {
        result += String(value, 2) + ",";  // Convert each float to a string with 2 decimal places
    }
    if (result.length() > 0) {
        result.remove(result.length() - 1);  // Remove the last comma
    }
    return result;
}

// Function to send data over ESP-NOW in 250-byte chunks
void sendTemperatureData(const String& dataStr) {
    const int maxPacketSize = 250; // Max size of each packet
    int totalLength = dataStr.length();
    int totalPackets = (totalLength / maxPacketSize) + (totalLength % maxPacketSize ? 1 : 0);
    
    for (int i = 0; i < totalLength; i += maxPacketSize) {
        String packet = dataStr.substring(i, min(i + maxPacketSize, totalLength));
        
        // Add '*' to the last packet to signal the end
        if (i + maxPacketSize >= totalLength) {
            packet += "*";
        }

        // Print the packet being sent
        Serial.println("Packet " + String(i / maxPacketSize + 1) + " of " + String(totalPackets) + ": " + packet);

        // Send packet
        esp_err_t result = esp_now_send(receiverAddress, (uint8_t *)packet.c_str(), packet.length());
        
        if (result == ESP_OK) {
            Serial.println("Sent: " + packet);
        } else {
            Serial.println("Error sending data");
        }

    }

}

int calculateLengthWithoutCommas(const String& dataStr) {
    int length = 0;
    for (char c : dataStr) {
        if (c != ',') {
            length++;
        }
    }
    return length;
}

void setup() {
    Serial.begin(115200);
    delay(2000);  // Initial 2-second delay

    // Initialize SD Card
    if (!SD.begin(SD_CS)) {
        Serial.println("SD Card initialization failed!");
        return;
    }
    Serial.println("SD Card initialized successfully.");

    // Initialize WiFi and ESP-NOW
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW initialization failed");
        return;
    }

    // Register peer
    esp_now_peer_info_t peerInfo;
    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, receiverAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
    }

    // Register the send callback
    esp_now_register_send_cb(onSent);

    // Read temperature data from SD Card as float values
    Serial.println("Membaca data suhu dari SD Card...");
    suhuData = readDataFromSD("/dht22_data_suhu.txt");

    // Convert the float data to a string for transmission
    suhuDataStr = convertFloatVectorToString(suhuData);

    
    // Calculate and print the length without commas
    int data = calculateLengthWithoutCommas(suhuDataStr);
    Serial.print("Ukuran data sebelum kompresi (tanpa koma): ");
    Serial.println(data);
}

void loop() {
    // Print the temperature data string before sending
    Serial.println("Data suhu yang akan dikirim: " + suhuDataStr);
    
    // Send temperature data
    Serial.println("Mengirim data suhu...");

    int64_t startTime = esp_timer_get_time(); // Record the start time in microseconds
    sendTemperatureData(suhuDataStr);
    int64_t endTime = esp_timer_get_time(); // Record the end time in microseconds

    // Calculate the elapsed time in microseconds
    int64_t elapsedTime = (endTime - startTime) / 1000.0; // Convert to milliseconds

    Serial.print("Total time taken to send data: ");
    Serial.print(elapsedTime); // Print elapsed time in milliseconds
    Serial.println(" milliseconds");

    // Wait for 2 seconds before the next transmission
    delay(2000);  // Adjusted delay for the next transmission
}
