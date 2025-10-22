/*
  SD card test for ESP32
   
 This example shows how to get info about your SD card on ESP32.
 Very useful for testing a card when you're not sure whether it's working or not.
     
 The circuit:
 * SD card attached to SPI bus as follows:
 ** ESP32: MOSI - GPIO23, MISO - GPIO19, CLK - GPIO18, CS - configurable (default GPIO5)
 
 Modified for ESP32 compatibility


It writes a a text file to the SD card and reads it later to check-.
 */

#include "main.h"
#include "config.h"


void perform_SD_Test(){
  if (!SD.begin(SD_CS)) {
    Serial.println("Card Mount Failed!");
    Serial.println("Things to check:");
    Serial.println("* Is a card inserted?");
    Serial.println("* Is your wiring correct?");
    Serial.println("* Did you change the chipSelect pin to match your setup?");
    Serial.println("* Is the card formatted as FAT32?");
    while (true);
  }

  Serial.println("initialization done.");

  // USE ABSOLUTE PATH with leading slash - IMPORTANT FOR ESP32
  File myFile = SD.open("/test.txt", FILE_WRITE);

  if (myFile) {
    Serial.print("Writing to test.txt...");
    myFile.println("testing 1, 2, 3, Das ist ein Super Test. Hurra, Hurra.");
    myFile.close();
    Serial.println("done.");
  } else {
    Serial.println("error opening test.txt for writing");
  }

  // USE ABSOLUTE PATH for reading too
  myFile = SD.open("/test.txt");
  if (myFile) {
    Serial.println("test.txt:");
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    myFile.close();
  } else {
    Serial.println("error opening test.txt for reading");
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  
  Serial.println("\nESP32 SD Card Test");
  Serial.println("==================");

  // Initialize SPI with custom pins (optional - ESP32 can use default pins)
  SPI.setFrequency(4000000);
  //SPI.begin(SCK, MISO, MOSI, SD_CS);
  SPI.begin(); // This does work just fine.

  perform_SD_Test();
}

void loop() {
  delay(1000); 
}

/*
// Function to list directory contents
void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if(!root){
    Serial.println("Failed to open directory");
    return;
  }
  if(!root.isDirectory()){
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while(file){
    if(file.isDirectory()){
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if(levels){
        listDir(fs, file.path(), levels -1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

// Function to read a file
void readFile(fs::FS &fs, const char * path) {
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if(!file){
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("File content: ");
  while(file.available()){
    Serial.write(file.read());
  }
  Serial.println();
  file.close();
}

// Function to write a file
void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\n", path);

  // Add leading slash if not present
  String fullPath = String(path);
  if (!fullPath.startsWith("/")) {
    fullPath = "/" + fullPath;
    Serial.printf("Corrected path to: %s\n", fullPath.c_str());
  }

  // Check if SD card is still mounted
  if (SD.cardType() == CARD_NONE) {
    Serial.println("SD card not detected!");
    return;
  }

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("File written successfully");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}


void print_card_info(){
  // Get card type
  uint8_t cardType = SD.cardType();
  print_cardType(cardType);

  // Get card size
  uint64_t cardSize = SD.cardSize();
  Serial.printf("SD Card Size: %llu bytes\n", cardSize);
  Serial.printf("SD Card Size: %.2f MB\n", (float)cardSize / (1024 * 1024));
  Serial.printf("SD Card Size: %.2f GB\n", (float)cardSize / (1024 * 1024 * 1024));

  // Get total space and used space
  uint64_t totalBytes = SD.totalBytes();
  uint64_t usedBytes = SD.usedBytes();
  uint64_t freeBytes = totalBytes - usedBytes;
  
  Serial.printf("Total space: %llu bytes (%.2f MB)\n", totalBytes, (float)totalBytes / (1024 * 1024));
  Serial.printf("Used space: %llu bytes (%.2f MB)\n", usedBytes, (float)usedBytes / (1024 * 1024));
  Serial.printf("Free space: %llu bytes (%.2f MB)\n", freeBytes, (float)freeBytes / (1024 * 1024));

  // List files in root directory
  Serial.println("\nFiles found on the card:");
  Serial.println("========================");
  listDir(SD, "/", 0);
}
*/