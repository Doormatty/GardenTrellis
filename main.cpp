#include <FastLED.h>

#include "Adafruit_NeoTrellis.h"
#define BRIGHTNESS 24
#define ROWS 8
#define COLS 8

static uint16_t x_pos;
static uint16_t y_pos;
static uint16_t z_pos;

bool freeze = false;
uint8_t selected_palette = 0;

// We're using the x/y dimensions to map to the x/y pixels on the matrix.  We'll
// use the z-axis for "time".  speed determines how fast time moves forward.  Try
// 1 for a very slow moving effect, or 60 for something that ends up looking like
// water.
// uint16_t speed = 1; // almost looks like a painting, moves very slowly
uint16_t speed = 50;

// Scale determines how far apart the pixels in our noise matrix are.  Try
// changing these values around to see how it affects the motion of the display.  The
// higher the value of scale, the more "zoomed out" the noise iwll be.  A value
// of 1 will be so zoomed in, you'll mostly see solid colors.
uint16_t scale = 20;

uint16_t noise[ROWS][COLS];

// matrix of trellis panels
Adafruit_NeoTrellis t_array[ROWS / 4][COLS / 4] = {
    {Adafruit_NeoTrellis(0x30), Adafruit_NeoTrellis(0x2F)},
    {Adafruit_NeoTrellis(0x32), Adafruit_NeoTrellis(0x31)}};

// Pass the matrix to the MultiTrellis
Adafruit_MultiTrellis trellis((Adafruit_NeoTrellis *)t_array, ROWS / 4, COLS / 4);

CRGBPalette16 myPal = RainbowColors_p;
CRGBPalette16 currentPalette(PartyColors_p);

uint8_t colorLoop = 1;

uint32_t color_merge(CRGB input) {
  uint32_t final_color = input.red;
  final_color <<= 8;
  final_color |= input.green;
  final_color <<= 8;
  final_color |= input.blue;
  return final_color;
}

void DrawOneFrame() {
  uint8_t ms = millis();
  int8_t yHueDelta8 = ((int32_t)cos16(ms * (12 / 1)) * (200 / ROWS)) / 32768;
  int8_t xHueDelta8 = ((int32_t)cos16(ms * (26 / 1)) * (200 / COLS)) / 32768;
  uint8_t lineStartHue = ms / 65536;
  for (uint8_t y = 0; y < ROWS; y++) {
    lineStartHue += yHueDelta8;
    uint8_t pixelHue = lineStartHue;
    for (uint8_t x = 0; x < COLS; x++) {
      pixelHue += xHueDelta8;
      // CRGB tmp_color = ColorFromPalette( myPal, pixelHue);
      trellis.setPixelColor(x, y, color_merge(CHSV(pixelHue, 255, 255)));
    }
  }
}

void mapNoiseToLEDsUsingPalette() {
  static uint8_t ihue = 0;
  for (int x = 0; x < COLS; x++) {
    for (int y = 0; y < ROWS; y++) {
      uint8_t index = noise[x][y];
      uint8_t bri = noise[y][x];
      if (colorLoop) {  // if this palette is a 'loop', add a slowly-changing base value
        index += ihue;
      }
      if (bri > 127) {  // brighten up, as the color palette itself often contains the light/dark dynamic range desired
        bri = 255;
      } else {
        bri = dim8_raw(bri * 2);
      }
      trellis.setPixelColor(x, y, color_merge(ColorFromPalette(currentPalette, index, bri)));
    }
  }
  ihue += 1;
}

void create_noise() {
  uint8_t dataSmoothing = 0;
  if (speed < 50) {
    dataSmoothing = 200 - (speed * 4);
  }

  for (int x = 0; x < COLS; x++) {
    for (int y = 0; y < ROWS; y++) {
      uint8_t data = inoise8(x_pos + scale * x, y_pos + scale * y, z_pos);
      data = qsub8(data, 16);  // The range of the inoise8 function is roughly 16-238. These two operations expand those values out to roughly 0..255
      data = qadd8(data, scale8(data, 39));
      if (dataSmoothing) {
        data = scale8(noise[x][y], dataSmoothing) + scale8(data, 256 - dataSmoothing);
      }
      noise[x][y] = data;
    }
  }
  // apply slow drift to X and Y, just for visual variation.
  z_pos += speed;
  x_pos += speed / 8;
  y_pos -= speed / 16;
}

void SetupRandomPalette() {
  // This function generates a random palette that's a gradient
  // between four different colors.  The first is a dim hue, the second is
  // a bright hue, the third is a bright pastel, and the last is
  // another bright hue.  This gives some visual bright/dark variation
  // which is more interesting than just a gradient of different hues.
  currentPalette = CRGBPalette16(
      CHSV(random8(), 255, 32),
      CHSV(random8(), 255, 255),
      CHSV(random8(), 128, 255),
      CHSV(random8(), 255, 255));
}

