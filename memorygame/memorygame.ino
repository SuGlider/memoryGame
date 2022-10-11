#include <Bounce2.h>
//#include "notes.h"

// Pinout set for ESP32-C3 DevkitC
// Connect with S2 Kaluga via UART:
//   C3   | Kaluga
// RX1 18 | 19 TX
// TX1 19 | 20 RX

#define RED_LED 4
#define GREEN_LED 5
#define BLUE_LED 6
#define YELLOW_LED 7

#define RED_BUTTON 10
#define GREEN_BUTTON 1
#define BLUE_BUTTON 2
#define YELLOW_BUTTON 3

//for use with gameSequence[n]
uint8_t button_pin_map[4] = {RED_BUTTON, GREEN_BUTTON, BLUE_BUTTON, YELLOW_BUTTON};
char button_text_map[4][14] = {"RED_BUTTON", "GREEN_BUTTON", "BLUE_BUTTON", "YELLOW_BUTTON"};
uint8_t led_pin_map[4] = {RED_LED, GREEN_LED, BLUE_LED, YELLOW_LED};
char led_text_map[4][14] = {"RED_LED", "GREEN_LED", "BLUE_LED", "YELLOW_LED"};

enum command {RESTART, STARTUP, LEVEL_UP, FAILED};

// GPIO 8 is for RGD LED on board

#define BUZZER_PIN 9

#define MAX_GAME_SEQUENCE 30
#define FLASH_DELAY 250

Bounce debouncerRed = Bounce();
Bounce debouncerGreen = Bounce();
Bounce debouncerBlue = Bounce();
Bounce debouncerYellow = Bounce();

unsigned short gameSequence[MAX_GAME_SEQUENCE];

bool gameInProgress = false;
bool attractLEDOn = false;
bool showingSequenceToUser;
unsigned short currentSequenceLength;
unsigned short userPositionInSequence;
unsigned short n;
unsigned long flash_timer;

void led_on(uint8_t pin){
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
}

void all_leds_on(){
  led_on(RED_LED);
  led_on(GREEN_LED);
  led_on(BLUE_LED);
  led_on(YELLOW_LED);
}

void led_off(uint8_t pin){
  pinMode(pin, INPUT);
}

void all_leds_off(){
  led_off(RED_LED);
  led_off(GREEN_LED);
  led_off(BLUE_LED);
  led_off(YELLOW_LED);
}

void update_buttons(){
  debouncerRed.update();
  debouncerGreen.update();
  debouncerBlue.update();
  debouncerYellow.update();
}

void fail_strobe(){
  for(int i = 0; i < 10; ++i){
    all_leds_on();
    delay(75);
    all_leds_off();
    delay(75);
  }
}

void generate_sequence(unsigned short *sequence, unsigned long max_game_sequence){
  // Create a new game sequence.
  randomSeed(analogRead(0));
  unsigned short second_prev;
  unsigned short prev;

  sequence[0] = random(0, 3);
  second_prev = sequence[0];
  sequence[1] = random(0, 3);
  prev = sequence[1];

  for (int i = 2; i < max_game_sequence; i++) {
    sequence[i] = random(0, 3);
    if(sequence[i] == second_prev || sequence[i] == prev){
      // prevent 3 matching colors in a row (2 in row is ok)
      sequence[i] = (sequence[i]+1)%4;
    }
    Serial.printf("[%d]=%d\n", i, sequence[i]);
    second_prev = prev;
    prev = sequence[i];
  }
}

void correct_strobe(){
    led_on(YELLOW_LED);delay(100);
    led_on(BLUE_LED);delay(100);
    led_on(GREEN_LED);delay(100);
    led_on(RED_LED); delay(100);

    led_off(RED_LED);delay(100);
    led_off(GREEN_LED);delay(100);
    led_off(BLUE_LED);delay(100);
    led_off(YELLOW_LED);delay(100);
}

void setup() {
  Serial.begin(115200);
  while(!Serial){delay(100);}

  debouncerRed.attach(RED_BUTTON, INPUT_PULLUP);
  debouncerRed.interval(30);
  debouncerGreen.attach(GREEN_BUTTON, INPUT_PULLUP);
  debouncerGreen.interval(30);
  debouncerBlue.attach(BLUE_BUTTON, INPUT_PULLUP);
  debouncerBlue.interval(30);
  debouncerYellow.attach(YELLOW_BUTTON, INPUT_PULLUP);
  debouncerYellow.interval(30);

  pinMode(BUZZER_PIN, OUTPUT);

  //Serial1.begin(115200);
  //while(!Serial1){delay(100);}

  //Serial1.write(RESTART);
  flash_timer = millis();
}

