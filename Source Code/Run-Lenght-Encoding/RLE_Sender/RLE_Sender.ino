#include <map>
#include <vector>
#include <string>
#include <queue>
#include <esp_now.h>
#include <WiFi.h>
#include <SD.h>
#include <SPI.h>

#define SD_CS 5  // Adjust this to your SD Card CS pin

// Function prototypes
String rleCompress(const std::vector<float>& data);
void sendData(const String& compressedData, const uint8_t* receiverAddress);
std::vector<float> readDataFromSD(const char* filename);

// Global variables
std::vector<float> suhuData;
uint8_t receiverAddress[] = {0xfc, 0xe8, 0xc0, 0x78, 0xd1, 0xd4}; // Change to the receiver's MAC address

// Function to compress temperature data using RLE
String rleCompress(const std::vector<float>& data) {
    String compressed = "";
    int count = 1;

    // Check if the data vector is empty
    if (data.empty()) {
        return compressed; // Return empty string if no data
    }

    for (size_t i = 0; i < data.size() - 1; i++) {
        if (data[i] == data[i + 1]) {
            count++;
        } else {
            // When data is different, append the value with @N
            compressed += String(data[i], 2) + "@" + String(count) + "|";
            count = 1; // Reset count for new value
        }
    }

    // Handle the last element
    compressed += String(data.back(), 2) + "@" + String(count) + "|";

    return compressed;
}

// Function to send data via ESP-NOW
void sendData(const String& compressedData, const uint8_t* receiverAddress) {
    String message = compressedData + "*"; // Add '*' at the end
    const size_t maxPacketSize = 250; // Adjust ESP-NOW max size if necessary
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

// Function to read data from SD Card
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

    // Read temperature data from SD Card
    Serial.println("Membaca data suhu dari SD Card...");
    suhuData = readDataFromSD("/dht22_data_suhu.txt");
        // Print the temperature data with right alignment
    Serial.print("Data Suhu: ");
    for (float num : suhuData) {
        Serial.print(num, 2); // Format angka dengan 2 desimal
        Serial.print(" ");
    }
    Serial.println();

    Serial.println("Setup selesai. Memulai siklus kompresi dan pengiriman...");
}

void loop() {
    // Step 1: Compress data
    
    Serial.println("\nMemulai kompresi data...");
      // Start timer for compression
    float start_time = esp_timer_get_time();
    String compressedData = rleCompress(suhuData);
      // End timer for compression
    float end_time = esp_timer_get_time();
        // Calculate and print compression time in milliseconds
    float compression_time = (end_time - start_time) / 1000.0; // Convert to milliseconds
    Serial.printf("Waktu kompresi: %.3f ms\n", compression_time);

    String suhuDataStr = "";
  
    // Konversi tiap elemen float dalam suhuData ke string dan tambahkan ke suhuDataStr
    for (float suhu : suhuData) {
        suhuDataStr += String(suhu, 2);
    }

    // Hitung ukuran data dalam string (tanpa koma)
    int dataLength = 0;
    for (int i = 0; i < suhuDataStr.length(); i++) {
        if (suhuDataStr[i] != ',') {
            dataLength++;
        }
    }
    Serial.print("Ukuran data sebelum kompresi: ");
    Serial.print(dataLength);
    Serial.println(" bytes");
    Serial.print("Ukuran data setelah kompresi: ");
    Serial.print(compressedData.length());
    Serial.println(" bytes");

    delay(3000);  // 3-second delay after compression

    // Step 2: Send data
    Serial.println("\nMemulai pengiriman data...");
      // Start timer for data transmission
    start_time = esp_timer_get_time();
    sendData(compressedData, receiverAddress);
    // End timer for data transmission
    end_time = esp_timer_get_time();
        // Calculate and print transmission time in milliseconds
    float transmission_time = (end_time - start_time) / 1000.0; // Convert to milliseconds
    Serial.printf("Waktu pengiriman: %.3f ms\n", transmission_time);

    
    Serial.println("Pengiriman data selesai.");

    delay(2000);  // 2-second delay after sending data
}
