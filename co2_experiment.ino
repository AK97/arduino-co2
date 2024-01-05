#include <LiquidCrystal.h>
#include <SoftwareSerial.h>
#include <SD.h>
#include <SPI.h>

// Establish which pin is connected to what
#define SERIAL_A_RX A12 // Sensor A is furthest from the breadboard
#define SERIAL_A_TX A13
#define SERIAL_B_RX A14
#define SERIAL_B_TX A15
#define SERIAL_C_RX A8
#define SERIAL_C_TX A9
#define SERIAL_D_RX A10 // Sensor D is closest to the breadboard
#define SERIAL_D_TX A11
#define SD_CARD_PIN 53
#define NOISE_PORT A0

// Digital pins for LCD
#define Vo 12
#define Rs 30
#define E 3
#define D4 4
#define D5 32
#define D6 6
#define D7 34

// Global constants
const int BAUD_RATE = 9600;
const int NUM_BYTES = 9;
const int NUM_SENSORS = 4;
const int TIME_DELAY = 500; // Readings taken every half second
const int CONTRAST = 75;

// Initialize sensors
SoftwareSerial SensorA(SERIAL_A_RX, SERIAL_A_TX);
SoftwareSerial SensorB(SERIAL_B_RX, SERIAL_B_TX);
SoftwareSerial SensorC(SERIAL_C_RX, SERIAL_C_TX);
SoftwareSerial SensorD(SERIAL_D_RX, SERIAL_D_TX);
SoftwareSerial *Sensors[NUM_SENSORS] = {&SensorA, &SensorB, &SensorC, &SensorD};

// Initialize LCD
LiquidCrystal LCD(Rs, E, D4, D5, D6, D7);

// SD Data File
File dataFile;
String DATA_FILE_NAME;

// Read gas density command. 9 bytes
unsigned char hexdata[NUM_BYTES] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};

void setup()
{
    Serial.begin(BAUD_RATE);
    while (!Serial)
    {
        // Await active connection to PC
    }
    SensorA.begin(BAUD_RATE);
    SensorB.begin(BAUD_RATE);
    SensorC.begin(BAUD_RATE);
    SensorD.begin(BAUD_RATE);

    pinMode(SD_CARD_PIN, OUTPUT);
    while (!SD.begin(SD_CARD_PIN))
    {
        // Await succcessful SD card startup
        Serial.println("Attempting to startup SD...");
    }
    Serial.println("Connected to SD Card.");

    analogWrite(Vo, CONTRAST);
    LCD.begin(16, 2); // 2 rows of 16 chars
    LCD.print("HELLO NASA!");
    delay(3000);

    // Create file on SD card to hold data for this run
    randomSeed(analogRead(NOISE_PORT));
    long file_id = random(100000, 1000000); // Give this experiment a unique identifier
    DATA_FILE_NAME = String(file_id) + ".csv"; // Note: 8.3 filename enforced
    Serial.println("Data file name: " + DATA_FILE_NAME);

    // Display file name on LCD
    // To identify datasets later with help of video recording
    LCD.setCursor(0, 0);
    LCD.print("Data file name: ");
    LCD.setCursor(0, 1);
    LCD.print(DATA_FILE_NAME);
    delay(3000);

    dataFile = SD.open(DATA_FILE_NAME, FILE_WRITE);

    if (dataFile)
    {
        dataFile.println("Sensor A,Sensor B,Sensor C,Sensor D"); // header
        dataFile.flush();
    }
    else
    {
        Serial.println("Failed to write headers to datafile");
    }
}

void loop()
{
    // Variable to hold the CO2 concentration value at one time point for all
    // sensors
    int CO2[NUM_SENSORS];

    // Read the data from each sensor
    for (int sensor = 0; sensor < NUM_SENSORS; sensor++)
    {
        // Tell the sensor to take a reading
        Sensors[sensor]->listen();
        delay(TIME_DELAY / (NUM_SENSORS * 2)); // Give time to start listening
        Sensors[sensor]->write(hexdata, NUM_BYTES);
        delay(TIME_DELAY / (NUM_SENSORS * 2)); // Give time to write data
        // These two delays total 1 TIME_DELAY per loop() to read all sensors

        // Read one byte at a time
        for (int byte = 0; byte < NUM_BYTES; byte++)
        {
            if (Sensors[sensor]->available())
            {
                int byte_value = Sensors[sensor]->read(); // reads next byte
                int hi, lo;

                switch (byte)
                {
                    case 2:
                        hi = byte_value;
                        break;
                    case 3:
                        lo = byte_value;
                        break;
                    case 8:
                        CO2[sensor] =
                            hi * 256 + lo; // equation provided by sensor documentation
                        break;
                }
            }
        }
    }

    /*
    //// LCD Display Example ////

    //////////////////////////////////////////////////////
    // [3][4][2][1][ ][p][p][m][2][3][9][ ][ ][p][p][m] //
    // [2][8][7][3][ ][p][p][m][1][1][0][4][3][p][p][m] //
    //////////////////////////////////////////////////////

    // Top left: Sensor A
    // Top right: Sensor B
    // Bottom left: Sensor C
    // Bottom right: Sensor D
    */

    // Print the readings to the serial
    for(int i = 0; i < NUM_SENSORS; i++) {
      Serial.print("| " + String(CO2[i]) + " |");
    }
    Serial.println();

    LCD.clear();
     // Display data on LCD
    LCD.setCursor(0, 0);
    LCD.print(round(CO2[0]));
    LCD.setCursor(8, 0);
    LCD.print(round(CO2[1]));
    LCD.setCursor(0, 1);
    LCD.print(round(CO2[2]));
    LCD.setCursor(8, 1);
    LCD.print(round(CO2[3]));
    // Write units
    LCD.setCursor(5, 0);
    LCD.print("ppm");
    LCD.setCursor(13, 0);
    LCD.print("ppm");
    LCD.setCursor(5, 1);
    LCD.print("ppm");
    LCD.setCursor(13, 1);
    LCD.print("ppm");

    // Write data to SD card
    // File dataFile = SD.open(DATA_FILE_NAME, FILE_WRITE);
    String data_line = String(CO2[0]) + "," + String(CO2[1]) + "," + String(CO2[2]) + "," + String(CO2[3]);

    if (dataFile)
    {
        dataFile.println(data_line);
        dataFile.flush();
    }
}