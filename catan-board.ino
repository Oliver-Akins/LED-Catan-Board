#include <FastLED.h>

// The digital input pin that the button is connected to
#define BUTTON_PIN 7

// The digital output pin that the LED data signal is coming out of
#define LED_PIN 3

// Whether or not to save the board's state to the EEPROM after we're done
// randomizing.
#define SAVE_BOARD_STATE false

// A tool used to identify which LEDs are in what program group, this will
// cause each group to light up independantly of every other group, and then
// go to the next one.
#define IDENTIFY_GROUPS false

// Whether or not the neutral 3:1 ports should be a static colour, or cycling
// through the colour spectrum
#define USE_RAINBOW_PORTS false

// Whether or not to allow the board to randomize again immediately after
// randomizing once.
#define ALLOW_DISCO_RANDOMIZATION false

// The brightness of the LEDs, can be a value between 0(off) and 255(full)
#define BRIGHTNESS 255

// The delay between lighting up each segment of light. Can be any non-negative
// value. Setting to 0 disables delay and will light up all of the LEDs
// simultaneously.
#define ILLUMINATE_DELAY 0

// The colours for the different resource types. This expects three comma-separated
// values representing the Red, Green, Blue values of the light. Each of the
// three numbers can be anywhere between 0 and 255, with 0,0,0 being off and
// 255,255,255 being white
#define light_off 0, 0, 0
#define grain_colour 255, 255, 0//199, 72, 245 // yellow
#define wool_colour 200, 200, 200 // white
#define wood_colour 0, 255, 0 // green
#define bricks_colour 255, 0, 0 // red
#define ore_colour 0, 0, 255 // blue
#define desert_colour 0, 255, 55 // Seafoam green
#define neutral_colour 0, 255, 255//255, 192, 203 // pink


//===========================================================================//
// Do not change ANYTHING below this unless you know what you are doing

#define NUM_LEDS 123
CRGB leds[NUM_LEDS];

#define HONEYCOMB_OFFSET 9
#define PORT_OFFSET 0

#define GRAIN 1
#define WOOL 2
#define WOOD 3
#define BRICKS 4
#define ORE 5
#define DESERT 6
#define NEUTRAL 7
#define PORT_COUNT 9
#define HONEYCOMB_COUNT 19

// DO NOT CHANGE THESE
#define NUM_LED_GROUPS 28
#define LED_GROUP_SIZE 6
int led_groups[NUM_LED_GROUPS][LED_GROUP_SIZE] = {
	{0, -1, -1, -1, -1, -1}, // Port 1
	{1, -1, -1, -1, -1, -1}, // Port 2
	{2, -1, -1, -1, -1, -1}, // Port 3
	{3, -1, -1, -1, -1, -1}, // Port 4
	{4, -1, -1, -1, -1, -1}, // Port 5
	{5, -1, -1, -1, -1, -1}, // Port 6
	{6, -1, -1, -1, -1, -1}, // Port 7
	{7, -1, -1, -1, -1, -1}, // Port 8
	{8, -1, -1, -1, -1, -1}, // Port 9
	{9, 10, 11, 12, 13, 14}, // Hex 1
	{15, 16, 17, 18, 19, 20}, // Hex 2
	{21, 22, 23, 24, 25, 26}, // Hex 3
	{27, 28, 29, 30, 31, 32}, // Hex 4
	{33, 34, 35, 36, 37, 38}, // Hex 5
	{39,40,41,42,43,44}, // Hex 6
	{45,46,47,48,49,50}, // Hex 7
	{51,52,53,54,55,56}, // Hex 8
	{57,58,59,60,61,62}, // Hex 9
	{63,64,65,66,67,68}, // Hex 10
	{69,70,71,72,73,74}, // Hex 11
	{75,76,77,78,79,80}, // Hex 12
	{81,82,83,84,85,86}, // Hex 13
	{87,88,89,90,91,92}, // Hex 14
	{93,94,95,96,97,98}, // Hex 15
	{99,100,101,102,103,104}, // Hex 16
	{105,106,107,108,109,110}, // Hex 17
	{111,112,113,114,115,116}, // Hex 18
	{117,118,119,120,121,122} // Hex 19
};


// The distribution of all resources within the hex grid
int honeycomb[HONEYCOMB_COUNT] = {
	GRAIN, GRAIN, GRAIN, GRAIN,
	WOOL, WOOL, WOOL, WOOL,
	WOOD, WOOD, WOOD, WOOD,
	BRICKS, BRICKS, BRICKS,
	ORE, ORE, ORE,
	DESERT
};

// The distribution of all of the ports that allow trading for resources
int ports[PORT_COUNT] = {
	GRAIN, WOOL, WOOD, BRICKS, ORE,
	NEUTRAL, NEUTRAL, NEUTRAL, NEUTRAL
};

/**
 * These are the ports which cycle through the HSV spectrum to indicate that
 * they can be used to trade 3 of anything to 1 of something else
 */
#if USE_RAINBOW_PORTS
int rainbow_ports[4] = {-1,-1,-1,-1};
#define HUE_DELTA 3;
#define FPS 60
int hue = 0;
#endif

/**
 * Implemented using the Fisher-Yates shuffle algorithm
 * https://en.wikipedia.org/wiki/Fisher%E2%80%93Yates_shuffle
 */
void shuffle_array(int* array, int size) {
	for (int i = size-1; i > 0; i--) {
		int swap_index = random(i + 1);
		int tmp = array[i];
		array[i] = array[swap_index];
		array[swap_index] = tmp;
	};
};


