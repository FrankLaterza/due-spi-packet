#include <SPI.h>

#define START_DELIMITER 0xAA
#define END_DELIMITER 0x55
#define MAX_BYTES 85
#define SPI_PACKET_BYTE_SIZE 83
#define NUM_FLOATS 19
#define NUM_BYTES 7
#define SS 10

// Struct of Eu
struct SpiTelePacket {
    float commandVoltage;
    float sinePos;
    float sineNeg;
    float currentPos;
    float currentNeg;
    float calibrationVoltsPos;
    float calibrationVoltsNeg;
    float voltageSinePosPeak;
    float voltageSineNegPeak;
    float currentPosPeak;
    float currentNegPeak;
    float calibrationVoltsPosPeak;
    float calibrationVoltsNegPeak;
    float RTD;
    float NTC3;
    float NTC4;
    float VIN;
    float wattsOut;
    float gainOut;
    uint8_t error;
    uint8_t errorCount;
    uint8_t missedData;
    uint8_t extra1;
    uint8_t extra2;
    uint8_t extra3;
    uint8_t extra4;
};
struct SpiTelePacket spiTelePacket;

// We can set the float and have access to the
// Bytes by splitting them into uint8_t array
union SpiTelePacketUnion {
    struct SpiTelePacket spiTelePacket;
    uint8_t bytes[SPI_PACKET_BYTE_SIZE];
    float floats[NUM_FLOATS];
};
union SpiTelePacketUnion spiTelePacketUnion = {};

// Spi states.
enum SpiTask { START, RECEIVE, END };
enum SpiTask spiState = START;
// The main iterator for spi
int spiIt = 0;
// The Receive Array
uint8_t spiRecPacket[SPI_PACKET_BYTE_SIZE] = { 0 };
// Receive Variable;
uint8_t rec = 0;
uint8_t checksum = 0;

// Access data from bytes or by struct
void printPacketFromUnion() {
    // Print the floats
    for (int i = 0; i < NUM_FLOATS; i++) {
        Serial.println(spiTelePacketUnion.floats[i]);
    }

    // Print the bytes
    for (int i = SPI_PACKET_BYTE_SIZE - NUM_BYTES; i < SPI_PACKET_BYTE_SIZE; i++) {
        Serial.println(spiTelePacketUnion.bytes[i]);
    }
}

void printPacketFromStruct() {
    // Put the union into the struct (makes copy)
    spiTelePacket = spiTelePacketUnion.spiTelePacket;

    // Print out the struct
    Serial.println("SpiTelePacket Values:");
    Serial.print("Command Voltage: ");
    Serial.println(spiTelePacket.commandVoltage);
    Serial.print("Sine Positive: ");
    Serial.println(spiTelePacket.sinePos);
    Serial.print("Sine Negative: ");
    Serial.println(spiTelePacket.sineNeg);
    Serial.print("Current Positive: ");
    Serial.println(spiTelePacket.currentPos);
    Serial.print("Current Negative: ");
    Serial.println(spiTelePacket.currentNeg);
    Serial.print("Calibration Volts Positive: ");
    Serial.println(spiTelePacket.calibrationVoltsPos);
    Serial.print("Calibration Volts Negative: ");
    Serial.println(spiTelePacket.calibrationVoltsNeg);
    Serial.print("Voltage Sine Positive Peak: ");
    Serial.println(spiTelePacket.voltageSinePosPeak);
    Serial.print("Voltage Sine Negative Peak: ");
    Serial.println(spiTelePacket.voltageSineNegPeak);
    Serial.print("Current Positive Peak: ");
    Serial.println(spiTelePacket.currentPosPeak);
    Serial.print("Current Negative Peak: ");
    Serial.println(spiTelePacket.currentNegPeak);
    Serial.print("Calibration Volts Positive Peak: ");
    Serial.println(spiTelePacket.calibrationVoltsPosPeak);
    Serial.print("Calibration Volts Negative Peak: ");
    Serial.println(spiTelePacket.calibrationVoltsNegPeak);
    Serial.print("RTD Temperature: ");
    Serial.println(spiTelePacket.RTD);
    Serial.print("NTC3 Temperature: ");
    Serial.println(spiTelePacket.NTC3);
    Serial.print("NTC4 Temperature: ");
    Serial.println(spiTelePacket.NTC4);
    Serial.print("VIN: ");
    Serial.println(spiTelePacket.VIN);
    Serial.print("Watts Out: ");
    Serial.println(spiTelePacket.wattsOut);
    Serial.print("Gain Out: ");
    Serial.println(spiTelePacket.gainOut);
    Serial.print("Error: ");
    Serial.println(spiTelePacket.error);
    Serial.print("Error Count: ");
    Serial.println(spiTelePacket.errorCount);
    Serial.print("Missed Data Count: ");
    Serial.println(spiTelePacket.missedData);
    Serial.print("Extra Byte 1: ");
    Serial.println(spiTelePacket.extra1);
    Serial.print("Extra Byte 2: ");
    Serial.println(spiTelePacket.extra2);
    Serial.print("Extra Byte 3: ");
    Serial.println(spiTelePacket.extra3);
    Serial.print("Checksum: ");
    Serial.println(spiTelePacket.extra4);
}

