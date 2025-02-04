#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include <ArduinoJson.h>
#include "SPIFFS_function.h"

// تعریف پین‌های خروجی
const int OUTPUT_PINS[] = {26, 27, 28}; // شماره پین‌ها را متناسب با نیاز تغییر دهید
const int NUM_OUTPUTS = 3;

// تنظیمات RTC
RTC_DS3231 rtc;

// تنظیمات SPIFFS
const char* CONFIG_FILE = "/timer_config.json";
const size_t JSON_SIZE = 4096;

// ساختار نگهداری وضعیت خروجی‌ها
struct OutputState {
    bool isOn = false;
    bool isHoliday = false;
    bool hasSecondTimer = false;
};

OutputState outputStates[NUM_OUTPUTS];

// تبدیل ساعت و دقیقه به timestamp
long timeToTimestamp(int hours, int minutes) {
    return hours * 3600 + minutes * 60;
}

// تبدیل رشته زمان به timestamp
long convertTimeStringToTimestamp(String timeStr) {
    int hours = timeStr.substring(0, 2).toInt();
    int minutes = timeStr.substring(3, 5).toInt();
    return timeToTimestamp(hours, minutes);
}

// خواندن تنظیمات از SPIFFS
bool loadConfiguration() {
    if (!SPIFFS.exists(CONFIG_FILE)) {
        Serial.println("Config file not found");
        return false;
    }

    File file = SPIFFS.open(CONFIG_FILE, "r");
    if (!file) {
        Serial.println("Failed to open config file");
        return false;
    }

    StaticJsonDocument<JSON_SIZE> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.println("Failed to parse config file");
        return false;
    }

    // فقط تنظیمات روز جاری را نگه می‌داریم
    JsonArray dayArray = doc.as<JsonArray>();
    DateTime now = rtc.now();
    int currentDay = now.dayOfTheWeek(); // 0 = یکشنبه، 6 = شنبه
    
    // تبدیل به فرمت مورد نظر ما (0 = شنبه)
    currentDay = (currentDay + 6) % 7;
    
    if (currentDay < dayArray.size()) {
        JsonObject dayData = dayArray[currentDay];
        
        for (int i = 0; i < NUM_OUTPUTS; i++) {
            String keyPrefix = String(i + 1);
            outputStates[i].isHoliday = dayData[keyPrefix + "isholiday"].as<String>() == "1";
            outputStates[i].hasSecondTimer = dayData[keyPrefix + "is_timer2"].as<String>() == "1";
        }
    }

    return true;
}

// بررسی و بروزرسانی وضعیت خروجی‌ها
void updateOutputs() {
    DateTime now = rtc.now();
    int currentDay = (now.dayOfTheWeek() + 6) % 7;
    long currentTime = timeToTimestamp(now.hour(), now.minute());
    
    StaticJsonDocument<JSON_SIZE> doc;
    File file = SPIFFS.open(CONFIG_FILE, "r");
    if (!file) return;
    
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    if (error) return;

    JsonArray dayArray = doc.as<JsonArray>();
    if (currentDay >= dayArray.size()) return;
    
    JsonObject dayData = dayArray[currentDay];

    for (int i = 0; i < NUM_OUTPUTS; i++) {
        if (outputStates[i].isHoliday) {
            digitalWrite(OUTPUT_PINS[i], LOW);
            continue;
        }

        String keyPrefix = String(i + 1);
        long timeOn = convertTimeStringToTimestamp(dayData[keyPrefix + "on"].as<String>());
        long timeOff = convertTimeStringToTimestamp(dayData[keyPrefix + "off"].as<String>());
        
        bool shouldBeOn = false;

        // بررسی تایمر اول
        if (timeOn < timeOff) {
            shouldBeOn = (currentTime >= timeOn && currentTime < timeOff);
        } else {
            shouldBeOn = (currentTime >= timeOn || currentTime < timeOff);
        }

        // بررسی تایمر دوم
        if (outputStates[i].hasSecondTimer && !shouldBeOn) {
            long timeOn2 = convertTimeStringToTimestamp(dayData[keyPrefix + "on2"].as<String>());
            long timeOff2 = convertTimeStringToTimestamp(dayData[keyPrefix + "off2"].as<String>());
            
            if (timeOn2 < timeOff2) {
                shouldBeOn = (currentTime >= timeOn2 && currentTime < timeOff2);
            } else {
                shouldBeOn = (currentTime >= timeOn2 || currentTime < timeOff2);
            }
        }

        digitalWrite(OUTPUT_PINS[i], shouldBeOn ? HIGH : LOW);
        outputStates[i].isOn = shouldBeOn;
    }
}

void setup() {
   
    Serial.begin(115200);
    #define FORMAT_SPIFFS_IF_FAILED false
    if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
    {
      Serial.println("SPIFFS Mount Failed");
       return;
    }
    FSlistDir(String dirname, uint8_t levels);
    Serial.println(FSlistDir("/", 0));
    // راه‌اندازی RTC
    if (!rtc.begin()) {
        Serial.println("Couldn't find RTC");
        while (1);
    }

    if (rtc.lostPower()) {
        Serial.println("RTC lost power, lets set the time!");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    // راه‌اندازی SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
        return;
    }

    // تنظیم پین‌های خروجی
    for (int i = 0; i < NUM_OUTPUTS; i++) {
        pinMode(OUTPUT_PINS[i], OUTPUT);
        digitalWrite(OUTPUT_PINS[i], LOW);
    }

    // خواندن تنظیمات اولیه
    loadConfiguration();
}

void loop() {
    static unsigned long lastUpdate = 0;
    unsigned long currentMillis = millis();

    // بروزرسانی هر 1 ثانیه
    if (currentMillis - lastUpdate >= 1000) {
        updateOutputs();
        lastUpdate = currentMillis;
        
        // نمایش اطلاعات برای دیباگ
        DateTime now = rtc.now();
        Serial.printf("Time: %02d:%02d:%02d, Day: %d\n", 
            now.hour(), now.minute(), now.second(), 
            (now.dayOfTheWeek() + 6) % 7);
        
        for (int i = 0; i < NUM_OUTPUTS; i++) {
            Serial.printf("Output %d: %s\n", i + 1, 
                outputStates[i].isOn ? "ON" : "OFF");
        }
    }
}
