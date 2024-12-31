#include <map>
#include <vector>
#include <string>
#include <queue>
#include <esp_now.h>
#include <WiFi.h>
#include <SD.h>
#include <SPI.h>

#define SD_CS 5  // Adjust this to your SD Card CS pin

// Node for Huffman Tree
struct Node {
    float data;
    double probability;
    Node *left, *right;

    Node(float data, double probability) : data(data), probability(probability), left(nullptr), right(nullptr) {}
};

// Comparator for priority_queue
struct Compare {
    bool operator()(Node* l, Node* r) {
        return l->probability > r->probability;
    }
};

// Function prototypes
Node* buildHuffmanTree(std::vector<Node*>& nodes);
void generateHuffmanCodes(Node* root, std::map<float, std::string>& huffmanCodes, std::string str);
void compressData(const std::vector<float>& suhuData, std::map<float, std::string>& huffmanCodes, std::string& compressedData);
void sendData(const std::string& compressedData, const std::string& huffmanCodeData, const uint8_t* receiverAddress);
void printHuffmanCodes(const std::map<float, std::string>& huffmanCodes);
std::vector<float> readDataFromSD(const char* filename);

// Global variables
std::vector<float> suhuData;
uint8_t receiverAddress[] = {0xfc, 0xe8, 0xc0, 0x78, 0xd1, 0xd4}; // Change to the receiver's MAC address

// Function to build Huffman Tree
Node* buildHuffmanTree(std::vector<Node*>& nodes) {
    std::priority_queue<Node*, std::vector<Node*>, Compare> minHeap;

    for (auto node : nodes) {
        minHeap.push(node);
    }

    while (minHeap.size() > 1) {
        Node *left = minHeap.top(); minHeap.pop();
        Node *right = minHeap.top(); minHeap.pop();
        Node *top = new Node(-1, left->probability + right->probability);
        top->left = left;
        top->right = right;
        minHeap.push(top);
    }

    return minHeap.top();
}

// Function to generate Huffman Codes
void generateHuffmanCodes(Node* root, std::map<float, std::string>& huffmanCodes, std::string str) {
    if (!root) return;
    if (!root->left && !root->right) {
        huffmanCodes[root->data] = str;
    }
    generateHuffmanCodes(root->left, huffmanCodes, str + "0");
    generateHuffmanCodes(root->right, huffmanCodes, str + "1");
}

// Function to compress temperature data
void compressData(const std::vector<float>& suhuData, std::map<float, std::string>& huffmanCodes, std::string& compressedData) {
    std::map<float, unsigned> freq;
    for (float num : suhuData) {
        freq[num]++;
    }

    std::vector<Node*> nodes;
    double total = suhuData.size();
    for (const auto& pair : freq) {
        double probability = pair.second / total;
        nodes.push_back(new Node(pair.first, probability));
    }

    Node* root = buildHuffmanTree(nodes);
    generateHuffmanCodes(root, huffmanCodes, "");

    compressedData.clear();
    for (float num : suhuData) {
        compressedData += huffmanCodes[num];
    }

    for (Node* node : nodes) {
        delete node;
    }
}

// Function to send data via ESP-NOW
void sendData(const std::string& compressedData, const std::string& huffmanCodeData, const uint8_t* receiverAddress) {
    std::string message = compressedData + ";" + huffmanCodeData + "*"; // Add '*' at the end
    const size_t maxPacketSize = 250; // Adjust ESP-NOW max size if necessary
    size_t start = 0;

    while (start < message.length()) {
        size_t length = std::min(maxPacketSize, message.length() - start);
        std::string chunk = message.substr(start, length);
        Serial.print("Mengirim data: ");
        Serial.println(chunk.c_str());
        esp_now_send(receiverAddress, (const uint8_t*)chunk.c_str(), chunk.length());
        start += length;
    }
}

// Function to print Huffman Codes
void printHuffmanCodes(const std::map<float, std::string>& huffmanCodes) {
    Serial.println("Kode Huffman:");
    for (const auto& pair : huffmanCodes) {
        Serial.print("Karakter: ");
        Serial.print(pair.first, 2);  // Print with two decimals
        Serial.print(", Kode: ");
        Serial.println(pair.second.c_str());
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
    std::map<float, std::string> huffmanCodes;
    std::string compressedData;
    
    
    float start_time = esp_timer_get_time();

    compressData(suhuData, huffmanCodes, compressedData);
    // End timer for compression
    float end_time = esp_timer_get_time();

    float compression_time = (end_time - start_time) / 1000.0; // Convert to milliseconds
    Serial.printf("Waktu Kompresi: %.3f ms\n", compression_time);

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

    // Tampilkan hasil ukuran data
    Serial.print("Ukuran data sebelum kompresi ");
    Serial.print(dataLength);
    Serial.println(" bytes");
    Serial.print("Ukuran data setelah kompresi: ");
    Serial.print(compressedData.size());
    Serial.println(" bytes");

    delay(3000);  // 3-second delay after compression

    // Step 2: Send data
    Serial.println("\nMemulai pengiriman data...");
    std::string huffmanCodeData;
    for (const auto& pair : huffmanCodes) {
        char buffer[15];
        snprintf(buffer, sizeof(buffer), "%.2f:%s;", pair.first, pair.second.c_str());
        huffmanCodeData += buffer;
    }
    
    // Start timer for data transmission
    start_time = esp_timer_get_time();
    sendData(compressedData, huffmanCodeData, receiverAddress);
    // End timer for data transmission
    end_time = esp_timer_get_time();
    // Calculate and print transmission time in milliseconds
    float transmission_time = (end_time - start_time) / 1000.0; // Convert to milliseconds
    Serial.printf("Waktu pengiriman: %.3f ms\n", transmission_time);

    Serial.println("Pengiriman data selesai.");

    delay(2000);  // 2-second delay after sending data
}