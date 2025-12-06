#include <Bluepad32.h>

// === Motor driver pins ===
#define IN1 25   // Motor A input 1
#define IN2 26   // Motor A input 2
#define IN3 27   // Motor B input 1
#define IN4 14   // Motor B input 2
#define ENA 15   // Motor A ENA (Speed)
#define ENB 13   // Motor B ENB (Speed)

// === Motor driver PWM setup ===
#define PWM_FREQ 20000
#define PWM_RES 8
#define CH_ENA 0
#define CH_ENB 1
const int PWM_MIN = 90;
const int PWM_MAX = 255;

// === Speaker pins ===
#define SPEAKER_PIN 2

// === Speaker PWM setup ===
#define SPEAKER_PWM_RES 8
#define SPEAKER_PWM_DUTY 240
#define CH_SPEAKER 2
const int SPEAKER_FREQ_MIN  = 50;   // Hz
const int SPEAKER_FREQ_MAX  = 1000;  // Hz

// === Meow sound setup ===
const int MEOW_STEP_COUNT = 18;
const int MEOW_FREQ_SEQ[MEOW_STEP_COUNT] = {
  380, 450, 520, 600, 700, 800,
  880, 930, 880, 920, 880,
  800, 720, 640, 560, 480, 400, 340
};
const int MEOW_DUR_SEQ[MEOW_STEP_COUNT] = {
  60,  60,  70,  80,  90,  90,
  100, 80,  80,  80,  80,
  90,  90, 100, 110, 120, 130, 140
};
const int MEOW_PAUSE_BETWEEN = 2;  // ms

// === Super Mario melody setup ===
const int MARIO_STEP_COUNT = 24;
const int MARIO_FREQ_SEQ[MARIO_STEP_COUNT] = {
  659, 659, 659, 523, 659, 784, 392,
  523, 392, 330, 440, 494, 466, 440,
  392, 659, 784, 880, 698, 784, 659,
  523, 587, 494
};
const int MARIO_DUR_SEQ[MARIO_STEP_COUNT] = {
  120, 120, 120, 120, 120, 180, 180,
  120, 120, 120, 120, 120, 120, 120,
  150, 150, 150, 180, 120, 120, 150,
  120, 120, 200
};
const int MARIO_PAUSE_BETWEEN = 3;  // ms

// === States of the current tone sequence ===
bool soundActive      = false;
const int *soundFreqs = nullptr;
const int *soundDurs  = nullptr;
int soundCount        = 0;
int soundIndex        = 0;
int soundPauseMs      = 0;
unsigned long soundNextChange = 0;

// === LED pins ===
#define LED_WHITE_PIN 23
#define LED_BLUE_PIN  18
#define LED_GREEN_PIN 5
#define LED_RED_PIN   17

// === LED PWM setup ===
#define CH_LED_WHITE 3
#define CH_LED_BLUE  4
#define CH_LED_GREEN 5
#define CH_LED_RED   6

const int LED_PWM_MAX     = PWM_MAX;   // 0..255
const int LED_WHITE_BLINK_HZ = 2;
const int LED_BLUE_BLINK_HZ  = 3;
const int LED_GREEN_BLINK_HZ = 4;
const int LED_RED_BLINK_HZ   = 2;

// === LED state ===
bool ledWhiteOn = false;
bool ledBlueOn  = false;
bool ledGreenOn = false;
bool ledRedOn   = false;
bool ledWhiteLit = false;
bool ledBlueLit  = false;
bool ledGreenLit = false;
bool ledRedLit   = false;
unsigned long ledWhiteNextToggle = 0;
unsigned long ledBlueNextToggle  = 0;
unsigned long ledGreenNextToggle = 0;
unsigned long ledRedNextToggle   = 0;

// === Controller Buttons ===
bool prevButtonA = false;
bool prevButtonB = false;
bool prevButtonX = false;
bool prevButtonY = false;
bool prevButtonR1 = false;
bool prevButtonL1 = false;

// === Joystick setup ===
const int JOYSTICK_MIN = 0;
const int JOYSTICK_MAX = 512;
const int JOYSTICK_DEADZONE = 50;
const int THROTTLE_MIN = 0;
const int THROTTLE_MAX = 1024;

GamepadPtr myGamepad = nullptr;

// === Function prototypes ===
void onConnectedGamepad(GamepadPtr gp);
void onDisconnectedGamepad(GamepadPtr gp);
void handleControls();

