// Motor control pins
// Left Motor
int enL = 7;
int inL1 = 8;
int inL2 = 9;

// Right motor
int enR = 6;
int inR1 = 10;
int inR2 = 11;

// For encoder
int enLA = 18;
int enLB = 19;

int enRA = 2;
int enRB = 3;

unsigned long lastTime = 0;

// Control constants
const float d = 0.189;    // Distance between wheels (meters)
const float r = 0.0216; // Radius of the wheels (meters)
const float T = 0.5;      // Sampling time (seconds)

// increase -> higher speed
const float gamma = 0.2;  // Linear control gain
// controls how sharply the robot turns to align with its target
const float lamda = 0.25; // Angular control gain
// reduce deviation errors when the robot’s path isn't aligned well
const float h = 0.15;     // Offset in angle calculation

// Encoder constantss
int pulses_per_rev = 700; 
const float wheel_diameter = r*2; // Diameter of the wheel in meters
//const float M_PI = 3.14159265358979323846;
const float wheel_circumference = wheel_diameter * M_PI ; // 

const float x_g = 1.0;  // Goal X position (meters)
const float y_g = 1.0;  // Goal Y position (meters)
const float theta_g = 0.0; // Goal orientation (radians)

// Robot state variables
float x = 0.0;     // Current X position (meters)
float y = 0.0;     // Current Y position (meters)
float theta = 0.0; // Current orientation (radians)

// Encoder counts and speed control
volatile int leftEnCount = 0;
volatile int rightEnCount = 0;
// const int K = 30;  // Adjustment factor for speed control

void setup() {
  Serial.begin(9600);

  // Setup encoder interrupts
  attachInterrupt(digitalPinToInterrupt(enLA), leftEnISRA, RISING);
  attachInterrupt(digitalPinToInterrupt(enLB), leftEnISRB, RISING);
  attachInterrupt(digitalPinToInterrupt(enRA), rightEnISRA, RISING);
  attachInterrupt(digitalPinToInterrupt(enRB), rightEnISRB, RISING);

  // Set all the motor control pins to outputs
  pinMode(enR, OUTPUT);
  pinMode(enL, OUTPUT);
  pinMode(inR1, OUTPUT);
  pinMode(inR2, OUTPUT);
  pinMode(inL1, OUTPUT);
  pinMode(inL2, OUTPUT);

  // Turn off motors - Initial state
  digitalWrite(inR1, LOW);
  digitalWrite(inR2, LOW);
  digitalWrite(inL1, LOW);
  digitalWrite(inL2, LOW);
}

void loop() {
  // Calculate control inputs
  float deltaX = x_g - x;
  float deltaY = y_g - y;

  // distance error
  float rho = sqrt(deltaX * deltaX + deltaY * deltaY);
  // angle between the robot’s current heading (orientation) 
  // and the direction from the robot to the goal.
  float phi = atan2(deltaY, deltaX) - theta_g;
  // difference between the angle to the goal and the desired heading at the goal
  float alpha = atan2(deltaY, deltaX) - theta;

  // linear velocity
  float v = gamma * cos(alpha) * rho;
  // angular velocity
  float w = lamda * alpha + gamma * cos(alpha) * sin(alpha) * (alpha + h * phi) / alpha;

  //  Serial.print("v:");
  // Serial.println(v);
  // Serial.print("w:");
  // Serial.println(w); 

  float vr = v + d * w / 2.0; // v: m/s, w: rad/s or rpm below
  float vl = v - d * w / 2.0; 
 
  // linear velo to angular velo in Radians per Second: v/r
  // rad/s to RPM: revolution per sec = w/2pi -> rpm: x60
  float wr = vr/r * 60.0 / (2.0 * M_PI); // Angular velocity in RPM
  float wl = vl/r * 60.0 / (2.0 * M_PI);

  // Set wheel speeds: convert rpm to 0->255
  set_speedL(wl);
  set_speedR(wr);

  digitalWrite(inL1, LOW);
  digitalWrite(inL2, HIGH);
  digitalWrite(inR1, HIGH);
  digitalWrite(inR2, LOW); 

  // how to make sure it runs in 1.2 secs
  // Wait for the time step
  delay(T * 1000); // Convert seconds to milliseconds
      
  // unsigned long currentTime = millis();
  // unsigned long lastUpdate = millis();  // Initialize lastUpdate to the current time

  // if (currentTime - lastUpdate >= T * 1000) {
    float v1 = get_speedL();
    float v2 = get_speedR();

    x += (v1 + v2) / 2.0 * cos(theta) * T;
    y += (v1 + v2) / 2.0 * sin(theta) * T;
    theta += (v2 - v1) / d * T;

  //   lastUpdate = currentTime;  // Update lastUpdate to the current time
  // }

  

  // Print debugging information
  Serial.print("X: ");
  Serial.println(x);
  Serial.print(" Y: ");
  Serial.println(y);
  Serial.print(" Theta: ");
  Serial.println(theta);
  Serial.print(" Left Speed: ");
  Serial.println(v1);
  Serial.print(" Right Speed: ");
  Serial.println(v2);
  Serial.println();

  Serial.println("=================");
    Serial.println();



  // Check if goal is reached
  if (fabs(x_g - x) <= 0.05 && fabs(y_g - y) <= 0.05) {
    stop();
//    Serial.println("Goal reached!");
    while (1); // Stop further execution

   
  }
}

// Function to set the speed of the left wheel
void set_speedL(float speed) {
  // Serial.print("Left Speed:");

  // Serial.println(speed);
  int pwmValue = map(speed, 0, 460, 0, 255); // wL is in RPM, map it to 0-255  
  // pwmValue = constrain(pwmValue, 0, 255); // Ensure the PWM value is within the valid range
   analogWrite(enL, pwmValue);
}

// Function to set the speed of the right wheel
void set_speedR(float speed) {
    // Serial.print("Right Speed:");

  // Serial.println(speed);

  int pwmValue = map(speed, 0, 460, 0, 255); 
   analogWrite(enR, pwmValue);
}

// Function to get the current speed of the left wheel
float get_speedL() {
  // Convert encoder counts to speed in m/s
  float countsPerSecond = leftEnCount / T; // Counts per second
  float speedRPM = (countsPerSecond * 60) / pulses_per_rev; // RPM
  float speedMetersPerSecond = speedRPM * (wheel_circumference / 60); // m/s
  leftEnCount = 0; // Reset count after reading
  return speedMetersPerSecond;
}

// Function to get the current speed of the right wheel
float get_speedR() {
  // Convert encoder counts to speed in m/s
  float countsPerSecond = rightEnCount / T; // Counts per second
  float speedRPM = (countsPerSecond * 60) / pulses_per_rev; // RPM
  float speedMetersPerSecond = speedRPM * (wheel_circumference / 60); // m/s
  rightEnCount = 0; // Reset count after reading
  // Serial.print("pulses:");
  // Serial.println(countsPerSecond);
  //   Serial.print("rpm:");
  //     Serial.println(speedRPM);

  return speedMetersPerSecond;
}

// Encoder ISR for left wheel
void leftEnISRA() {
  leftEnCount++;
}

void leftEnISRB() {
  leftEnCount++;
}

// Encoder ISR for right wheel
void rightEnISRA() {
  rightEnCount++;
}

void rightEnISRB() {
  rightEnCount++;
}

// Stop the motors
void stop() {
  digitalWrite(inR1, LOW);
  digitalWrite(inR2, LOW);
  digitalWrite(inL1, LOW);
  digitalWrite(inL2, LOW);
}