void loop() {
  if (! gameInProgress) {
    // Waiting for someone to press any button...
      update_buttons();

    if (debouncerRed.fell() || debouncerGreen.fell() || debouncerBlue.fell() || debouncerYellow.fell()) {
      all_leds_off();
      
      generate_sequence(gameSequence, MAX_GAME_SEQUENCE);

      currentSequenceLength = 1;

      gameInProgress = true;
      showingSequenceToUser = true;

      // Little delay before the game starts.
      delay(500);
    } else {
      // Attract mode - flash the green LED.
      if (millis() - flash_timer >= FLASH_DELAY){
        attractLEDOn = ! attractLEDOn;
        if(attractLEDOn){
          all_leds_on();
        }else{
          all_leds_off();
        }
        flash_timer = millis();
      }
    }
  } else {
    // Game is in progress...
    if (showingSequenceToUser) {
      // Play the pattern out to the user
      for (n = 0; n < currentSequenceLength; n++) {
        Serial.printf("gameSequence[%d] = %d => Flash pin led_pin_map[%d] = %d = \"%s\"\n", n, gameSequence[n], gameSequence[n], led_pin_map[gameSequence[n]], led_text_map[gameSequence[n]]);
        led_on(led_pin_map[gameSequence[n]]);
        delay(FLASH_DELAY);
        led_off(led_pin_map[gameSequence[n]]);
        delay(FLASH_DELAY);
      }

      showingSequenceToUser = false;
      userPositionInSequence = 0;
    } else {
      // Waiting for the user to repeat the sequence back
      update_buttons();

      int userPressed = -1;

      if (debouncerRed.fell()) {
        Serial.println("red");
        led_on(RED_LED);
        delay(FLASH_DELAY);
        led_off(RED_LED);
        userPressed = RED_BUTTON;
      } else  if (debouncerGreen.fell()) {
        Serial.println("green");
        led_on(GREEN_LED);
        delay(FLASH_DELAY);
        led_off(GREEN_LED);
        userPressed = GREEN_BUTTON;
      } else  if (debouncerBlue.fell()) {
        Serial.println("blue");
        led_on(BLUE_LED);
        delay(FLASH_DELAY);
        led_off(BLUE_LED);
        userPressed = BLUE_BUTTON;
      } else if (debouncerYellow.fell()) {
        Serial.println("yellow");
        led_on(YELLOW_LED);
        delay(FLASH_DELAY);
        led_off(YELLOW_LED);
        userPressed = YELLOW_BUTTON;
      }

      if (userPressed > -1) {
        Serial.printf("Evaluate button press:\n userPressed=%d\nuserPositionInSequence=%d\ngameSequence[userPositionInSequence]=%d\nbutton_pin_map[gameSequence[userPositionInSequence]]=%d\nuserPressed != button_pin_map[gameSequence[userPositionInSequence]]=%d\n",userPressed, userPositionInSequence, gameSequence[userPositionInSequence], button_pin_map[gameSequence[userPositionInSequence]], userPressed != button_pin_map[gameSequence[userPositionInSequence]]);
        // A button was pressed, check it against current sequence...
        if (userPressed != button_pin_map[gameSequence[userPositionInSequence]]) {
          // Failed...
          Serial.println("  Fail");
          fail_strobe();
          //tone(BUZZER_PIN, NOTE_F3, 300);
          //
          //delay(300);
          //tone(BUZZER_PIN, NOTE_G3, 500);
          //delay(2500);
          gameInProgress = false;
        } else {
          Serial.println("  Correct");
          userPositionInSequence++;
          if (userPositionInSequence == currentSequenceLength) {
            correct_strobe();
            // User has successfully repeated back the sequence so make it one longer...
            currentSequenceLength++;

            // There's no win scenario here, so just reset...
            if (currentSequenceLength >= MAX_GAME_SEQUENCE) {
              gameInProgress = false;
            }

            // Play a tone...
            //tone(BUZZER_PIN, NOTE_A3, 300);
            //delay(2000);

            showingSequenceToUser = true;
          }
        }
      }
    }
  }
}
