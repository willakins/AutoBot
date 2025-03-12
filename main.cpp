#include "mbed.h"
#include "SDFileSystem.h"  // To handle SD card
#include "wave_player.h"   // Assuming you have a wave player library for sound

// Motor and sensor pin definitions
PwmOut motorLeftSpeed(PA_0);      // Left motor speed control (PWM)
PwmOut motorRightSpeed(PA_1);     // Right motor speed control (PWM)
DigitalOut motorLeftDirection(PA_2); // Left motor direction control
DigitalOut motorRightDirection(PA_3); // Right motor direction control

AnalogIn lineSensorLeft(PA_4);     // Left line sensor (analog)
AnalogIn lineSensorRight(PA_5);    // Right line sensor (analog)

DigitalIn button(SW2);             // Button to start/stop movement
DigitalIn dipSwitch(D0);           // DIP switch for speed control (adjust motor voltage)
Serial lidarSensor(D1, D0);        // LiDAR sensor for obstacle detection (RX, TX)

// Speaker and SD card initialization
PwmOut speaker(PB_0);             // Speaker control
SDFileSystem sd(PF_0, PF_1, PF_3, PF_2, "sd"); // SD card on pins PF_0, PF_1, PF_2, PF_3
WavePlayer wavPlayer;             // Assuming a wave player class to play .wav files

// States for line following and obstacle detection
enum RobotState { IDLE, FOLLOW_LINE, STOPPED, REVERSE, END_OF_LINE };
RobotState currentState = IDLE;

// PID controller parameters
float Kp = 0.5, Ki = 0.1, Kd = 0.05;
float previous_error = 0, integral = 0;

// Speed control via DIP switch
float baseSpeed = 0.5; // Base speed for the motors (adjusted by DIP switch)

void init_motor_control() {
    motorLeftSpeed.period_ms(20);   // PWM period for motor speed
    motorRightSpeed.period_ms(20);  // PWM period for motor speed
    motorLeftDirection = 0;         // Default left motor direction forward
    motorRightDirection = 0;        // Default right motor direction forward
}

void set_motor_speed(float leftSpeed, float rightSpeed) {
    motorLeftSpeed = leftSpeed;
    motorRightSpeed = rightSpeed;
}

void line_following() {
    float leftSensor = lineSensorLeft.read();  // Read line sensor value (0-1)
    float rightSensor = lineSensorRight.read();  // Read line sensor value (0-1)
    float error = leftSensor - rightSensor;

    integral += error;
    float derivative = error - previous_error;

    float output = Kp * error + Ki * integral + Kd * derivative;

    set_motor_speed(baseSpeed - output, baseSpeed + output); // Adjust motor speed based on error

    previous_error = error;

    // Check if we've reached the end of the line (both sensors detect white)
    if (leftSensor > 0.7 && rightSensor > 0.7) {
        currentState = END_OF_LINE;  // Transition to the END_OF_LINE state
    }
}

void lidar_obstacle_detection() {
    // Pseudo-code for reading data from LiDAR sensor
    int distance = lidarSensor.read();  // Replace with actual LiDAR API call

    if (distance < 30) { // Obstacle detected within 30 cm
        currentState = STOPPED;
        play_stop_sound();  // Play honk sound when stopped
    }
}

void play_tone() {
    speaker.period(1.0 / 1000);   // Frequency for tone (e.g., 1 kHz)
    speaker = 0.5;                // Play tone at 50% duty cycle
}

void play_stop_sound() {
    // Play sound from .wav file when the robot stops due to an obstacle
    wavPlayer.play("sd:/stop_sound.wav"); // Play sound from SD card
}

void reverse() {
    motorLeftDirection = 1; // Reverse left motor
    motorRightDirection = 1; // Reverse right motor
    set_motor_speed(baseSpeed, baseSpeed);
}

void stop() {
    set_motor_speed(0.0f, 0.0f); // Stop both motors
}

void play_end_of_line_tone() {
    speaker.period(1.0 / 500);   // Frequency for end-of-line tone (e.g., 500 Hz)
    speaker = 0.5;                // Play tone at 50% duty cycle for end of line
}

void check_button_press() {
    if (button) {
        if (currentState == IDLE) {
            currentState = FOLLOW_LINE;
        } else if (currentState == FOLLOW_LINE) {
            currentState = REVERSE;
        } else if (currentState == REVERSE) {
            currentState = FOLLOW_LINE;
        }
    }
}

void adjust_speed_with_DIP() {
    // Adjust speed based on DIP switch setting
    if (dipSwitch) {
        baseSpeed = 0.8;  // Increase speed if DIP switch is ON
    } else {
        baseSpeed = 0.5;  // Default speed
    }
}

int main() {
    init_motor_control();
    speaker = 0.0f;  // No sound initially

    while (true) {
        check_button_press();
        adjust_speed_with_DIP();

        switch (currentState) {
            case IDLE:
                stop();
                break;

            case FOLLOW_LINE:
                line_following();
                lidar_obstacle_detection();
                break;

            case STOPPED:
                stop();
                play_stop_sound();  // Play honk sound when the robot stops due to obstacle
                break;

            case REVERSE:
                reverse();
                lidar_obstacle_detection();
                break;

            case END_OF_LINE:
                stop();  // Stop the robot when the end of the line is reached
                play_end_of_line_tone();  // Play tone when the end of the line is reached
                currentState = IDLE;  // Reset to IDLE state after reaching the end of the line
                break;
        }
        
        wait_us(1000);  // Small delay to prevent excessive CPU usage
    }
}
