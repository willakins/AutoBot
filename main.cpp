#include "mbed.h"
#include "wave_player.h"
#include "SDBlockDevice.h"
#include "MyFATFileSystem.h"

/**
*       MOTOR PINS
*/
PwmOut motorLeftSpeed(p25);      // Left motor speed control (PWM)
PwmOut motorRightSpeed(p26);     // Right motor speed control (PWM)

DigitalOut motorLeftIn1(p22); // Left motor direction control
DigitalOut motorLeftIn2(P2_1); // Right motor direction control
DigitalOut motorRightIn1(p14); // Left motor direction control
DigitalOut motorRightIn2(p13); // Right motor direction control



/**
*       BUTTON PINS
*/
DigitalIn button(p30);             // Button to start/stop movement
DigitalIn dipSwitch(p29);          // DIP switch for speed control (adjust motor voltage)



/**
*       SENSOR PINS
*/
DigitalOut trigger(p10);
DigitalIn echo(p11);
DigitalIn l1(p20);
DigitalIn l2(p19);
DigitalIn l3(p17);
DigitalIn l4(p16);



/**
*       TIMERS
*/
Timer sonarTimer;
Timer debounceTimer;            // Timer to handle button debounce



/**
*       STATUS PINS
*/
DigitalOut green(p28);
DigitalOut red(p27);
AnalogOut DACout(p18); //speaker


/**
*       FILESYSTEM SETUP
*/
BufferedSerial pc(USBTX, USBRX, 115200); //ls /dev/tty.* then screen /dev/tty.usbmodem102 115200
FILE *fp = fdopen(&pc, "w");
SDBlockDevice sd(p5, p6, p7, p8);  // MOSI, MISO, SCK, CS pins
MyFATFileSystem fs("sd", &sd);
wave_player wavPlayer(&DACout);



/**
*       GLOBAL VARIABLES
*/
#define PI 3.14159265358979323846
#define MIN_DIST 20           // Distance for obstacle detection 20cm
#define DEBOUNCE_DELAY 0.05   // Debounce delay in seconds 50ms
#define THRESHOLD 0.7         // For detecting end of line

volatile float dist = 0.0f;  
bool buttonState = false;       // Current button state
bool lastButtonState = false;   // Previous button state
float baseSpeed = 0.5;     // Base speed for the motors

float Kp = 0.5, Ki = 0.1, Kd = 0.05; // PID controller parameters
float previous_error = 0, integral = 0;

enum RobotState { IDLE, FOLLOW_LINE, OBSTRUCTED, END }; // States for line following and obstacle detection
RobotState currentState = IDLE;

//Files for speaker
string honk_wav = "/sd/honk.wav";
string end_of_line_wav = "/sd/stop.wav";//That sound like when an applicance finishes running



/**
*
*       MOTOR CONTROL
*
*/
void set_motor_speed(float leftSpeed, float rightSpeed) {
    motorLeftSpeed = leftSpeed;
    motorRightSpeed = rightSpeed;
}

void init_motor_control() {
    motorLeftSpeed.period_ms(20);   // PWM period for motor speed
    motorRightSpeed.period_ms(20);  // PWM period for motor speed
    motorLeftIn1 = 0; // Left motor clockwise default
    motorLeftIn2 = 1;
    motorRightIn1 = 0; // Right motor clockwise default
    motorRightIn2 = 1;
    set_motor_speed(0.0f, 0.0f);
}

void stop() {
    set_motor_speed(0.0f, 0.0f); // Stop both motors
}

void turnAround() {
    motorLeftIn1 = 1; // Left motor counter-clockwise
    motorLeftIn2 = 0;
    set_motor_speed(min(baseSpeed * 2, 1), min(baseSpeed * 2, 1));
    wait_us(680000); //Test to see how long it takes to turn 180 degrees
    stop();
    motorLeftIn1 = 0; // Left motor clockwise
    motorLeftIn2 = 1;
}

float clamp(float speed) {
    return max(0, min(1, speed)); //Ensures speeds are [0, 1]
}




