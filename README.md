# Line-Following Robot with Obstacle Detection 🚀  

This project is a **PID-controlled line-following robot** using the **Mbed microcontroller**. The robot follows a black line on the ground, adjusts speed via **DIP switches**, stops when it detects an **obstacle using a TOF sensor**, and turns on an **RGB LED** to indicate its status.

---

## 🛠 Features
✅ **Line-following using PID control** for smooth movement  
✅ **Speed selection via DIP switch** (Slow / Fast mode)  
✅ **Obstacle detection** using a TOF sensor (Red LED on when an obstacle is detected)  
✅ **End-of-line detection** (Green LED on when the line disappears)  
✅ **PWM motor control** for smooth speed adjustment  

---

## 🔧 Hardware Requirements
| Component                              | Description |
|----------------------------------------|------------|
| **Mbed Microcontroller**               | Main control unit |
| **Reflectance Sensor Array (5 sensors)** | Detects black line on the surface |
| **TOF Sensor (Adafruit VL53L0X)**      | Detects obstacles |
| **Shadow Chassis with Motors**         | Base structure with wheels and motors |
| **SparkFun Motor Driver**              | Controls motors via PWM |
| **65mm Rubber Wheels (Pair)**          | Allows smooth movement |
| **DIP Switch (8-position)**            | Selects robot operation modes |
| **RGB LED (Red & Green used)**         | Status indicator |
| **5V Wall Adapter**                    | Powers the system |
| **Trimpot 10K**                         | Optional tuning component |
| **Electrolytic Capacitors (10uF, 1000uF)** | Power stability |

---

## 📜 How It Works
1. **Line Following (PID Control)**  
   - The **reflectance sensor array** detects the black line.  
   - A **PID controller** adjusts motor speeds based on sensor input.  
   
2. **Speed Selection**  
   - **DIP switch 1 (D2)**: Selects between **slow mode** and **fast mode**.  
   - **DIP switch 2 (D3)**: Reserved for additional features.  

3. **Obstacle Detection (TOF Sensor)**  
   - If an obstacle is detected, the **robot stops and the Red LED turns on**.  

4. **End-of-Line Detection**  
   - If no sensors detect the line, the **robot stops and the Green LED turns on**.  

---

## 💻 Code Overview
### **Main Control Code (`main.cpp`)**

