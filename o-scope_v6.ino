#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include <ADC.h>
#include <AnalogBufferDMA.h>
#include <DMAChannel.h>
#include <Encoder.h>

// For the Adafruit shield, these are the default.
#define TFT_DC 9
#define TFT_CS 10

#define COLUMNS 15
#define ROWS 11

// Vertical scale settings
const float vScale_min = 1.0;  // 2 volts per division
const float vScale_max = 5.0;  // 5 volts per division

float vScale = 2.0;  // Start at the minimum vertical scale
volatile long hScale = 1;
long prev_h;
long prev_v;

// Variables to store the last time the button state changed
unsigned long buttonPressStartTime = 0;
unsigned long buttonReleaseStartTime = 0;

// Time thresholds in milliseconds
const unsigned long pressThreshold = 1000;    // Time the button must be held to edit vScale
const unsigned long releaseThreshold = 1000;  // Time the button must be released to edit hScale

// Variable to track the current state of the button
bool buttonWasPressed = false;
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, 11, 13, 6, 12);

#if defined(ADC_USE_DMA) && defined(ADC_USE_TIMER)

const int readPin_adc_0 = 14;
const int readPin_adc_1 = 15;

ADC *adc = new ADC();

const uint32_t buffer_size = 1600;
DMAMEM static volatile uint16_t __attribute__((aligned(32))) dma_adc_buff1[buffer_size];
DMAMEM static volatile uint16_t __attribute__((aligned(32))) dma_adc_buff2[buffer_size];
AnalogBufferDMA abdma1(dma_adc_buff1, buffer_size, dma_adc_buff2, buffer_size);

DMAMEM static volatile uint16_t __attribute__((aligned(32))) dma_adc_buff2_1[buffer_size];
DMAMEM static volatile uint16_t __attribute__((aligned(32))) dma_adc_buff2_2[buffer_size];
AnalogBufferDMA abdma2(dma_adc_buff2_1, buffer_size, dma_adc_buff2_2, buffer_size);

elapsedMillis elapsed_sinc_last_display;

Encoder trigger_Enc(22, 21);
int triggerIndex = 0;
//Encoder scale_Enc(19, 20);
#define scale_select 18

int h_scale_min = 1;
int h_scale_max = 114;

double trigger;
bool trig_flag = false;

unsigned long int last = 0;
bool first_print = true;
volatile uint16_t *prev_data;

#define chA 19
#define chB 20
int aState;
int aLastState;
float hcounter = 1;
float vcounter = 1;
#define scale_select 18
volatile int buttonState = 0;


//Variables for low-pass filtering
const float alpha = 0.8;            //<- Making this valu lower intensifies filtering effect and vice-versa
double data_filtered[] = { 0, 0 };  // data_filtered[n] is where the most recent filtered value can be accessed
const int n = 1;


void setup() {
  pinMode(scale_select, INPUT);
  pinMode(chA, INPUT);
  pinMode(chB, INPUT);

  aLastState = digitalRead(chA);
  attachInterrupt(digitalPinToInterrupt(chA), update_encoder, CHANGE);
  Serial.begin(1000000);
  while (!Serial && millis() < 5000)
    ;

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(readPin_adc_0, INPUT_DISABLE);
  pinMode(readPin_adc_1, INPUT_DISABLE);

  adc->adc0->setAveraging(1);
  adc->adc0->setResolution(10);
  adc->adc1->setAveraging(80);
  adc->adc1->setResolution(10);

  abdma1.init(adc, ADC_0);
  abdma1.userData(0);
  abdma2.init(adc, ADC_1);
  abdma2.userData(0);

  adc->adc0->startSingleRead(readPin_adc_0);
  adc->adc0->startTimer(500000);
  adc->adc1->startSingleRead(readPin_adc_1);
  adc->adc1->startTimer(500000);

  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);  // Clear the screen once
  drawFrame();
}

void loop() {
  buttonState = digitalRead(scale_select);  // Read button state
  drawFrame();                              // initialize blank frame
  if (abdma1.interrupted() && abdma2.interrupted()) {
    ProcessAnalogData(&abdma1, 0);
    ProcessAnalogData(&abdma2, 1);
    elapsed_sinc_last_display = 0;
  }
  if (abdma1.interrupted()) {
    ProcessAnalogData(&abdma1, 0);
    elapsed_sinc_last_display = 0;
  }
  if (abdma2.interrupted()) {
    ProcessAnalogData(&abdma2, 1);
    elapsed_sinc_last_display = 0;
  }
}

