#include "mbed.h"
#include "chrono"
#include "string"


/**
*       MOTOR PINS
*/
PwmOut motorLeftSpeed(p25);      // Left motor speed control (PWM)
PwmOut motorRightSpeed(p26);     // Right motor speed control (PWM)

DigitalOut motorLeftIn1(p22); // Left motor direction control
DigitalOut motorLeftIn2(p21); // Right motor direction control
DigitalOut motorRightIn1(p14); // Left motor direction control
DigitalOut motorRightIn2(p13); // Right motor direction control



/**
*       BUTTON PINS
*/
DigitalIn button(p30);             // Button to start/stop movement
DigitalIn dipSwitch(p29);          // DIP switch for speed control 



/**
*       SENSOR PINS
*/
DigitalOut trigger(p10);
DigitalIn echo(p11);
DigitalInOut l1(p20);
DigitalInOut l3(p19);
DigitalInOut l2(p17);
DigitalInOut l4(p16);



/**
*       TIMERS
*/
Timer sonarTimer;
Timer debounceTimer;            // Timer to handle button debounce
Timer sensorTimer;



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




/**
*       GLOBAL VARIABLES
*/
#define PI 3.14159265358979323846
#define MIN_DIST 20           // Distance for obstacle detection 20cm
#define DEBOUNCE_DELAY 50ms   // Debounce delay in seconds 50ms
#define THRESHOLD 50        // For detecting end of line
#define CONF_THRES 30       // Allows for some off-line navigation while still stopping at end of line
#define CALIBRATION_TIME_MS 3000  // Spin for 3 seconds
#define MAX_TIME 250        // for line sensor discharging

volatile float dist = 0.0f;  
bool buttonState = false;       // Current button state
bool lastButtonState = false;   // Previous button state
float baseSpeed = 0.4;     // Base speed for the motors

int thresholds[4];  // Calibrated thresholds per sensor
int idleConfidence = 0;

float Kp = 5.0, Ki = 0.0, Kd = 0.05; // PID controller parameters
float previous_error = 0, integral = 0;

enum RobotState { IDLE, FOLLOW_LINE, OBSTRUCTED, END }; // States for line following and obstacle detection
RobotState currentState = IDLE;



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

    float speed = std::min(baseSpeed * 2.0f, 1.0f); // Clamp speed to max 1.0f
    set_motor_speed(speed, speed);

    wait_us(1000000);//wait_us(680000); // Test to see how long it takes to turn 180 degrees

    init_motor_control();

}

float clamp(float speed) {
    return std::max(0.0f, std::min(1.0f, speed)); // Ensures speed is between 0 and 1
}