/**
*
*       BUTTON CONTROL
*
*/
bool checkButtonState() { // Controls button debouncing
    if (debounceTimer.read_ms() > DEBOUNCE_DELAY * 1000) {  // Check if debounce delay has passed
        bool currentButtonState = button.read();

        // Check if the button state has changed
        if (currentButtonState != lastButtonState) {
            lastButtonState = currentButtonState;
            debounceTimer.reset();  // Reset the debounce timer

            if (currentButtonState == 1) {  // Button pressed
                return true;
            }
        }
    }
    return false;
}

void check_button_press() {
    if (checkButtonState()) {
        if (currentState == IDLE) {
            currentState = FOLLOW_LINE;
        } else if (currentState == FOLLOW_LINE) {
            currentState = IDLE;
        }
    }
}

void check_switch() {
    baseSpeed = dipSwitch ? 1 : 0.5;
}



/**
*
*       SENSOR CONTROL
*
*/
float measureDistance() {
    trigger = 1;
    wait_us(10);
    trigger = 0;
    
    while (echo == 0);
    sonarTimer.start();
    while (echo == 1);
    sonarTimer.stop();
    
    float timeElapsed = sonarTimer.read_us();
    sonarTimer.reset();
    return (timeElapsed * 0.017); // Convert to cm using speed of sound
}

void detect_obstacle() {
    if (measureDistance() < MIN_DIST) {
        currentState = OBSTRUCTED;
    }
}

void line_following() {
    set_motor_speed(baseSpeed, baseSpeed);
    float l1sensor = l1.read();  // returns (0-1)
    float l2sensor = l2.read();
    float l3sensor = l3.read();
    float l4sensor = l4.read();

    // Weighted error calculation, test these
    // Negative means line is toward left, positive means right
    float error = -3 * l1sensor - 1 * l2sensor + 1 * l3sensor + 3 * l4sensor;

    integral += error;
    float derivative = error - previous_error;

    float output = Kp * error + Ki * integral + Kd * derivative;

    set_motor_speed(clamp(baseSpeed - output), clamp(baseSpeed + output));

    previous_error = error;

    // both edge sensors see white so probably reached end of line
    if (l1sensor > THRESHOLD && l4sensor > THRESHOLD) {
        currentState = END;
    }
}



/**
*
*       SPEAKER CONTROL
*
*/
void play_sound(string filename) {
    FILE *waveFile = fopen(filename, "r");
    if (waveFile) {
        wavPlayer.play(waveFile);
        wait_us(500000);
        fclose(waveFile);
    } else {
        fprintf(fp, "Could not open %s!\n", filename.c_str());
    }
}

void play_tone(bool isHonk) {
    float frequency = isHonk ? 300.0f : 1000.0f;  // Low for honk, high for success tone
    float duration = 0.5f;  // Duration in seconds
    float sample_rate = 44100.0f;
    int samples = duration * sample_rate;

    for (int i = 0; i < samples; ++i) {
        float t = (float)i / sample_rate;
        float value = 0.5f + 0.5f * sinf(2.0f * PI * frequency * t);  // Generate sine wave [0,1]
        DACout.write(value);
        wait_us(1); // Delay to prevent overload (very simple timing)
    }

    DACout.write(0.5f);  // Reset to middle
}



/**
*
*       MAIN CODE
*
*/
void init_robot() {
    init_motor_control();
    debounceTimer.start(); 
    lastButtonState = button.read();  // Set the initial state of the button
    buttonState = lastButtonState;    // Make sure we start with the correct state

    if (fs.mount() != 0) { //Mount filesystem
        fprintf(fp, "Failed to mount SD card!\n");
    } else {
        fprintf(fp, "SD card mounted successfully.\n");
    }
}

int main() {
    init_robot();
    
    while (true) {
        check_button_press();
        check_switch();

        switch (currentState) {
            case IDLE:
                stop();
                green = 0;
                break;

            case FOLLOW_LINE:
                detect_obstacle();
                red = 0;
                line_following();
                break;

            case OBSTRUCTED:
                stop();
                red = 1;
                play_tone(true);
                //play_sound(honk_wav);  // Play honk sound when stopped
                currentState = FOLLOW_LINE;
                break;

            case END:
                stop();
                turnAround();
                green = 1;
                play_tone(false);
                //play_sound(end_of_line_wav);  // Play tone when the end of the line is reached
                wait_us(3000000);
                currentState = IDLE; 
                break;

        }
    }
}