void SetupBlackAndWhiteStripedPalette() {
  // This function sets up a palette of black and white stripes,
  // using code.  Since the palette is effectively an array of
  // sixteen CRGB colors, the various fill_* functions can be used
  // to set them up.
  // 'black out' all 16 palette entries...
  fill_solid(currentPalette, 16, CRGB::Black);
  // and set every fourth one to white.
  currentPalette[0] = CRGB::White;
  currentPalette[4] = CRGB::White;
  currentPalette[8] = CRGB::White;
  currentPalette[12] = CRGB::White;
}

void SetupPurpleAndGreenPalette() {
  // This function sets up a palette of purple and green stripes.
  CRGB purple = CHSV(HUE_PURPLE, 255, 255);
  CRGB green = CHSV(HUE_GREEN, 255, 255);
  CRGB black = CRGB::Black;

  currentPalette = CRGBPalette16(
      green, green, black, black,
      purple, purple, black, black,
      green, green, black, black,
      purple, purple, black, black);
}

void ChangePalette(char num) {
  switch (num) {
    case 0:
      currentPalette = RainbowColors_p;
      speed = 20;
      scale = 30;
      colorLoop = 1;
      break;
    case 1:
      SetupPurpleAndGreenPalette();
      speed = 10;
      scale = 50;
      colorLoop = 1;
      break;
    case 2:
      SetupBlackAndWhiteStripedPalette();
      speed = 20;
      scale = 30;
      colorLoop = 1;
      break;
    case 3:
      currentPalette = ForestColors_p;
      speed = 8;
      scale = 120;
      colorLoop = 0;
      break;
    case 4:
      currentPalette = CloudColors_p;
      speed = 4;
      scale = 30;
      colorLoop = 0;
      break;
    case 5:
      currentPalette = LavaColors_p;
      speed = 8;
      scale = 50;
      colorLoop = 0;
      break;
    case 6:
      currentPalette = OceanColors_p;
      speed = 20;
      scale = 90;
      colorLoop = 0;
      break;
    case 7:
      currentPalette = PartyColors_p;
      speed = 20;
      scale = 30;
      colorLoop = 1;
      break;
    case 8:
      SetupRandomPalette();
      speed = 20;
      scale = 20;
      colorLoop = 1;
      break;
    case 9:
      SetupRandomPalette();
      speed = 50;
      scale = 50;
      colorLoop = 1;
      break;
    case 10:
      SetupRandomPalette();
      speed = 90;
      scale = 90;
      colorLoop = 1;
      break;
    case 11:
      currentPalette = RainbowStripeColors_p;
      speed = 30;
      scale = 20;
      colorLoop = 1;
      break;
    default:
      break;
  }
}

TrellisCallback callback(keyEvent evt) {
  if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) {  // Pressed
    Serial.print("Pressed ");
    Serial.println(evt.bit.NUM);
    if (evt.bit.NUM >= 0 && evt.bit.NUM <= 7) {
      trellis.setBrightness((evt.bit.NUM + 1) * 15);
    } else if (evt.bit.NUM >= 8 && evt.bit.NUM <= 19) {
      ChangePalette(evt.bit.NUM - 8);

    } else if (evt.bit.NUM == 47) {
      if (colorLoop == 0) {
        colorLoop = 1;
      } else {
        colorLoop = 0;
      }

    } else if (evt.bit.NUM >= 48 && evt.bit.NUM <= 55) {
      // speed changes
      switch (evt.bit.NUM - 48) {
        case 0:
          speed = 2;
          break;
        case 1:
          speed = 4;
          break;
        case 2:
          speed = 8;
          break;
        case 3:
          speed = 16;
          break;
        case 4:
          speed = 32;
          break;
        case 5:
          speed = 64;
          break;
        case 6:
          speed = 128;
          break;
        case 7:
          speed = 256;
          break;
      }

    } else if (evt.bit.NUM >= 56 && evt.bit.NUM <= 63) {
      // scale changes
      switch (evt.bit.NUM - 56) {
        case 0:
          scale = 2;
          break;
        case 1:
          scale = 4;
          break;
        case 2:
          scale = 8;
          break;
        case 3:
          scale = 16;
          break;
        case 4:
          scale = 32;
          break;
        case 5:
          scale = 64;
          break;
        case 6:
          scale = 128;
          break;
        case 7:
          scale = 256;
          break;
      }
    }
  }
  return 0;
}

void setup() {
    Serial.begin(9600);
  if (!trellis.begin()) {
    while (1) delay(1);
  }
  trellis.setBrightness(BRIGHTNESS);
  x_pos = random16();
  y_pos = random16();
  z_pos = random16();

  // activate all keys and set callbacks
  for (int i = 0; i < 64; i++) {
    trellis.activateKey(i, SEESAW_KEYPAD_EDGE_RISING);
    trellis.registerCallback(i, callback);
  }

  currentPalette = RainbowColors_p;
  speed = 20;
  scale = 30;
  colorLoop = 1;
}

void loop() {
  EVERY_N_MILLISECONDS(20) {
    trellis.read();
  }
  create_noise();
  mapNoiseToLEDsUsingPalette();
  // DrawOneFrame();
  trellis.show();
}
