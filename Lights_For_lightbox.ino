// ============================================================
//  Addressable LED Full Control — WS2812B
//  v2 — fixed phantom button presses from LED EMI noise
// ============================================================
//  Wiring:
//    D6  -> LED data line (+ 300-500Ω resistor)
//    A0  -> Potentiometer wiper (left=GND, right=5V)
//    D2  -> Button MODE  (other pin to GND)
//    D3  -> Button A     (other pin to GND)
//    D4  -> Button B     (other pin to GND)
//    Optional: 100nF ceramic cap across each button pin + GND
//
//  Button behaviour per mode:
//  ┌───────────┬─────────────────────┬──────────────────────┐
//  │   Mode    │      Button A       │      Button B        │
//  ├───────────┼─────────────────────┼──────────────────────┤
//  │  WHITE    │  Warmer             │  Cooler              │
//  │  PARTY    │  Next preset        │  Shift color         │
//  │  COLOR    │  Toggle auto-cycle  │  Step hue            │
//  └───────────┴─────────────────────┴──────────────────────┘
// ============================================================

#include <FastLED.h>

#define DATA_PIN       6
#define NUM_LEDS       11        // ← change to your strip length
#define LED_TYPE       WS2812B
#define COLOR_ORDER    GRB
#define POT_PIN        A0
#define BTN_MODE       2
#define BTN_A          3
#define BTN_B          4
#define MIN_BRIGHTNESS 5
#define MAX_BRIGHTNESS 255

// ── Debounce ──────────────────────────────────────────────
// DEBOUNCE_MS: minimum ms between valid presses.
// CONFIRM_MS:  re-check delay to reject EMI spikes.
// Raise DEBOUNCE_MS further (e.g. 800) if still jumping.
#define DEBOUNCE_MS    600
#define CONFIRM_MS     12

CRGB leds[NUM_LEDS];

// ── Modes ─────────────────────────────────────────────────
enum Mode { WHITE, PARTY, COLOR, NUM_MODES };
uint8_t mode = WHITE;

// ── White mode ────────────────────────────────────────────
int warmth = 128;
#define WARMTH_STEP 25

// ── Party mode ────────────────────────────────────────────
uint8_t partyPreset = 0;
uint8_t partyHue    = 0;
#define NUM_PRESETS    6
#define PARTY_HUE_STEP 32

// ── Color mode ────────────────────────────────────────────
uint8_t colorHue     = 0;
bool    colorCycling = false;
#define HUE_STEP 20

// ── Button state ──────────────────────────────────────────
uint32_t lastPress[3]   = {0, 0, 0};
bool     lastState[3]   = {HIGH, HIGH, HIGH}; // unpressed = HIGH (pullup)

// ── Noise-hardened button press detector ──────────────────
// 1. Checks debounce window has elapsed
// 2. Waits CONFIRM_MS and re-reads pin — rejects EMI spikes
// 3. Requires pin to return HIGH before accepting next press
//    (prevents held-button repeat triggering)
bool btnPressed(uint8_t pin, uint8_t idx) {
  bool currentState = digitalRead(pin);

  // Detect falling edge (HIGH -> LOW) and past debounce window
  if (lastState[idx] == HIGH && currentState == LOW) {
    if (millis() - lastPress[idx] > DEBOUNCE_MS) {
      delay(CONFIRM_MS);                     // short wait
      if (digitalRead(pin) == LOW) {         // still LOW = real press
        lastPress[idx] = millis();
        lastState[idx] = LOW;
        return true;
      }
    }
  }

  // Track when pin returns HIGH (released) so next press is clean
  if (currentState == HIGH) {
    lastState[idx] = HIGH;
  }

  return false;
}

// ─────────────────────────────────────────────────────────
void setup() {
  delay(500);
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS)
         .setCorrection(TypicalLEDStrip);
  FastLED.clear();
  pinMode(BTN_MODE, INPUT_PULLUP);
  pinMode(BTN_A,    INPUT_PULLUP);
  pinMode(BTN_B,    INPUT_PULLUP);
}

// ── Potentiometer brightness ──────────────────────────────
void updateBrightness() {
  static int smoothed = 512;
  int raw  = analogRead(POT_PIN);
  smoothed = (smoothed * 7 + raw) / 8;
  FastLED.setBrightness(map(smoothed, 0, 1023, MIN_BRIGHTNESS, MAX_BRIGHTNESS));
}

// ── Button handler ────────────────────────────────────────
void handleButtons() {
  if (btnPressed(BTN_MODE, 0)) {
    mode = (mode + 1) % NUM_MODES;
    FastLED.clear();
    colorCycling = false;
  }

  switch (mode) {
    case WHITE:
      if (btnPressed(BTN_A, 1)) warmth = constrain(warmth + WARMTH_STEP, 0, 255);
      if (btnPressed(BTN_B, 2)) warmth = constrain(warmth - WARMTH_STEP, 0, 255);
      break;

    case PARTY:
      if (btnPressed(BTN_A, 1)) {
        partyPreset = (partyPreset + 1) % NUM_PRESETS;
        FastLED.clear();
      }
      if (btnPressed(BTN_B, 2)) partyHue += PARTY_HUE_STEP;
      break;

    case COLOR:
      if (btnPressed(BTN_A, 1)) colorCycling = !colorCycling;
      if (btnPressed(BTN_B, 2)) { colorHue += HUE_STEP; colorCycling = false; }
      break;
  }
}