// === Setup ===
void setup() {
  Serial.begin(115200);
  Serial.println("Starting Bluepad32 - Arcade Drive Mode");

  // Motor pin setup
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  // LED pin setup
  pinMode(LED_WHITE_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN,  OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_RED_PIN,   OUTPUT);

  // Attach and configure PWM
  ledcSetup(CH_ENA,       PWM_FREQ,         PWM_RES);
  ledcSetup(CH_ENB,       PWM_FREQ,         PWM_RES);
  ledcSetup(CH_SPEAKER,   SPEAKER_FREQ_MIN, SPEAKER_PWM_RES);
  ledcSetup(CH_LED_WHITE, PWM_FREQ,         PWM_RES);
  ledcSetup(CH_LED_BLUE,  PWM_FREQ,         PWM_RES);
  ledcSetup(CH_LED_GREEN, PWM_FREQ,         PWM_RES);
  ledcSetup(CH_LED_RED,   PWM_FREQ,         PWM_RES);

  ledcAttachPin(ENA,           CH_ENA);
  ledcAttachPin(ENB,           CH_ENB);
  ledcAttachPin(SPEAKER_PIN,   CH_SPEAKER);
  ledcAttachPin(LED_WHITE_PIN, CH_LED_WHITE);
  ledcAttachPin(LED_BLUE_PIN,  CH_LED_BLUE);
  ledcAttachPin(LED_GREEN_PIN, CH_LED_GREEN);
  ledcAttachPin(LED_RED_PIN,   CH_LED_RED);

  // Initialize Bluepad32
  BP32.setup(&onConnectedGamepad, &onDisconnectedGamepad);
  BP32.forgetBluetoothKeys();  // optional

  Serial.println("Waiting for controller...");
}

// === Loop ===
void loop() {
  BP32.update();

  if (myGamepad && myGamepad->isConnected()) {
    handleControls();
  } else {
    // stop all motors, speaker and LED
    ledcWrite(CH_ENA,       0);
    ledcWrite(CH_ENB,       0);
    ledcWrite(CH_SPEAKER,   0);
    ledcWrite(CH_LED_WHITE, 0);
    ledcWrite(CH_LED_BLUE,  0);
    ledcWrite(CH_LED_GREEN, 0);
    ledcWrite(CH_LED_RED,   0);

    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
  }

  delay(10);
}

// === Bluepad32 callbacks ===
void onConnectedGamepad(GamepadPtr gp) {
  myGamepad = gp;
  Serial.println("✅ Controller connected!");
  playMeow();
}

void onDisconnectedGamepad(GamepadPtr gp) {
  if (myGamepad == gp) {
    myGamepad = nullptr;
    Serial.println("❌ Controller disconnected!");
  }
}

// === Utility: map joystick to PWM ===
int mapJoystickToPWM(int value) {
  if (value == 0) return 0;
  int pwm = map(abs(value), JOYSTICK_MIN, JOYSTICK_MAX, PWM_MIN, PWM_MAX);
  return constrain(pwm, PWM_MIN, PWM_MAX);
}

// === Utility: map throttle to frequency ===
int mapThrottleToFreq(int value) {
  if (value <= 0) return 0;
  int freq = map(value, THROTTLE_MIN, THROTTLE_MAX, SPEAKER_FREQ_MIN, SPEAKER_FREQ_MAX);
  return constrain(freq, SPEAKER_FREQ_MIN, SPEAKER_FREQ_MAX);
}

// === Drive helper ===
void driveMotor(int channelEN, int pinIN1, int pinIN2, int speed) {
  if (speed > 0) {
    digitalWrite(pinIN1, HIGH);
    digitalWrite(pinIN2, LOW);
    ledcWrite(channelEN, speed);
  } else if (speed < 0) {
    digitalWrite(pinIN1, LOW);
    digitalWrite(pinIN2, HIGH);
    ledcWrite(channelEN, abs(speed));
  } else {
    digitalWrite(pinIN1, LOW);
    digitalWrite(pinIN2, LOW);
    ledcWrite(channelEN, 0);
  }
}

void controlMotor() {
  int turn     = -myGamepad->axisRX();
  int forward  = -myGamepad->axisY();

  // --- Deadzone ---
  if (abs(forward) < JOYSTICK_DEADZONE) forward = 0;
  if (abs(turn) < JOYSTICK_DEADZONE) turn = 0;

  // --- Mix for arcade drive ---
  int leftSpeed  = forward + turn;
  int rightSpeed = forward - turn;

  // Clamp to joystick range (-512 to 512)
  leftSpeed  = constrain(leftSpeed, -JOYSTICK_MAX, JOYSTICK_MAX);
  rightSpeed = constrain(rightSpeed, -JOYSTICK_MAX, JOYSTICK_MAX);

  // Map to PWM
  int leftPWM  = mapJoystickToPWM(leftSpeed);
  int rightPWM = mapJoystickToPWM(rightSpeed);

  // Apply sign (direction)
  driveMotor(CH_ENA, IN1, IN2, leftSpeed >= 0 ? leftPWM : -leftPWM);
  driveMotor(CH_ENB, IN3, IN4, rightSpeed >= 0 ? rightPWM : -rightPWM);  
}

// === Speaker helper ===
void driveSpeaker(int freq) {
  if (freq == 0) {
    ledcWrite(CH_SPEAKER, 0);
  } else {
    ledcSetup(CH_SPEAKER, freq, SPEAKER_PWM_RES);
    ledcWrite(CH_SPEAKER, SPEAKER_PWM_DUTY);
  }
}

