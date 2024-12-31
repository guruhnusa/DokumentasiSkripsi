#include <Arduino.h>
#include <map>
#include <vector>
#include <string>
#include <esp_now.h>
#include <WiFi.h>

std::map<std::string, float> huffmanCodes;
std::string dataBuffer; // Buffer untuk menampung data yang diterima
const std::string endDelimiter = ";;"; // Tanda bahwa data sudah lengkap

// Fungsi untuk melakukan dekompresi data
void decompressData(const std::string& compressedData, const std::map<std::string, float>& huffmanCodes) {
    std::vector<float> decompressedData;
    std::string buffer;

    for (char bit : compressedData) {
        buffer += bit;

        // Cek apakah buffer saat ini adalah kode Huffman yang valid
        auto it = huffmanCodes.find(buffer);
        if (it != huffmanCodes.end()) {
            decompressedData.push_back(it->second); // Simpan data terdekompresi ke dalam vector
            buffer.clear(); // Bersihkan buffer setelah menemukan kode
        }
    }

    // Tampilkan data hasil dekompresi
    Serial.print("Data hasil dekompresi: ");
    for (float num : decompressedData) {
        Serial.print(num, 2); // Format angka dengan 2 desimal
        Serial.print(" ");
    }
    Serial.println();

    String suhuDataStr = "";
    

    // Konversi tiap elemen float dalam suhuData ke string dan tambahkan ke suhuDataStr
    for (float suhu : decompressedData) {
        suhuDataStr += String(suhu, 2);
    }

    // Hitung ukuran data dalam string (tanpa koma)
    int dataLength = 0;
    for (int i = 0; i < suhuDataStr.length(); i++) {
        if (suhuDataStr[i] != ',') {
            dataLength++;
        }
    }

    Serial.print("Ukuran data setelah dekompresi: ");
    Serial.print(dataLength);
    Serial.println(" bytes");
}

// Fungsi untuk memproses data yang diterima ketika sudah lengkap
void processReceivedData(const std::string& fullData) {
    // Pisahkan data hasil kompresi dan kode Huffman
    size_t delimiterPos = fullData.find(';');
    if (delimiterPos == std::string::npos) {
        Serial.println("Delimiter not found in received data.");
        return;
    }
    std::string compressedData = fullData.substr(0, delimiterPos);
    std::string huffmanCodeData = fullData.substr(delimiterPos + 1);

    // Check for the end marker
    if (huffmanCodeData.back() == '*') {
        huffmanCodeData.pop_back(); // Remove the end marker
    }

    // Parse kode Huffman
    huffmanCodes.clear();
    size_t pos = 0;
    Serial.println("Kode Huffman diterima:");
    while ((pos = huffmanCodeData.find(';')) != std::string::npos) {
        std::string code = huffmanCodeData.substr(0, pos);
        size_t colonPos = code.find(':');
        if (colonPos == std::string::npos) {
            Serial.println("Invalid Huffman code format.");
            return;
        }
        float character = std::stof(code.substr(0, colonPos)); // Use std::stof for float
        std::string huffmanCode = code.substr(colonPos + 1);
        huffmanCodes[huffmanCode] = character;
        huffmanCodeData.erase(0, pos + 1);
        Serial.print("Karakter: ");
        Serial.print(character, 2); // Format angka dengan 2 desimal
        Serial.print(", Kode: ");
        Serial.println(huffmanCode.c_str());
    }

    // Dekode data
    decompressData(compressedData, huffmanCodes);
}

// Callback untuk menerima data
void onDataReceive(const esp_now_recv_info* info, const uint8_t* data, int len) {
    // Tambahkan data yang diterima ke buffer
    dataBuffer.append(reinterpret_cast<const char*>(data), len);

    // Periksa apakah data yang diterima sudah lengkap dengan mendeteksi tanda akhir data
    if (dataBuffer.find(endDelimiter) != std::string::npos || dataBuffer.find('*') != std::string::npos) {
        // Jika tanda akhir ditemukan, kita hapus tanda tersebut
        size_t endPos = dataBuffer.find(endDelimiter);
        if (endPos == std::string::npos) {
            endPos = dataBuffer.find('*'); // Check for end marker
        }
        std::string fullData = dataBuffer.substr(0, endPos);

        // Proses data yang sudah lengkap
        Serial.print("Data lengkap diterima: ");
        Serial.println(fullData.c_str());

        processReceivedData(fullData);

        // Bersihkan dataBuffer setelah data diproses
        dataBuffer.clear();
    }
}

void setup() {
    Serial.begin(115200);

    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW initialization failed");
        return;
    }

    // Daftarkan callback penerima data
    esp_now_register_recv_cb(onDataReceive);
}

void loop() {
    // Tidak ada kode di sini
}
