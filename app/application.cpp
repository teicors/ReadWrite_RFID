/* 
 * MFRC522 - Library to use ARDUINO RFID MODULE KIT 13.56 MHZ WITH TAGS SPI W AND R BY COOQROBOT.
 * The library file MFRC522.h has a wealth of useful info. Please read it.
 * The functions are documented in MFRC522.cpp.
 *
 * Based on code Dr.Leong   ( WWW.B2CQSHOP.COM )
 * Created by Nicola Pison (webingenerale.com), Jen 2014.
 * Released into the public domain.
 *
/*
*	Some example code how to copy a card with known keys
*	Uses MFRC522 - Library to use ARDUINO RFID MODULE KIT 13.56 MHZ WITH TAGS SPI W AND R BY COOQROBOT. 
*	by ebc ( 2014) http://ebc81.wordpress.com/
*   V0.1
*-----------------------------------------------------------------------------
* Pin layout should be as follows:
* Signal     Pin              Pin               Pin
*            Arduino Uno#     Arduino Mega      MFRC522 board
* ------------------------------------------------------------
* Reset      9                5                 RST
* SPI SS     10               53                SDA
* SPI MOSI   11               51                MOSI
* SPI MISO   12               50                MISO
* SPI SCK    13               52                SCK
*
*  #Note: Code will work not with Arduino UNO because of not enough SRAM 
*		  Code will work with Mifare Classic 1K Card compatibles, for other code changes in code may necessary
*/
#include <SPI.h>
#include <rfid/MFRC522.h>
 
#include <user_config.h>
#include <SmingCore/SmingCore.h>

#define SS_PIN 5  // d1
#define RST_PIN 4 // d2
 

MFRC522 mfrc522(SS_PIN, RST_PIN);        // Create MFRC522 instance.
Timer procTimer;

MFRC522::MIFARE_Key key;


/**
 * Helper routine to dump a byte array as hex values to Serial.
 */
void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        Serial.print(buffer[i], HEX);
    }
}


/**
 * Main loop.
 */
void RfidCopy() {
    // Look for new cards
    if ( ! mfrc522.PICC_IsNewCardPresent())
        return;

    // Select one of the cards
    if ( ! mfrc522.PICC_ReadCardSerial())
        return;

    // Show some details of the PICC (that is: the tag/card)
    Serial.print("Card UID:");
    dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
    Serial.println();
    Serial.print("PICC type: ");
    byte piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
//    Serial.println(mfrc522.PICC_GetTypeName(piccType));

    // Check for compatibility
    if (    piccType != MFRC522::PICC_TYPE_MIFARE_MINI
        &&  piccType != MFRC522::PICC_TYPE_MIFARE_1K
        &&  piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
        Serial.println("This sample only works with MIFARE Classic cards.");
        return;
    }

    // In this sample we use the second sector,
    // that is: sector #1, covering block #4 up to and including block #7
    byte sector         = 1;
    byte blockAddr      = 4;
    byte dataBlock[]    = {
        0x01, 0x02, 0x03, 0x04, //  1,  2,   3,  4,
        0x05, 0x06, 0x07, 0x08, //  5,  6,   7,  8,
        0x08, 0x09, 0xff, 0x0b, //  9, 10, 255, 12,
        0x0c, 0x0d, 0x0e, 0x0f  // 13, 14,  15, 16
    };
    byte trailerBlock   = 7;
    byte status;
    byte buffer[18];
    byte size = sizeof(buffer);

    // Authenticate using key A
    Serial.println("Authenticating using key A...");
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print("PCD_Authenticate() failed: ");
//        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }

    // Show the whole sector as it currently is
    Serial.println("Current data in sector:");
    mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
    Serial.println();

    // Read data from the block
    Serial.print("Reading data from block "); Serial.print(blockAddr);
    Serial.println(" ...");
    status = mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.print("MIFARE_Read() failed: ");
//        Serial.println(mfrc522.GetStatusCodeName(status));
    }
    Serial.print("Data in block "); Serial.print(blockAddr); Serial.println(":");
    dump_byte_array(buffer, 16); Serial.println();
    Serial.println();

    // Authenticate using key B
    Serial.println("Authenticating again using key B...");
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print("PCD_Authenticate() failed: ");
//        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }

    // Write data to the block
    Serial.print("Writing data into block "); Serial.print(blockAddr);
    Serial.println(" ...");
    dump_byte_array(dataBlock, 16); Serial.println();
    status = mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
    if (status != MFRC522::STATUS_OK) {
        Serial.print("MIFARE_Write() failed: ");
//        Serial.println(mfrc522.GetStatusCodeName(status));
    }
    Serial.println();

    // Read data from the block (again, should now be what we have written)
    Serial.print("Reading data from block "); Serial.print(blockAddr);
    Serial.println(" ...");
    status = mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.print("MIFARE_Read() failed: ");
//        Serial.println(mfrc522.GetStatusCodeName(status));
    }
    Serial.print("Data in block "); Serial.print(blockAddr); Serial.println(":");
    dump_byte_array(buffer, 16); Serial.println();

    // Check that data in block is what we have written
    // by counting the number of bytes that are equal
    Serial.println("Checking result...");
    byte count = 0;
    for (byte i = 0; i < 16; i++) {
        // Compare buffer (= what we've read) with dataBlock (= what we've written)
        if (buffer[i] == dataBlock[i])
            count++;
    }
    Serial.print("Number of bytes that match = "); Serial.println(count);
    if (count == 16) {
        Serial.println("Success :-)");
    } else {
        Serial.println("Failure, no match :-(");
        Serial.println("  perhaps the write didn't work properly...");
    }
    Serial.println();

    // Dump the sector data
    Serial.println("Current data in sector:");
    mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
    Serial.println();

    // Halt PICC
    mfrc522.PICC_HaltA();
    // Stop encryption on PCD
    mfrc522.PCD_StopCrypto1();

}



/**
 * Initialize.
 */
void init() {

    SPI.begin();        // Init SPI bus
    mfrc522.PCD_Init(); // Init MFRC522 card

    // Prepare the key (used both as key A and as key B)
    // using FFFFFFFFFFFFh which is the default at chip delivery from the factory
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }

    Serial.println("Scan a MIFARE Classic PICC to demonstrate read and write.");
    Serial.print("Using key (for A and B):");
    dump_byte_array(key.keyByte, MFRC522::MF_KEY_SIZE);
    Serial.println();

    Serial.println("BEWARE: Data will be written to the PICC, in sector #1");
    
    procTimer.initializeMs(100, RfidCopy).start();    
}