// ─────────────────────────────────────────────────────────
void loop() {
  updateBrightness();
  handleButtons();

  switch (mode) {
    case WHITE: runWhite(); break;
    case PARTY: runParty(); break;
    case COLOR: runColor(); break;
  }

  FastLED.show();
}

// ══════════════════════════════════════════════════════════
//  WHITE MODE
// ══════════════════════════════════════════════════════════
void runWhite() {
  uint8_t w = (uint8_t)warmth;
  CRGB c = CRGB(
    map(w, 0, 255, 155, 255),
    map(w, 0, 255, 195, 160),
    map(w, 0, 255, 255,  40)
  );
  fill_solid(leds, NUM_LEDS, c);
  delay(20);
}

// ══════════════════════════════════════════════════════════
//  COLOR MODE
// ══════════════════════════════════════════════════════════
void runColor() {
  if (colorCycling) colorHue++;
  fill_solid(leds, NUM_LEDS, CHSV(colorHue, 255, 255));
  delay(20);
}

// ══════════════════════════════════════════════════════════
//  PARTY MODE
// ══════════════════════════════════════════════════════════
void runParty() {
  switch (partyPreset) {
    case 0: partyRainbowWave();  break;
    case 1: partyColorChase();   break;
    case 2: partyTwinkle();      break;
    case 3: partyFire();         break;
    case 4: partyBouncingBall(); break;
    case 5: partyMeteorRain();   break;
  }
}

// ── Party preset 1: Rainbow Wave ──────────────────────────
void partyRainbowWave() {
  static uint8_t offset = 0;
  for (int i = 0; i < NUM_LEDS; i++)
    leds[i] = CHSV(partyHue + offset + (i * 256 / NUM_LEDS), 255, 255);
  offset += 2;
  delay(20);
}

// ── Party preset 2: Color Chase ───────────────────────────
void partyColorChase() {
  static int pos = 0;
  for (int i = 0; i < NUM_LEDS; i++) leds[i].nscale8(200);
  leds[pos] = CHSV(partyHue, 255, 255);
  pos = (pos + 1) % NUM_LEDS;
  delay(30);
}

// ── Party preset 3: Twinkle ───────────────────────────────
void partyTwinkle() {
  for (int i = 0; i < NUM_LEDS; i++) leds[i].nscale8(230);
  for (int n = 0; n < random8(1, 4); n++) {
    int pos = random16(NUM_LEDS);
    leds[pos] = CHSV(partyHue + random8(40), 220, 255);
  }
  delay(40);
}

// ── Party preset 4: Fire ──────────────────────────────────
#define FIRE_COOLING   55
#define FIRE_SPARKING  120

void partyFire() {
  static byte heat[NUM_LEDS];
  for (int i = 0; i < NUM_LEDS; i++)
    heat[i] = qsub8(heat[i], random8(0, ((FIRE_COOLING * 10) / NUM_LEDS) + 2));
  for (int k = NUM_LEDS - 1; k >= 2; k--)
    heat[k] = (heat[k-1] + heat[k-2] + heat[k-2]) / 3;
  if (random8() < FIRE_SPARKING) {
    int y = random8(7);
    heat[y] = qadd8(heat[y], random8(160, 255));
  }
  for (int j = 0; j < NUM_LEDS; j++) {
    CRGB base = HeatColor(heat[j]);
    uint8_t blend = 255 - heat[j];
    CRGB tint = CHSV(partyHue, 255, blend / 2);
    leds[j] = base + tint;
  }
  delay(15);
}

// ── Party preset 5: Bouncing Ball ─────────────────────────
void partyBouncingBall() {
  static float pos = 0;
  static float velocity = 0.8;
  static bool movingUp = true;
  for (int i = 0; i < NUM_LEDS; i++) leds[i].nscale8(180);
  int p = (int)pos;
  if (p >= 0 && p < NUM_LEDS) {
    leds[p] += CHSV(partyHue, 255, 255);
    if (p > 0)            leds[p-1] += CHSV(partyHue, 200, 120);
    if (p < NUM_LEDS - 1) leds[p+1] += CHSV(partyHue, 200, 120);
  }
  pos += movingUp ? velocity : -velocity;
  velocity = max(0.2f, velocity - 0.005f);
  if (pos >= NUM_LEDS - 1) { movingUp = false; velocity = 0.8; }
  if (pos <= 0)             { movingUp = true;  velocity = 0.8; }
  delay(16);
}

// ── Party preset 6: Meteor Rain ───────────────────────────
#define METEOR_SIZE        8
#define METEOR_TRAIL_DECAY 64

void partyMeteorRain() {
  static int meteorPos = 0;
  for (int i = 0; i < NUM_LEDS; i++)
    if (random8(10) > 4) leds[i].nscale8(255 - METEOR_TRAIL_DECAY);
  for (int j = 0; j < METEOR_SIZE; j++) {
    int drawPos = meteorPos - j;
    if (drawPos >= 0 && drawPos < NUM_LEDS) {
      uint8_t bright = 255 - (j * (255 / METEOR_SIZE));
      leds[drawPos] = CHSV(partyHue, 255, bright);
    }
  }
  meteorPos++;
  if (meteorPos > NUM_LEDS + METEOR_SIZE) meteorPos = 0;
  delay(20);
}