void printPacketFromStructBinary() {
    // Put the union into the struct (makes copy)
    spiTelePacket = spiTelePacketUnion.spiTelePacket;
    Serial.write(uint8_t(START_DELIMITER));
    for (int i = 0; i < SPI_PACKET_BYTE_SIZE; i++) {
        Serial.write(uint8_t(spiTelePacketUnion.bytes[i]));
    }
    Serial.write(uint8_t(END_DELIMITER));
} 

// Setup
void setup() {
    Serial.begin(9600);
    SPI.begin(SS);
    SPI.setDataMode(SS, 2);
    SPI.setClockDivider(SS, 21);  // 4MHz
}

// Main
void loop() {
    switch (spiState) {
    case START:
        // Reset checksum
        checksum = 0;
        // Look for start character
        rec = SPI.transfer(SS, 0, SPI_CONTINUE);
        if (rec == START_DELIMITER) {
            // Serial.println("start");
            spiState = RECEIVE;
        } else {
            // Error has occurred. To reset inverter board spi
            // routine allow timeout or consider feeding until
            Serial.print("error start not found: ");
            Serial.println(rec);
            Serial.println("delay for timeout");
            delay(1000);
            spiState = START;
        }
        break;
    case RECEIVE:
        // Get the data
        rec = SPI.transfer(SS, 0);
        // Store byte
        spiTelePacketUnion.bytes[spiIt] = rec;
        // Print
        // Serial.print(spiIt + 1);
        // Serial.print(") ");
        // Serial.println((uint8_t)rec);
        // Move state at end of packet
        if (spiIt >= SPI_PACKET_BYTE_SIZE - 1) {
            if (checksum == rec) {
                // Serial.println("Checksum match");
            }
            else {
                Serial.print("Checksum mismatch. Found: ");
                Serial.print(rec);
                Serial.print(", Recieved: ");
                Serial.println(checksum);
            }
            spiState = END;
            break;
        }
        // add checksum
        checksum += rec;
        // Increment
        spiIt++;
        break;
    case END:
        // Reset checksum
        checksum = 0;
        rec = SPI.transfer(SS, 0);
        if (rec != END_DELIMITER) {
            // Error has occurred. To reset inverter board spi
            // routine allow timeout.
            Serial.print("error end not found: ");
            Serial.println(rec);
            Serial.println("delay for spi reset");
            spiIt = 0;
            spiState = START;
            delay(1000);
        } else {
            spiIt = 0;
            spiState = START;
            // Serial.println("end");
            // printPacketFromStruct();
            printPacketFromStructBinary();
            delay(5000);
        }
        break;
    }

    delay(50);

}
