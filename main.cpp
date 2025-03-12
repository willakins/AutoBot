#include "mbed.h"

// Motor control pins
PwmOut leftMotorSpeed(PWM1);
PwmOut rightMotorSpeed(PWM2);
DigitalOut leftMotorDir(DIR1);
DigitalOut rightMotorDir(DIR2);

// Line sensor inputs
AnalogIn sensors[5] = {A0, A1, A2, A3, A4};

// DIP Switch for speed selection
DigitalIn dipSwitch1(D2); // Speed control: Low/High
DigitalIn dipSwitch2(D3); // Additional tuning or features

// RGB LED for status indication
DigitalOut ledRed(D5);
DigitalOut ledGreen(D6);

// TOF Sensor (Simulated for simplicity)
DigitalIn obstacleDetected(D4); // Replace with real I2C-based sensor

// PID control parameters
float Kp = 0.4;
float Ki = 0.02;
float Kd = 0.1;
float integral = 0, previousError = 0;

// Default speed settings (adjustable via DIP switch)
float baseSpeed = 0.4;
float speedLow = 0.3;  // Slow speed
float speedHigh = 0.6; // High speed

// Function to get the position of the line
int getLinePosition() {
    float weights[5] = {-2, -1, 0, 1, 2};
    float sum = 0, total = 0;

    for (int i = 0; i < 5; i++) {
        float value = 1.0 - sensors[i].read();  // Invert reading (black = high, white = low)
        sum += value * weights[i];
        total += value;
    }

    if (total == 0) return 100; // No line detected
    return sum / total;
}

int main() {
    while (true) {
        // Read DIP switch values
        bool isHighSpeed = dipSwitch1.read();
        bool extraFeature = dipSwitch2.read();  // Can be used for debugging or alternate modes

        // Set speed based on DIP switch
        baseSpeed = isHighSpeed ? speedHigh : speedLow;

        // Get line position
        int position = getLinePosition();

        // PID control calculation
        float error = position;
        integral += error;
        float derivative = error - previousError;
        float correction = Kp * error + Ki * integral + Kd * derivative;
        previousError = error;

        // Adjust motor speeds
        float leftSpeed = baseSpeed - correction;
        float rightSpeed = baseSpeed + correction;

        // Constrain motor speeds
        leftMotorSpeed.write(fmax(0, fmin(1, leftSpeed)));
        rightMotorSpeed.write(fmax(0, fmin(1, rightSpeed)));

        // Check for obstacles (TOF sensor)
        if (obstacleDetected.read()) {
            leftMotorSpeed.write(0);
            rightMotorSpeed.write(0);
            ledRed = 1;  // Turn on Red LED (obstacle detected)
        } else {
            ledRed = 0;
        }

        // End-of-line detection
        if (position == 100) {  
            ledGreen = 1;  // Turn on Green LED
            leftMotorSpeed.write(0);
            rightMotorSpeed.write(0);
        } else {
            ledGreen = 0;
        }

        wait(0.01);
    }
}
