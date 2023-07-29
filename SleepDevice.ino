// SleepDevice.ino
//
// Converts a LilyGo T-Watch 2020 V3 into a sleep therapy device for positional obstructive sleep apnea (POSA) and snoring.
// Monitors how long the wearer has been on their back, and vibrates periodically to encourage them to roll onto their side.
// A similar concept to Somnibel (somnibel.eu), Night Shift (nightshifttherapy.com) etc.
//
// Uses TTGO_TWatch_Library-1.4.3 from https://github.com/Xinyuan-LilyGO/TTGO_TWatch_Library

#include "config.h"

#define MIN_TO_S 60         // Minutes to seconds
#define S_TO_mS 1000ULL     // Seconds to milliseconds
#define S_TO_uS 1000000ULL  // Seconds to microseconds
#define TIME_TO_SLEEP  120  // Sleep time between each run (in seconds)

// Serial debug trace wrapper - enable/disable with DEBUG 0/1
#define DEBUG 0
#if DEBUG
#define D_SerialBegin(...) Serial.begin(__VA_ARGS__);
#define D_print(...)    Serial.print(__VA_ARGS__)
#define D_printf(...)   Serial.printf(__VA_ARGS__)
#define D_write(...)    Serial.write(__VA_ARGS__)
#define D_println(...)  Serial.println(__VA_ARGS__)
#else
#define D_SerialBegin(...)
#define D_print(...)
#define D_printf(...)
#define D_write(...)
#define D_println(...)
#endif

// RTC data (persists across deep sleep)
RTC_DATA_ATTR uint32_t timeOnBack = 0; // in seconds, how long has the wearer been on their back?
RTC_DATA_ATTR bool isFirstRun = true; // is this the first run of the device after power on, or are we waking from deep sleep?

// Globals for this run
char buf[128] = {0};
TTGOClass *watch = nullptr;

void setup()
{
    D_SerialBegin(115200);
    D_println("setup()");
    D_printf("isFirstRun: %d\n", isFirstRun);
    D_printf("timeOnBack: %u\n", timeOnBack);

    // Initialize the hardware, the BMA423 sensor has been initialized internally
    watch = TTGOClass::getWatch();
    watch->begin();

    // Initialize the motor
    watch->motor_begin();

    // Configure and enable the BMA423 accelerometer
    Acfg cfg;
    cfg.odr = BMA4_OUTPUT_DATA_RATE_100HZ;  // BMA4_OUTPUT_DATA_RATE_0_78HZ?
    cfg.range = BMA4_ACCEL_RANGE_2G;        // G-Range
    cfg.bandwidth = BMA4_ACCEL_NORMAL_AVG4; // Bandwidth parameter, determines filter configuration
    cfg.perf_mode = BMA4_CONTINUOUS_MODE;   // Filter performance mode
    watch->bma->accelConfig(cfg);
    watch->bma->enableAccel();

    // If woken from deep sleep via PEK key, treat as if a full reboot to show user battery info
    const esp_sleep_wakeup_cause_t wakeup_cause = esp_sleep_get_wakeup_cause();
    D_printf("wakeup_cause: %d\n", wakeup_cause);
    if (wakeup_cause == ESP_SLEEP_WAKEUP_EXT1)
    {
      isFirstRun = true;
    }

    if (isFirstRun)
    {
      timeOnBack = 0;

      // Turn on the battery ADC, allow enough time for things to stabilise, read battery percentage, then turn off ADC again
      watch->power->adc1Enable(AXP202_VBUS_VOL_ADC1 | AXP202_VBUS_CUR_ADC1 | AXP202_BATT_CUR_ADC1 | AXP202_BATT_VOL_ADC1, true);
      delay(250);
      const int battPercentage = watch->power->getBattPercentage(); // 0-100%
      const float battVoltage = watch->power->getBattVoltage() / 1000.0f; // Volts
      watch->power->adc1Enable(AXP202_VBUS_VOL_ADC1 | AXP202_VBUS_CUR_ADC1 | AXP202_BATT_CUR_ADC1 | AXP202_BATT_VOL_ADC1, false);
      D_printf("battery: %d%% (%0.2fV)\n", battPercentage, battVoltage);

      // Vibrate motor
      watch->motor->onec(200);

      // Display splash message
      watch->openBL();
      watch->tft->setRotation(WATCH_SCREEN_TOP_EDGE);
      watch->tft->fillScreen(TFT_BLACK);
      watch->tft->setTextColor(TFT_WHITE);
      sprintf(buf, "Battery: %d%% (%0.2fV)", battPercentage, battVoltage);
      watch->tft->drawCentreString(buf, 120, 70, 2);
      watch->tft->drawCentreString("Back Sleep Alert", 120, 100, 4);
      watch->tft->drawCentreString("Wear on front, display facing body", 120, 130, 2);
      delay(10000);
      watch->closeBL();
      isFirstRun = false;
    }
    else
    {
      // Delay to allow accelerometer to stabilise
      delay(250);
    }

    // Turn off screen
    watch->displaySleep();

    // Obtain the accelerometer direction
    const uint8_t direction = watch->bma->direction();
    D_printf("direction: %u\n", direction);

    const bool isWearerOnBack = direction == DIRECTION_DISP_DOWN;
    D_printf("isWearerOnBack: %d\n", isWearerOnBack);
    if (isWearerOnBack)
    {
      // Issue a warning to the wearer if they've been on their back a while
      if (timeOnBack >= 30 * MIN_TO_S)
      {
          // Stop warning after a while as wearer probably took off device
      }
      else if (timeOnBack >= 10 * MIN_TO_S)
      {
          D_println("Strongest warning");
          watch->motor->onec(2000);
          delay(3000);
          watch->motor->onec(2000);
          delay(3000);
          watch->motor->onec(2000);
          delay(2100);
      }
      else if (timeOnBack >= 4 * MIN_TO_S)
      {
          D_println("Strong warning");
          watch->motor->onec(2000);
          delay(2100);
      }
      else
      {
          // No warning for first little while
      }

      timeOnBack += TIME_TO_SLEEP;
    }
    else
    {
      timeOnBack = 0;
    }

    // Prepare for deep sleep wake from PEK key 
    pinMode(AXP202_INT, INPUT_PULLUP);
    watch->power->enableIRQ(AXP202_PEK_SHORTPRESS_IRQ, true);
    watch->power->clearIRQ();    

    // Enter deep sleep, disabling power for everything except RTC memory
    watch->power->setPowerOutPut(AXP202_LDO3, false);
    watch->power->setPowerOutPut(AXP202_LDO4, false);
    watch->power->setPowerOutPut(AXP202_LDO2, false);
    watch->power->setPowerOutPut(AXP202_EXTEN, false);
    watch->power->setPowerOutPut(AXP202_DCDC2, false);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH,   ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_ON);
    esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL,         ESP_PD_OPTION_OFF);
    esp_sleep_enable_timer_wakeup(S_TO_uS * TIME_TO_SLEEP);
    esp_sleep_enable_ext1_wakeup(GPIO_SEL_35, ESP_EXT1_WAKEUP_ALL_LOW); // wake from PEK key
    esp_deep_sleep_start();
}

void loop()
{
  // never run, but let's print just to be sure
  D_println("loop()");
}