#if SAVE_BOARD_STATE
#include <EEPROM.h>
/**
 * Reads the previously written board state from EEPROM
 */
void load_state() {
	for (int i = 0; i < NUM_LEDS; i++) {
		byte red = EEPROM.read(i);
		byte green = EEPROM.read(i + 1);
		byte blue = EEPROM.read(i + 2);
		leds[i].red = red;
		leds[i].green = green;
		leds[i].blue = blue;
	}
};

/**
 * Writes the current board state to the EEPROM memory onboard, this allows the
 * board state to be resumed if power to the board is lost.
 */
void save_state() {
	for (int i = 0; i < NUM_LEDS; i++) {
		// Update the values in the EEPROM, using update here instead of write
		// does an automatic assertion the two values are different before
		// writing to the storage, helping to decrease the number of writes
		// made, extending the life-span of the memory.
		EEPROM.update(i, leds[i].red);
		EEPROM.update(i + 1, leds[i].green);
		EEPROM.update(i + 2, leds[i].blue);
	}
};
#endif

/** Lights up all of the LEDs in the group with the provided RGB values */
void illuminate(int r, int g, int b, int* led_group) {
	for (int i = 0; i < LED_GROUP_SIZE; i++) {
		if (led_group[i] >= 0) {
			leds[led_group[i]].setRGB(r, g, b);
		};
	};
	FastLED.show();
};

/** Changes the colour of the LED group to be that of type */
void change_colour(int group, int type) {
	switch (type) {
		case WOOL:
			illuminate(wool_colour, led_groups[group]);
			break;
		case WOOD:
			illuminate(wood_colour, led_groups[group]);
			break;
		case BRICKS:
			illuminate(bricks_colour, led_groups[group]);
			break;
		case GRAIN:
			illuminate(grain_colour, led_groups[group]);
			break;
		case ORE:
			illuminate(ore_colour, led_groups[group]);
			break;
		case DESERT:
			illuminate(desert_colour, led_groups[group]);
			break;
		case NEUTRAL:
			#if USE_RAINBOW_PORTS
			// Cycle through the rainbow for the ports
			for (int i = 0; i < 4; i++) {
				if (rainbow_ports[i] < 0) {
					rainbow_ports[i] = led_groups[group][0];
					break;
				};
			};
			#else
			// Use a static colour for the neutral ports
			illuminate(neutral_colour, led_groups[group]);
			#endif
			break;
		default:
			illuminate(light_off, led_groups[group]);
	};
};

void randomize_ports() {
	#if USE_RAINBOW_PORTS
		// Reset the rainbow ports when they are in use so that they don't stay
		// as the same ports on every randomization trigger
		if (rainbow_ports[i] < 0) {
			rainbow_ports[i] = -1;
			break;
		};
	#endif

	// Shuffle the port distribution array
	shuffle_array(ports, PORT_COUNT);

	// Light up all of the ports in the required colours
	for (int i = 0; i < PORT_COUNT; i++) {
		change_colour(PORT_OFFSET + i, ports[i]);

		#if ILLUMINATE_DELAY > 0
			delay(ILLUMINATE_DELAY);
		#endif
	};
};

void randomize_honeycomb() {
	// Shuffle the array of honeycomb distribution
	shuffle_array(honeycomb, HONEYCOMB_COUNT);

	// Light up all of the honeycombs with the specific colour
	for (int i = 0; i < HONEYCOMB_COUNT; i++) {
		change_colour(HONEYCOMB_OFFSET + i, honeycomb[i]);

		#if ILLUMINATE_DELAY > 0
			delay(ILLUMINATE_DELAY);
		#endif
	};
};

void randomize_LEDs() {
	// all_off();
	randomize_ports();
	randomize_honeycomb();
};


#if USE_RAINBOW_PORTS
// The function that is used to change all of the rainbow port colours to the
// next hue step
void rainbowify() {
	for (int i = 0; i < 4; i++) {
		if (rainbow_ports[i] >= 0) {
			leds[rainbow_ports[i]].setHue(hue);
		};
	};
	FastLED.show();
	hue += HUE_DELTA;
};
#endif

void setup() {

	// Register the button's board pin as input so that we can properly read
	// from it.
	pinMode(BUTTON_PIN, INPUT);

	// seed the RNG so we actually get randomized positions
	randomSeed(analogRead(0));

	// Setup the LED controller
	FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
	FastLED.setBrightness(BRIGHTNESS);

	// Determine how we are wanting to handle the LEDs on startup
	#if !IDENTIFY_GROUPS
		#if SAVE_BOARD_STATE
			load_state();
			FastLED.show();
		#else
			randomize_LEDs();
		#endif
	#else
		for (int g = 0; g < NUM_LED_GROUPS; g++) {
			illuminate(0, 255, 0, led_groups[g]);
			delay(1000);
			illuminate(0, 0, 0, led_groups[g]);
		};
	#endif
};

// the time indicator of the last frame rendered
unsigned long previous = 0;

// whether or not the button is currently being held
bool held = false;
void loop() {

	// Ensuring that the randomization logic doesn't happen multiple times in
	// one button press (even though just holding the button will still
	// randomize repeatedly, just with a delay between each time)
	if (!held) {
		// Ensure the button is pressed
		if (digitalRead(BUTTON_PIN) == HIGH) {
			held = true;
			randomize_LEDs();
			delay(500);
			held = false;
		}
	};

	// Change the colour of the rainbow ports if the required amount of time
	// has passed since the last frame.
	#if USE_RAINBOW_PORTS
	if (millis() > previous + ( 1000 / FPS )) {
		rainbowify();
		previous = millis();
	};
	#endif
};