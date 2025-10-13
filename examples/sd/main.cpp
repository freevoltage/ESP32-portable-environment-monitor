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

#include <Arduino.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

#include "main.h"

// Pin definitions for ESP32 (adjust according to your wiring)
#define SD_CS    8    // Chip Select pin - change this to match your wiring
#define SD_MOSI  22   // Master Out Slave In
#define SD_MISO  23   // Master In Slave Out  
#define SD_SCK   21   // Serial Clock

void setup() {
  Serial.begin(115200);
  delay(1000); // Give serial time to initialize
  
  Serial.println("\nESP32 SD Card Test");
  Serial.println("==================");

  // Initialize SPI with custom pins (optional - ESP32 can use default pins)
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  
  Serial.print("Initializing SD card...");

  // Initialize SD card
  if (!SD.begin(SD_CS)) {
    Serial.println("Card Mount Failed!");
    Serial.println("Things to check:");
    Serial.println("* Is a card inserted?");
    Serial.println("* Is your wiring correct?");
    Serial.println("* Did you change the chipSelect pin to match your setup?");
    Serial.println("* Is the card formatted as FAT32?");
    return;
  }
  
  Serial.println("SD card initialized successfully!");

  // Get card type
  uint8_t cardType = SD.cardType();
  Serial.print("\nCard Type: ");
  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    return;
  } else if(cardType == CARD_MMC){
    Serial.println("MMC");
  } else if(cardType == CARD_SD){
    Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

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

  // Test creating a file
  Serial.println("\nTesting file operations...");
  writeFile(SD, "/test.txt", "Hello ESP32 SD Card!");
  readFile(SD, "/test.txt");
  
  Serial.println("\nSD Card test completed!");
}

void loop() {
  // Nothing to do here
  delay(1000);
}

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