void ProcessAnalogData(AnalogBufferDMA *pabdma, int8_t adc_num1) {
  trigger = trigger_Enc.read();
  trigger= clamp(trigger,-20,20);
  hScale = clamp(hScale, h_scale_min, h_scale_max);
  vScale = clamp(vScale, vScale_min, vScale_max);

  volatile uint16_t *pbuffer = pabdma->bufferLastISRFilled();
  volatile uint16_t *end_pbuffer = pbuffer + pabdma->bufferCountLastISRFilled();

  if ((uint32_t)pbuffer >= 0x20200000u) {
    arm_dcache_delete((void *)pbuffer, sizeof(dma_adc_buff1));
  }

  int16_t x = 0;
  tft.drawFastHLine(0, mapVoltageToRange(trigger), 280, ILI9341_YELLOW);  // draw trigger line

  // Only set the trigger index if we found a valid trigger point
  volatile uint16_t triggerIndex = get_trigger_index(pabdma);

  if (triggerIndex != 65535) {
    pbuffer += (triggerIndex - (160));
  }

  float y_prev;
  float x_prev;
Serial.print("\nTrigger index:");
Serial.print(triggerIndex);
  for (volatile uint16_t *i = pbuffer; i < pbuffer + 320; i++) {
    float y1 = ((((*i * 3.3) / 1024) / 1.65));
    float y2 = (y1 - 1.17) * 20 * (-1);
    float y3 = mapVoltageToRange(y2);

    if (adc_num1 == 0) {
      if (!first_print) {
        tft.drawLine(x_prev, y_prev, x * 100 / hScale, y3, ILI9341_GREEN);
      }
    } else {
      if (!first_print) {
        tft.drawLine(x_prev, y_prev, x * 100 / hScale, y3, ILI9341_BLUE);
      }
    }
    x_prev = x * 100 / hScale;
    x++;
    y_prev = y3;
  }

  pabdma->clearInterrupt();

  trig_flag = false;
  first_print = false;
  if (!first_print) {
    prev_data = pbuffer;
  }
  delay(300);
}

int mapVoltageToRange(float voltage) {
  const float screenHeight = 240;       // Screen height in pixels
  const float centerY = screenHeight / 2;  // Center Y position (middle of the screen)
  const float vDivisions = 10;           // Number of vertical divisions on the screen

  // Calculate pixels per division
  float pixelsPerDiv = screenHeight / vDivisions;

  // Calculate the pixel value based on voltage and vScale
  float mappedValue = centerY - (voltage / vScale)*0.5 * pixelsPerDiv;

  // Ensure the mapped value is within the screen bounds
  if (mappedValue < 0) {
    mappedValue = 0;
  } else if (mappedValue > screenHeight) {
    mappedValue = screenHeight;
  }

  return static_cast<int>(mappedValue);
}


void drawFrame() {
  tft.fillRect(0, 0, tft.width(), tft.height(), ILI9341_BLACK);
  // Draw the grid using individual pixels
  for (int16_t i = 0; i < COLUMNS; i++) {
    for (int16_t j = 0; j < ROWS; j++) {
      for (int16_t k = -2; k <= 2; k++) {
        tft.drawPixel(20 * i, j * 20 + k, ILI9341_WHITE);  // Vertical pixels
        tft.drawPixel(20 * i + k, j * 20, ILI9341_WHITE);  // Horizontal pixels
      }
    }
  }

  // Draw the scale indicators
  tft.setCursor(0, ROWS * 20);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_YELLOW);
  tft.print("V Scale: ");
  tft.print(vScale);
  //tft.println(" V/div");
  tft.setCursor(100, ROWS * 20);
  tft.print("H Scale: ");
  tft.println(hScale);
}


#else
void setup() {}
void loop() {}
#endif

int16_t clamp(int16_t val, int min, int max) {
  if (val < min) {
    val = min;
  }

  if (val > max) {
    val = max;
  }
  return val;
}

volatile uint16_t get_trigger_index(AnalogBufferDMA *dma_buffer_in) {
  volatile uint16_t *screenBuffer = dma_buffer_in->bufferLastISRFilled();
  volatile uint16_t *end_pbuffer = screenBuffer + dma_buffer_in->bufferCountLastISRFilled();
  volatile uint16_t *triggerIndex = screenBuffer + 140;


  for (volatile uint16_t *i = screenBuffer + 160; i < end_pbuffer - 160; i++) {
    float y1 = ((((*i * 3.3) / 1024) / 1.65));
    float y2 = (y1 - 1.17) * 20 * (-1);
      Serial.print("\nTrigger voltage: ");
  Serial.print(trigger);
  Serial.print("  Current voltage: ");
  Serial.print(y2);
    if (abs(trigger -y2) < 0.5) {
      Serial.print("\n\nTriggered\n\n");

      triggerIndex = i;
      trig_flag = true;
      return triggerIndex;
    }
  }
  trig_flag = false;
  return -1;
}



void update_encoder() {
  int aState = digitalRead(chA);  // Read the "current" state of channel A

  // If the state of channel A has changed, a pulse has occurred
  if (aState != aLastState) {
    // Determine the direction of rotation
    if (digitalRead(chB) != aState) {
      if (buttonState) {
        Serial.print("\nButton pushed! Increment hScale.");
        hScale++;
      } else {
        Serial.print("\nButton not pushed! Increment vScale.");
        vScale++;
      }
    } else {
      if (buttonState) {
        Serial.print("\nButton pushed! Decrement hScale.");
        hScale--;
      } else {
        Serial.print("\nButton not pushed! Decrement vScale.");
        vScale--;
      }
    }
  }
  // Update the last state
  aLastState = aState;
}

double filter_data(float data_in) {
  // if the received serial command tells us to send sensor data, take a measurement from the photodiode,
  // pass it through the lowpass filter, and send it back to the PC/
  //  if (c == send_data) {
  // Retrieve Data


  // Low Pass Filter
  data_filtered[n] = alpha * data_in + (1 - alpha) * data_filtered[n - 1];

  // Store the last filtered data in data_filtered[n-1]
  data_filtered[n - 1] = data_filtered[n];
  return data_filtered[n];
}
