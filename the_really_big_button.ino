#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define LED 5
#define BTN 4

#define TIMEOUT_MS 3000

int debug = true;

enum states { STATE_IDLE, STATE_STARTING, STATE_DELAYING, STATE_WAITING, STATE_ERR, STATE_DONE };
unsigned long on_time, start_time, stop_time, delta, delay_time;
enum states state = STATE_IDLE;

void db(char *msg) {
  if (debug)
    Serial.println(msg);
}

void flashLedForMs(unsigned long ms) {
  digitalWrite(LED, HIGH);
  delay(ms);
  digitalWrite(LED, LOW);
}

void errorFlashes() {
  for (int n=0; n<5; n++) {
    flashLedForMs(100);
    delay(100);
  }
}

void buttonHandler() {
  if (state == STATE_IDLE) {
    state = STATE_STARTING;
    return;
  }
  if (state == STATE_WAITING) {
    stop_time = millis();
    state = STATE_DONE;
    return;
  }
  if (state == STATE_DELAYING) {
    if (millis() - on_time < 250) {
      return;
    }
    Serial.println(millis() - start_time);
    state = STATE_ERR;
    return;
  }
}

void say(char *str) {
  db(str);
  display.clearDisplay();
  display.setCursor(0, 0);
  char *p = str;
  while (*p) {
    display.write(*p);
    p++;
  }
  display.display();
}

void setup() {
  Serial.begin(9600);
  Serial.println("begin");

  on_time = 0;
  start_time = 0;
  stop_time = 0;
  delay_time = 0;
  delta = 0;

  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);
  
  pinMode(BTN, INPUT_PULLUP);

  flashLedForMs(500);

  Serial.begin(9600);
  Serial.println("begin");

  attachInterrupt(digitalPinToInterrupt(BTN), buttonHandler, FALLING);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
  }

  display.display();

  display.setTextSize(3);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.cp437(true);         // Use full 256 char 'Code Page 437' font

  errorFlashes();
  
  say("Ready");
}

void loop() {
  char str_delta[16];
  switch(state) {
    case STATE_IDLE:
      break;
    case STATE_STARTING:
      digitalWrite(LED, HIGH);
      on_time = millis();
      delay_time = millis() + random(750, 3000);
      state = STATE_DELAYING;
      say("Wait");
      break;
    case STATE_DELAYING:
      if (millis() > delay_time) {
        state = STATE_WAITING;
        digitalWrite(LED, LOW);
        start_time = millis();
      }
      break;
    case STATE_WAITING:
      if (millis() > delay_time + TIMEOUT_MS) {
        say("Timeout");
        for (int n=0; n<5; n++) {
          flashLedForMs(100);
          delay(100);
        }
        state = STATE_IDLE;
        say("Ready");
      }
      break;
    case STATE_ERR:
      say("XXX");
      errorFlashes();
      errorFlashes();
      say("Ready");
      state = STATE_IDLE;
      break;
    case STATE_DONE:
      //stop_time = millis();
      flashLedForMs(50);
      delta = stop_time - start_time;
      Serial.println(delta);
      say(itoa(delta, str_delta, 10));
      delay(250); // give the switch pleny of time to debounce, moving to STATE_IDLE means that the next falling interrupt starts a new game
      state = STATE_IDLE;
    default:
      // this should never happen
      break;
  }
}