void playToneSequence(const int *freqs, const int *durations, int count, int pauseMs) {
  if (!freqs || !durations || count <= 0) return;
  soundFreqs   = freqs;
  soundDurs    = durations;
  soundCount   = count;
  soundPauseMs = pauseMs;
  soundIndex   = 0;
  soundActive  = true;
  soundNextChange = 0;  // sofort starten
}

void updateToneSequence() {
  if (!soundActive) return;

  unsigned long now = millis();
  if (now < soundNextChange) return;

  if (soundIndex >= soundCount) {
    driveSpeaker(0);
    soundActive = false;
    return;
  }

  int freq = soundFreqs[soundIndex];
  int dur  = soundDurs[soundIndex];

  if (freq <= 0) {
    driveSpeaker(0);
  } else {
    freq = constrain(freq, SPEAKER_FREQ_MIN, SPEAKER_FREQ_MAX);
    driveSpeaker(freq);
  }

  if (dur < 0) dur = 0;
  soundNextChange = now + (unsigned long)dur + (unsigned long)soundPauseMs;
  soundIndex++;
}

void playMeow() {
  playToneSequence(MEOW_FREQ_SEQ, MEOW_DUR_SEQ, MEOW_STEP_COUNT, MEOW_PAUSE_BETWEEN);
}

void playMario() {
  playToneSequence(MARIO_FREQ_SEQ, MARIO_DUR_SEQ, MARIO_STEP_COUNT, MARIO_PAUSE_BETWEEN);
}

void handleSoundButton(bool buttonState, bool &prevState, void (*soundFunc)()) {
  if (buttonState && !prevState && !soundActive) {
    soundFunc();
  }
  prevState = buttonState;
}

void controlSpeaker() {
  // R1 triggers a single meow sound
  handleSoundButton(myGamepad->r1(), prevButtonR1, playMeow);
  // L1 triggers the Mario melody
  handleSoundButton(myGamepad->l1(), prevButtonL1, playMario);

  // Continue running tone sequence in the background if sound is active
  if (soundActive) {
    updateToneSequence();
    return;
  }

  // Throttle controls continuous tone
  int throttle = myGamepad->throttle();
  int freq = mapThrottleToFreq(throttle);
  driveSpeaker(freq);
}

// === LED helpers ===
bool updateLEDState(bool button, bool &prevState, bool &ledState) {
  bool changed = false;
  if (button && !prevState) {
    ledState = !ledState;
    changed = true;
  }
  prevState = button;
  return changed;
}

unsigned long blinkIntervalMs(int freqHz) {
  if (freqHz <= 0) return 1000;
  return max(1UL, 500UL / (unsigned long)freqHz);  // half period
}

void updateBlinkingLED(bool enabled, bool &lit, unsigned long &nextToggle, int channel, int freqHz) {
  unsigned long now = millis();

  if (!enabled) {
    lit = false;
    nextToggle = 0;
    ledcWrite(channel, 0);
    return;
  }

  if (nextToggle == 0) {
    lit = true;
    nextToggle = now + blinkIntervalMs(freqHz);
    ledcWrite(channel, LED_PWM_MAX);
    return;
  }

  if (now >= nextToggle) {
    lit = !lit;
    nextToggle = now + blinkIntervalMs(freqHz);
    ledcWrite(channel, lit ? LED_PWM_MAX : 0);
  }
}

void controlLEDs() {
  bool btnA = myGamepad->a();
  bool btnB = myGamepad->b();
  bool btnX = myGamepad->x();
  bool btnY = myGamepad->y();

  if (updateLEDState(btnA, prevButtonA, ledWhiteOn)) ledWhiteNextToggle = 0;
  if (updateLEDState(btnB, prevButtonB, ledBlueOn))  ledBlueNextToggle  = 0;
  if (updateLEDState(btnX, prevButtonX, ledGreenOn)) ledGreenNextToggle = 0;
  if (updateLEDState(btnY, prevButtonY, ledRedOn))   ledRedNextToggle   = 0;

  updateBlinkingLED(ledWhiteOn, ledWhiteLit, ledWhiteNextToggle, CH_LED_WHITE, LED_WHITE_BLINK_HZ);
  updateBlinkingLED(ledBlueOn,  ledBlueLit,  ledBlueNextToggle,  CH_LED_BLUE,  LED_BLUE_BLINK_HZ);
  updateBlinkingLED(ledGreenOn, ledGreenLit, ledGreenNextToggle, CH_LED_GREEN, LED_GREEN_BLINK_HZ);
  updateBlinkingLED(ledRedOn,   ledRedLit,   ledRedNextToggle,   CH_LED_RED,   LED_RED_BLINK_HZ);
}

// === Core control logic ===
void handleControls() {
  controlMotor();
  controlSpeaker();
  controlLEDs();
}
