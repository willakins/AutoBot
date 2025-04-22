# ECE-4180 Final Project  
**By:** William Akins  

---

## Introduction

This project aims to design and implement an autonomous robot using the Mbed microcontroller. The robot will follow a black line on the ground upon the press of a button and navigate until the line ends. Once the robot has reached the end of the line, the same button (upon being pressed) will cause the robot to travel in the opposite direction until the line ends again.

Additionally, it incorporates a Ultrasonic distance sensor to detect obstacles in its path and halt movement accordingly. For flexibility, a DIP switch onboard allows users to adjust the speed of the robot. A speaker enables the robot to play a “honk” sound effect when movement is halted due to an obstacle, and a tone when the robot has reached the end of the line.

An RGB LED provides visual feedback: it flashes red while the robot is blocked and green when it successfully reaches the end of the line. The integration of line-following and obstacle detection enhances the robot’s navigation capabilities, making it a simple but effective model for automated guided vehicle (AGV) applications in warehouses, factories, and smart transport systems.

---

## Parts Needed

### Microcontroller & Power
- Mbed Microcontroller  
- 4 AA Batteries  

### Chassis & Movement
- Shadow Chassis  
- Shadow Chassis Motor x2  
- Wheel - 65mm x2  
- SparkFun Motor Driver  

### Sensors
- Ultrasonic Distance Sensor - 5V (HC-SR04)  
- QTR-8RC Reflectance Sensor Array  

### Control & Interface
- DIP Switch (at least 1 switch)  
- Push Button  

### Audio
- Speaker  
- Mono Audio Amp Breakout - TPA2005D1 

### Indicators
- RGB LED  

---

## Explanation of Parts

- **Mbed Microcontroller**: Handles logic between all sensors and actuators.  
- **4 AA Batteries**: Enable untethered, mobile operation.  
- **Shadow Chassis + Motors**: Provide a compact base and differential drive for maneuverability.  
- **SparkFun Motor Driver**: Allows control over speed and direction of motors.  
- **Ultrasonic Distance Sensor**: Detects objects in the robot’s path to avoid collisions.  
- **QTR-8RC Reflectance Sensor Array**: Guides the robot along the line using PID logic.  
- **DIP Switch**: Lets the user select between different speed levels.  
- **Push Button**: Toggles robot motion (on/off).  
- **Speaker**: Plays Analog tones.  
- **RGB LED**: Indicates status (e.g., red for blocked, green for success).
- **Mono Audio Amp Breakout - TPA2005D1**: Allows for audible sounds to be played to the speaker.

---

## Difficulties and Improvements

Two major difficulties during development:

1. **Robot PID tuning**: My line following robot relies on measuring reflectance of a surface, and from testing this is incredibly variable based on the environment (lighting, flooring color, etc.), making it quite difficult to tune the robot control algorithm that ensures the robot follows the black line in all conditions.
2. **Sensor Mounting and Turning Behavior**: The QTR sensor array didn’t mount cleanly onto the chassis, requiring additional hardware engineering. Also, implementing reliable turning at the end of the line proved challenging; current behavior is based on a timed 180-degree turn, which is inconsistent across different surfaces.

**Future Improvements**:
- Create a custom file system library for SD card interfacing, allowing for more flexible sounds to be played.
- Use sensor feedback instead of timed delays for accurate turning and re-alignment with the line.

---

## Similarities and Differences to Real-World Systems

While real-world warehouse robots are significantly more advanced, this project serves as a scaled-down prototype. Both systems use:
- Differential drive with two motors
- Basic collision detection

**Key Differences**:
- Real-world robots use advanced LiDAR, SLAM, and path-planning algorithms.
- This robot relies on low-cost components and predefined paths.

**Advantages of This Design**:
- Efficient for simple, small-scale tasks.
- Cost-effective alternative where full warehouse-grade automation is unnecessary.
- Ideal for educational demonstrations and prototype development.

---

## Circuit Diagram

![Circuit Diagram](./ECE-Final_bb.png)
