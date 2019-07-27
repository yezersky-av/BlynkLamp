/*
  Скетч к проекту "Многофункциональный RGB светильник"
  Страница проекта (схемы, описания): https://alexgyver.ru/GyverLamp/
  Исходники на GitHub: https://github.com/AlexGyver/GyverLamp/
  Нравится, как написан код? Поддержи автора! https://alexgyver.ru/support_alex/
  Автор: AlexGyver, AlexGyver Technologies, 2019
  https://AlexGyver.ru/
*/

/*
  Версия 1.4:
  - Исправлен баг при смене режимов
  - Исправлены тормоза в режиме точки доступа
*/

// Ссылка для менеджера плат:
// http://arduino.esp8266.com/stable/package_esp8266com_index.json

// ============= НАСТРОЙКИ =============

// ---------- МАТРИЦА ---------
#define BRIGHTNESS 40         // стандартная маскимальная яркость (0-255)
#define CURRENT_LIMIT 2000    // лимит по току в миллиамперах, автоматически управляет яркостью (пожалей свой блок питания!) 0 - выключить лимит

#define WIDTH 16              // ширина матрицы
#define HEIGHT 16             // высота матрицы

#define COLOR_ORDER GRB       // порядок цветов на ленте. Если цвет отображается некорректно - меняйте. Начать можно с RGB

#define MATRIX_TYPE 0         // тип матрицы: 0 - зигзаг, 1 - параллельная
#define CONNECTION_ANGLE 1    // угол подключения: 0 - левый нижний, 1 - левый верхний, 2 - правый верхний, 3 - правый нижний
#define STRIP_DIRECTION 0     // направление ленты из угла: 0 - вправо, 1 - вверх, 2 - влево, 3 - вниз
// при неправильной настройке матрицы вы получите предупреждение "Wrong matrix parameters! Set to default"
// шпаргалка по настройке матрицы здесь! https://alexgyver.ru/matrix_guide/

// ============= ДЛЯ РАЗРАБОТЧИКОВ =============
#define LED_PIN 0             // пин ленты
#define BTN_PIN 4
#define MODE_AMOUNT 18

#define NUM_LEDS WIDTH * HEIGHT
#define SEGMENTS 1            // диодов в одном "пикселе" (для создания матрицы из кусков ленты)
// ---------------- БИБЛИОТЕКИ -----------------
#define FASTLED_INTERRUPT_RETRY_COUNT 0
#define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_ESP8266_RAW_PIN_ORDER
#define NTP_INTERVAL 60 * 1000    // обновление (1 минута)

#define CURRENT_MODE_ADDRESS 10

#include "settings.h"
#include <FastLED.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <GyverButton.h>

// ------------------- ТИПЫ --------------------
CRGB leds[NUM_LEDS];
//GButton touch(BTN_PIN, LOW_PULL, NORM_OPEN);

// ----------------- ПЕРЕМЕННЫЕ ------------------
static const byte maxDim = max(WIDTH, HEIGHT);

struct Mode {
    byte brightness = 50;
    byte speed = 30;
    byte scale = 40;
    struct {
        byte r = 255;
        byte g = 255;
        byte b = 255;
    } rgb;
};

Mode modes[MODE_AMOUNT];

struct {
    boolean state = false;
    int time = 0;
} alarm[7];

byte dawnOffsets[] = {5, 10, 15, 20, 25, 30, 40, 50, 60};
byte dawnMode;
boolean dawnFlag = false;
long thisTime;
boolean manualOff = false;

int8_t currentMode = 0;
boolean loadingFlag = true;
boolean ONflag = true;
uint32_t eepromTimer;
boolean settChanged = false;
// Конфетти, Огонь, Радуга верт., Радуга гориз., Смена цвета,
// Безумие 3D, Облака 3D, Лава 3D, Плазма 3D, Радуга 3D,
// Павлин 3D, Зебра 3D, Лес 3D, Океан 3D,

unsigned char matrixValue[8][16];

int getModeAddress(int modeIndex) {
    return (sizeof(Mode) * modeIndex) + 100;
}

void saveModeToEEPROM(int modeIndex) {
    const int address = getModeAddress(modeIndex);

    EEPROM.write(address, modes[modeIndex].brightness);
    EEPROM.write(address + 1, modes[modeIndex].speed);
    EEPROM.write(address + 2, modes[modeIndex].scale);
    EEPROM.write(address + 3, modes[modeIndex].rgb.r);
    EEPROM.write(address + 4, modes[modeIndex].rgb.g);
    EEPROM.write(address + 5, modes[modeIndex].rgb.b);
    EEPROM.commit();
}

