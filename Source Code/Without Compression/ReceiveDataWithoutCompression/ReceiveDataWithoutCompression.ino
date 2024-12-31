#include <WiFi.h>
#include <esp_now.h>

#define MAX_DATA_SIZE 250  // Max size of each packet

String receivedData = "";  // Buffer to store received data


int calculateLengthWithoutCommas(const String& dataStr) {
    int length = 0;
    for (char c : dataStr) {
        if (c != ',') {
            length++;
        }
    }
    return length;
}

// Callback function for ESP-NOW message reception
void onReceive(const esp_now_recv_info* sender, const uint8_t* data, int len) {
    String packet = "";
    
    // Convert the received data to a string
    for (int i = 0; i < len; i++) {
        packet += (char)data[i];
    }

    // Append the received packet to the receivedData string
    receivedData += packet;
    
    // Check if the packet is the last one
    if (packet.endsWith("*")) {
        // Remove the last asterisk
        receivedData.remove(receivedData.length() - 1);
        
        // Print the complete received data
        Serial.println("Data received: " + receivedData);

        // Print the length of the received data without the asterisk
         int data = calculateLengthWithoutCommas(receivedData);
         Serial.print("Ukuran data ");
            Serial.println(data);
        
        // Clear the buffer for the next reception
        receivedData = "";
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);  // Initial delay

    // Initialize WiFi
    WiFi.mode(WIFI_STA);
    
    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW initialization failed");
        return;
    }

    // Register the receive callback
    esp_now_register_recv_cb(onReceive);
    
    // Print the MAC address of the receiver
    Serial.print("Receiver MAC Address: ");
    Serial.println(WiFi.macAddress());
}

void loop() {
    // Nothing to do here, waiting for incoming packets
}
