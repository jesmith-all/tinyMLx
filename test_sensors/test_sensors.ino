/*
  Active Learning Labs
  Harvard University 
  tinyMLx - Sensor Test

  Requires the Arduino_LSM9DS1 library and the Arduino_OV767X library
*/

#include <Arduino_LSM9DS1.h>
#include <Arduino_OV767X.h>
#include <PDM.h>

int testIndex = 0; // 0 - IMU, 1 - microphone, 2 - camera
int imuIndex = 0; // 0 - accelerometer, 1 - gyroscope, 2 - magnetometer
bool commandRecv = false; // flag used for indicating receipt of commands from serial port
bool printFlag = false; // flag as true to print sensor data
bool liveFlag = false; // flag as true to live stream raw camera bytes, set as false to take single images on command
bool captureFlag = false;

// PDM buffer
short sampleBuffer[256];
volatile int samplesRead;

// Image buffer;
byte image[176 * 144 * 2]; // QCIF: 176x144 x 2 bytes per pixel (RGB565)
int bytesPerFrame;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // Initialize IMU
  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU");
    while (1);
  }

  // Initialize camera
  // TODO: decide on final camera format
  if (!Camera.begin(QCIF, RGB565, 1)) {
    Serial.println("Failed to initialize camera");
    while (1);
  }
  bytesPerFrame = Camera.width() * Camera.height() * Camera.bytesPerPixel();

  PDM.onReceive(onPDMdata);
  // Initialize PDM microphone in mono mode with 16 kHz sample rate
  if (!PDM.begin(1, 16000)) {
    Serial.println("Failed to start PDM");
    while (1);
  }

  Serial.println("Welcome to the sensor test for the Nano 33 BLE Sense and accompanying OV7675 camera\n");
  Serial.println("Testing IMU, available commands:");
  Serial.println("A - display accelerometer readings in g's in x, y, and z directions");
  Serial.println("G - display gyroscope readings in deg/s in x, y, and z directions");
  Serial.println("M - display magnetometer readings in uT in x, y, and z directions");
  Serial.println("Type \"done\" when you would like to move onto testing the microphone");
}