void retrieveModeFromEEPROM(int modeIndex) {
    const int address = getModeAddress(modeIndex);

    modes[modeIndex].brightness = EEPROM.read(address);
    modes[modeIndex].speed = EEPROM.read(address + 1);
    modes[modeIndex].scale = EEPROM.read(address + 2);
    modes[modeIndex].rgb.r = EEPROM.read(address + 3);
    modes[modeIndex].rgb.g = EEPROM.read(address + 4);
    modes[modeIndex].rgb.b = EEPROM.read(address + 5);
}

void setup() {
    Serial.begin(115200);
    Serial.println();

    Blynk.begin(auth, ssid, pass);

    EEPROM.begin(512);
    delay(50);

    if (EEPROM.read(0) != 20) {
        EEPROM.write(0, 20);

        EEPROM.write(CURRENT_MODE_ADDRESS, 0);
        EEPROM.commit();

        for (byte i = 0; i < MODE_AMOUNT; i++) {
            saveModeToEEPROM(i);
        }
    }

    currentMode = (int8_t) EEPROM.read(CURRENT_MODE_ADDRESS);

    for (byte i = 0; i < MODE_AMOUNT; i++) {
        retrieveModeFromEEPROM(i);
    }

    // ЛЕНТА
    FastLED.addLeds<WS2812B, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS)/*.setCorrection( TypicalLEDStrip )*/;
    FastLED.setBrightness(modes[currentMode].brightness);
    if (CURRENT_LIMIT > 0) FastLED.setMaxPowerInVoltsAndMilliamps(5, CURRENT_LIMIT);
    FastLED.clear();
    FastLED.show();

//    touch.setStepTimeout(100);
//    touch.setClickTimeout(500);

    randomSeed(micros());

    Blynk.virtualWrite(V0, currentMode + 1);
    Blynk.virtualWrite(V1, modes[currentMode].brightness);
    Blynk.virtualWrite(V2, modes[currentMode].speed);
    Blynk.virtualWrite(V3, modes[currentMode].scale);
    Blynk.virtualWrite(V4, ONflag);
}

BLYNK_WRITE(V0)
        {
                currentMode = param.asInt() - 1; // assigning incoming value from pin V1 to a variable
        saveModeToEEPROM(currentMode);

        EEPROM.write(CURRENT_MODE_ADDRESS, currentMode);
        EEPROM.commit();
        }

BLYNK_WRITE(V1)
        {
                int brightness = param.asInt(); // assigning incoming value from pin V1 to a variable
        if (brightness < 0) brightness = 0;
        if (brightness > 255) brightness = 255;
        modes[currentMode].brightness = brightness;
        FastLED.setBrightness(modes[currentMode].brightness);
        saveModeToEEPROM(currentMode);

        loadingFlag = true;
        settChanged = true;
        }

BLYNK_WRITE(V2)
        {
                int speed = param.asInt(); // assigning incoming value from pin V1 to a variable
        if (speed < 0) speed = 0;
        if (speed > 255) speed = 255;
        modes[currentMode].speed = speed;
        saveModeToEEPROM(currentMode);

        loadingFlag = true;
        settChanged = true;
        }

BLYNK_WRITE(V3)
        {
                int scale = param.asInt(); // assigning incoming value from pin V1 to a variable
        if (scale < 0) scale = 0;
        if (scale > 255) scale = 255;
        modes[currentMode].scale = scale;
        saveModeToEEPROM(currentMode);

        loadingFlag = true;
        settChanged = true;
        }

BLYNK_WRITE(V4)
        {
                int on = param.asInt(); // assigning incoming value from pin V1 to a variable

        ONflag = (on == 1? true : false);
        FastLED.clear();
        FastLED.show();

        loadingFlag = true;
        settChanged = true;
        }

BLYNK_WRITE(V5)
        {
                modes[currentMode].rgb.r = param[0].asInt();
        modes[currentMode].rgb.g = param[1].asInt();
        modes[currentMode].rgb.b = param[2].asInt();
        saveModeToEEPROM(currentMode);

        loadingFlag = true;
        settChanged = true;
        }

void loop() {
    Blynk.run();

    effectsTick();
//    buttonTick();
}