/**
*
*       BUTTON CONTROL
*
*/
bool checkButtonState() {
    if (debounceTimer.elapsed_time() > DEBOUNCE_DELAY) {
        bool currentButtonState = button.read();

        // Check if the button state has changed
        if (currentButtonState != lastButtonState) {
            lastButtonState = currentButtonState;
            debounceTimer.reset();  // Reset the debounce timer

            if (currentButtonState) {  // Button pressed
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
    baseSpeed = dipSwitch ? baseSpeed * 1.5 : baseSpeed;
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

    auto timeElapsed = sonarTimer.elapsed_time();
    sonarTimer.reset();
    float timeUs = std::chrono::duration_cast<std::chrono::microseconds>(timeElapsed).count();
    return timeUs * 0.01715f;
}

void detect_obstacle() {
    if (measureDistance() < MIN_DIST) {
        currentState = OBSTRUCTED;
    }
}

void line_following() {
    int s[4];
    DigitalInOut* sensors[4] = { &l1, &l2, &l3, &l4 };

    // Charge all sensors
    for (int i = 0; i < 4; i++) {
        sensors[i]->output();
        *(sensors[i]) = 1;
    }
    wait_us(10);

    // Switch to input
    for (int i = 0; i < 4; i++) {
        sensors[i]->input();
    }

    // Measure discharge time
    sensorTimer.reset();
    sensorTimer.start();

    for (int i = 0; i < 4; i++) {
        while (sensors[i]->read() == 1) {
            if (sensorTimer.elapsed_time().count() >= MAX_TIME) break;
        }
        s[i] = chrono::duration_cast<chrono::microseconds>(sensorTimer.elapsed_time()).count();
    }

    sensorTimer.stop();

    // Normalize discharge times
    float l1sensor = (float)s[0] / MAX_TIME;
    float l2sensor = (float)s[1] / MAX_TIME;
    float l3sensor = (float)s[2] / MAX_TIME;
    float l4sensor = (float)s[3] / MAX_TIME;

    // Weighted error calculation for PID
    float error = -3 * l1sensor - 1 * l2sensor + 1 * l3sensor + 2.6 * l4sensor;

    integral += error;
    integral = std::max(-1.0f, std::min(1.0f, integral));
    float derivative = error - previous_error;

    float output = Kp * error + Ki * integral + Kd * derivative;
    output = std::max(-1 * baseSpeed, std::min(baseSpeed, output)); // Something here needs to be done for speed agnostic

    set_motor_speed(clamp(baseSpeed - output), clamp(baseSpeed + output));
    printf("s0:%d, s1:%d, s2:%d, s3:%d, out:%d, confidence:%d\n", s[0], s[1], s[2], s[3], 
    (int) output * 100, 
    (int) idleConfidence);
    previous_error = error;

    if (s[1] < thresholds[0] && s[2] < thresholds[0]) {
        idleConfidence++;
        if (idleConfidence >= CONF_THRES) {
            currentState = END;
        }
    } else {
        idleConfidence = 0;
    }
}

void calibrate_sensors() {
    int minVals[4] = { MAX_TIME, MAX_TIME, MAX_TIME, MAX_TIME };
    int maxVals[4] = { 0, 0, 0, 0 };

    DigitalInOut* sensors[4] = { &l1, &l2, &l3, &l4 };
    Timer calibrationTimer;
    calibrationTimer.start();

    printf("Starting calibration...\n");

    while (calibrationTimer.elapsed_time().count() < CALIBRATION_TIME_MS * 1000) {
        // Slowly spin in place
        set_motor_speed(-0.8f, 0.8f);

        // Charge
        for (int i = 0; i < 4; i++) {
            sensors[i]->output();
            *(sensors[i]) = 1;
        }
        wait_us(10);

        // Switch to input
        for (int i = 0; i < 4; i++) {
            sensors[i]->input();
        }

        // Measure discharge time
        sensorTimer.reset();
        sensorTimer.start();
        int readings[4];

        for (int i = 0; i < 4; i++) {
            sensorTimer.reset();
            sensorTimer.start();
            while (sensors[i]->read() == 1) {
                if (sensorTimer.elapsed_time().count() >= MAX_TIME) break;
            }
            readings[i] = chrono::duration_cast<chrono::microseconds>(sensorTimer.elapsed_time()).count();
        }
        sensorTimer.stop();

        // Update min/max per sensor
        for (int i = 0; i < 4; i++) {
            if (readings[i] < minVals[i]) minVals[i] = readings[i];
            if (readings[i] > maxVals[i]) maxVals[i] = readings[i];
        }

        thread_sleep_for(20);
    }
    set_motor_speed(0, 0);

    // Compute thresholds
    for (int i = 0; i < 4; i++) {
        thresholds[i] = minVals[i] + (maxVals[i] - minVals[i]) * 0.9f;
        printf("Sensor %d: min=%d, max=%d, threshold=%d\n", i + 1, minVals[i], maxVals[i], thresholds[i]);
    }

    calibrationTimer.stop();
    printf("Calibration complete.\n");
}



/**
*
*       SPEAKER CONTROL
*
*/
struct Note {
    float frequency;   // Hz
    float duration;    // seconds
};

// Use freqeuncies to create simple sound effects. Honk is like duh - pause - duh. Success is duh, duh, duh, rising
Note honkMelody[] = {
    {700.0f, 0.08f},
    {0.0f,   0.05f},
    {700.0f, 0.1f}
};

Note successMelody[] = {
    {1000.0f, 0.15f},
    {1200.0f, 0.15f},
    {1500.0f, 0.2f}
};

void play_note(float frequency, float duration, float volume = 0.5f) {
    float sample_rate = 44100.0f;
    int samples = duration * sample_rate;
    
    for (int i = 0; i < samples; ++i) {
        float t = static_cast<float>(i) / sample_rate;
        float wave = 0.5f + volume * sinf(2.0f * PI * frequency * t);
        DACout.write(wave);
        wait_us(1000000 / sample_rate);
    }

    DACout.write(0.1f);
    wait_us(20000);
}

void play_tone(bool isHonk) {
    Note* melody;
    int length;

    if (isHonk) {
        melody = honkMelody;
        length = sizeof(honkMelody) / sizeof(Note);
    } else {
        melody = successMelody;
        length = sizeof(successMelody) / sizeof(Note);
    }

    for (int i = 0; i < length; ++i) {
        play_note(melody[i].frequency, melody[i].duration);
    }
}



/**
*
*       MAIN CODE
*
*/
void init_robot() {
    init_motor_control();
    calibrate_sensors();
    debounceTimer.start(); 
    lastButtonState = button.read();  // Set the initial state of the button
    buttonState = lastButtonState;    // Make sure we start with the correct state
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
                currentState = FOLLOW_LINE;
                break;

            case END:
                stop();
                turnAround();
                green = 1;
                play_tone(false);
                currentState = IDLE; 
                break;
        }
    }
}