void loop() {
  int i = 0;
  String command;

  // Read incoming commands from serial monitor
  while (Serial.available()) {
    char c = Serial.read();
    if ((c != '\n') && (c != '\r')) {
      command.concat(c);
    } 
    else if (c == '\r') {
      commandRecv = true;
      command.toLowerCase();
    }
  }

  // Command interpretation
  if (commandRecv) {
    commandRecv = false;
    if (command == "done") {
      switch (++testIndex) {
        case 1: // just completed IMU test, moving onto microphone
          Serial.println("\nIMU testing completed!\n");
          Serial.println("Next test: built-in microphone");
          Serial.println("Close the serial monitor, and open the serial plotter to view the raw waveform");
          Serial.println("When finished testing, re-open the serial monitor and send the \"done\" command");
          Serial.println("\nType \"Start\" to begin streaming data from the microphone");
          printFlag = false;
          break;
        case 2: // just completed the microphone test, moving onto camera
          // Still figuring out exactly how we're going to handle this
          // Options: 1. stream data raw to processing after a short delay, similar to mic test
          //          2. stream data to serial monitor using a command in hex, copy and paste to Python (in a Colab?) to render an image 
          Serial.println("\nMicrophone testing completed!\n");
          Serial.println("Next test: OV7675 camera"); 
          Serial.println("\nAvailable commands:");
          Serial.println("Live - the raw bytes of images will be streamed live over the serial port");
          Serial.println("Single - take a single image and print out the hexadecimal for each pixel (default)");
          Serial.println("Capture - when in single image mode, initiates an image capture");
          printFlag = false;
          break;
        case 3:
          Serial.println("\nAll testing complete!");
          while (1);
          // possibly add a command to re-do testing, or to test a particular component again (e.g. type IMU, mic, cam to test again)?
          break;
        default:
          break;
      }
    } 
    else if (command == "a") {
      if (testIndex == 0) {
        imuIndex = 0;
        Serial.println("\nAccelerometer data will begin streaming in 3 seconds...");
        printFlag = true;
        delay(3000);
      }
    }
    else if (command == "g") {
      if (testIndex == 0) {
        imuIndex = 1;
        Serial.println("\nGyroscope data will begin streaming in 3 seconds...");
        printFlag = true;
        delay(3000);
      }
    }
    else if (command == "m") {
      if (testIndex == 0) {
        imuIndex = 2;
        Serial.println("\nMagnetometer data will begin streaming in 3 seconds...");
        printFlag = true;
        delay(3000);
      }
    }
    else if (command == "start") {
      if (testIndex == 1) {
        Serial.println("\nMicrophone data will begin streaming in 3 seconds...");
        printFlag = true;
        delay(3000);
      }
    }
    else if (command == "live") {
      if (testIndex == 2) {
        Serial.println("\nRaw image data will begin streaming in 5 seconds...");
        printFlag = true;
        liveFlag = true;
        delay(5000);
      }
    }
    else if (command == "single") {
      if (testIndex == 2) {
        Serial.println("\nCamera in single mode, type \"capture\" to initiate an image capture");
        printFlag = true;
        liveFlag = false;
        delay(200);
      }
    }
    else if (command == "capture") {
      if (testIndex == 2) {
        if (!liveFlag) {
          Serial.println("\nImage data will be printed out in 3 seconds...");
          printFlag = true;
          captureFlag = true;
          delay(3000);
        }
        else {
          Serial.println("\nCamera is not in single mode, type \"single\" first");
          delay(1000);
        }
      }
    }
  }

  if (testIndex == 0) { // testing IMU
    float x, y, z;
    
    if (printFlag) {
      if (imuIndex == 0) { // testing accelerometer
        if (IMU.accelerationAvailable()) {
          IMU.readAcceleration(x, y, z);

          Serial.print("Ax: ");
          Serial.print(x);
          Serial.print(" g  ");
          Serial.print("Ay: ");
          Serial.print(y);
          Serial.print(" g  ");
          Serial.print("Az: ");
          Serial.print(z);
          Serial.println(" g");
        }
      }
      else if (imuIndex == 1) { // testing gyroscope
        if (IMU.gyroscopeAvailable()) {
          IMU.readGyroscope(x, y, z);

          Serial.print("wx: ");
          Serial.print(x);
          Serial.print(" deg/s  ");
          Serial.print("wy: ");
          Serial.print(y);
          Serial.print(" deg/s  ");
          Serial.print("wz: ");
          Serial.print(z);
          Serial.println(" deg/s");
        }
      }
      else if (imuIndex == 2) { // testing magnetometer
        if (IMU.magneticFieldAvailable()) {
          IMU.readMagneticField(x, y, z);

          Serial.print("Bx: ");
          Serial.print(x);
          Serial.print(" uT  ");
          Serial.print("By: ");
          Serial.print(y);
          Serial.print(" uT  ");
          Serial.print("Bz: ");
          Serial.print(z);
          Serial.println(" uT");
        }
      }
    }
  }
  else if (testIndex == 1) { // testing microphone
    if (printFlag) {
      // wait for samples to be read
      if (samplesRead) {

        // print samples to serial plotter
        for (int i = 0; i < samplesRead; i++) {
          Serial.println(sampleBuffer[i]);
        }

        // clear read count
        samplesRead = 0;
      }
    }
  }
  else if (testIndex == 2) { // testing camera
    if (printFlag) {
      if (liveFlag) {
        Camera.readFrame(image);
        Serial.write(image, bytesPerFrame);
      }
      else {
        if (captureFlag) {
          captureFlag = false;
          Camera.readFrame(image);
          delay(3000);
          for (int i = 0; i < bytesPerFrame - 1; i += 2) {
            Serial.print("0x");
            Serial.print(image[i+1], HEX);
            Serial.print(image[i], HEX);
            if (i != bytesPerFrame - 2) {
              Serial.print(", ");
            }
          }
          Serial.println();
        }
      }
    }
  }

}

void onPDMdata() {
  // query the number of bytes available
  int bytesAvailable = PDM.available();

  // read data into the sample buffer
  PDM.read(sampleBuffer, bytesAvailable);

  samplesRead = bytesAvailable / 2;
}
