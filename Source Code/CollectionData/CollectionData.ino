#include <SD.h>
#include <SPI.h>
#include <DHT.h>

#define SD_CS 5     // Pin CS untuk kartu SD
#define DHTPIN 4    // Pin data untuk DHT22
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

const char* filename = "/dht22.txt";
unsigned long startTime;
const unsigned long duration = 120000; 

void setup() {
  Serial.begin(115200);
  dht.begin();
 
  if (!SD.begin(SD_CS)) {
    Serial.println("Inisialisasi kartu SD gagal!");
    return;
  }
  Serial.println("Inisialisasi kartu SD berhasil.");
  startTime = millis();
  Serial.println("Mulai mengumpulkan data selama 1 menit...");
}

void loop() {
  unsigned long currentTime = millis();
  if (currentTime - startTime < duration) {
    float temperature = dht.readTemperature();

    if (isnan(temperature)) {
      Serial.println("Gagal membaca dari sensor DHT22!");
      return;
    }

    File dataFile = SD.open(filename, FILE_APPEND);  
    if (dataFile) {
      dataFile.println(temperature);
      dataFile.close();
      
      Serial.print("Data tersimpan: ");
      Serial.print(currentTime - startTime);
      Serial.print("ms, Temp: ");
      Serial.print(temperature);
      Serial.println("C");
    } else {
      Serial.println("Error membuka file!");
    }
    delay(100);
  } else if (currentTime - startTime >= duration) {
    Serial.println("Pengumpulan data selesai.");
    while(1); 
  }
}
