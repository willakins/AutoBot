#include "mbed.h"

#define NUM_SENSORS 5  // Number of sensors in the array

// Motor control pins
PwmOut leftMotorSpeed(PWM1);
PwmOut rightMotorSpeed(PWM2);
DigitalOut leftMotorDir(DIR1);
DigitalOut rightMotorDir(DIR2);

// Line sensor inputs (analog readings)
AnalogIn sensors[NUM_SENSORS] = {A0, A1, A2, A3, A4};

// PID Parameters (tune these experimentally)
float Kp = 0.5;  // Proportional gain
float baseSpeed = 0.4;  // Base motor speed (0-1)

// Function to read line sensor values
int getLinePosition() {
    float weights[NUM_SENSORS] = {-2, -1, 0, 1, 2};  // Assign weight to each sensor
    float sum = 0, total = 0;
    
    for (int i = 0; i < NUM_SENSORS; i++) {
        float value = 1.0 - sensors[i].read();  // Invert reading (black = high, white = low)
        sum += value * weights[i];
        total += value;
    }
    
    if (total == 0) return 0;  // If no sensor detects the line, return center
    return sum / total;  // Return weighted position of the line
}

// Main loop
int main() {
    while (true) {
        int position = getLinePosition();  // Get the line position

        float correction = Kp * position;  // Calculate correction value
        float leftSpeed = baseSpeed - correction;
        float rightSpeed = baseSpeed + correction;

        // Set motor speeds
        leftMotorSpeed.write(fmax(0, fmin(1, leftSpeed)));
        rightMotorSpeed.write(fmax(0, fmin(1, rightSpeed)));

        wait(0.01);  // Small delay for stability
    }
}
