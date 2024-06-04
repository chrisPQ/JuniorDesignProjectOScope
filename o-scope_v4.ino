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
const float vScale_min = 2.0;  // 2 volts per division
const float vScale_max = 5.0;  // 5 volts per division

float vScale = 2.0;  // Start at the minimum vertical scale
long hScale = 40;
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

Encoder trigger_Enc(21, 22);
int triggerIndex = 0;
Encoder scale_Enc(19, 20);
#define scale_select 18

int h_scale_min = 1;
int h_scale_max = 200;

double trigger;
bool trig_flag = false;

unsigned long int last = 0;
bool first_print = true;
volatile uint16_t *prev_data;

void setup() {
  pinMode(scale_select, INPUT);
  Serial.begin(1000000);
  while (!Serial && millis() < 5000)
    ;

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(readPin_adc_0, INPUT_DISABLE);
  pinMode(readPin_adc_1, INPUT_DISABLE);

  adc->adc0->setAveraging(1);
  adc->adc0->setResolution(10);
  adc->adc1->setAveraging(1);
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
  tft.setRotation(1);
  tft.fillScreen(ILI9341_BLACK);  // Clear the screen once
  drawFrame();
}

void loop() {
  unsigned long int interval = 1;
if (digitalRead(scale_select) == false) {
  if(millis()-last >= interval){
      vScale = scale_Enc.read();  // Assume encoder gives values in tenths of volts per division
    vScale = constrain(vScale, vScale_min, vScale_max);  // Constrain to min and max values
 // }
 Serial.print("\nVscale: ");
 Serial.print(vScale);
 last = millis();
  }
}

   // // Check if the button has been released long enough to edit hScale
  if (digitalRead(scale_select)) {
    if(millis()-last >= interval){
    hScale = scale_Enc.read();
     Serial.print("\nhscale: ");
 Serial.print(hScale);
    last = millis();
    }
  }




  drawFrame();  // initialize blank frame
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


  // Check if the button has been pressed long enough to edit vScale
  //if (buttonIsPressed && (millis() - buttonPressStartTime >= pressThreshold)) {


  hScale = clamp(hScale, h_scale_min, h_scale_max);

  volatile uint16_t *pbuffer = pabdma->bufferLastISRFilled();
  volatile uint16_t *end_pbuffer = pbuffer + pabdma->bufferCountLastISRFilled();

  if ((uint32_t)pbuffer >= 0x20200000u) {
    arm_dcache_delete((void *)pbuffer, sizeof(dma_adc_buff1));
  }

  int16_t x = 0;

  tft.drawFastHLine(0, trigger, 280, ILI9341_YELLOW);  // draw trigger line
  volatile uint16_t triggerIndex = get_trigger_index(pabdma);
  float y_prev;
  float x_prev;
  for (volatile uint16_t *i = pbuffer; i < pbuffer + 320; i++) {
    float y1 = ((((*i * 3.3) / 1024) / 1.65));
    float y2 = (y1 - 1.17) * 20 * (-1);
    float y3 = mapVoltageToRange(y2);
    if (adc_num1 == 0) {
      if (!first_print) {
        tft.drawLine(x_prev, y_prev, x * 100 / hScale, y3, ILI9341_GREEN);
      } else {
        tft.drawLine(x_prev, y_prev, x * 100 / hScale, y3, ILI9341_GREEN);
      }
    } else {
      if (!first_print) {
        tft.drawLine(x_prev, y_prev, x * 100 / hScale, y3, ILI9341_BLUE);
      } else {
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
  tft.println(" V/div");
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

  for (volatile uint16_t *i = screenBuffer + 140; i < end_pbuffer - 140; i++) {
    // Serial.print("\ntrigger level: ");
    // Serial.print(trigger);
    // Serial.print(",   current value: ");
    // Serial.println(*i);
    if (*i == trigger) {
      triggerIndex = i;
      // Serial.print("\nTriggered! ");
      // Serial.print("Trigger Index value: ");
      // Serial.println(*triggerIndex);
      trig_flag = true;
      break;
    }
  }
  return triggerIndex;
}

// Function to map voltage to the desired range and return an integer
int mapVoltageToRange(float voltage) {
  float max = 20.0;
  // Ensure the voltage is within the expected range
  if (voltage < (-1)*max) voltage = (-1)*max;
  if (voltage > max) voltage = max;
  int max_pixel = 240-(24*vScale);
  int min_pixel = 24*vScale;
  // Map the voltage to the range 0 to 240
  int mappedValue = (int)(((voltage + max) / (2*max)) * max_pixel) + min_pixel;
  return mappedValue;
}

