// служебные функции

double threeway_max(double a, double b, double c) {
    return max(a, max(b, c));
}

double threeway_min(double a, double b, double c) {
    return min(a, min(b, c));
}

// залить все
void fillAll(CRGB color) {
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = color;
    }
}

double rgbToHue(byte r, byte g, byte b) {
    double rd = (double) r / 255;
    double gd = (double) g / 255;
    double bd = (double) b / 255;
    double max = threeway_max(rd, gd, bd), min = threeway_min(rd, gd, bd);
    double h, s, v = max;

    double d = max - min;
    s = max == 0 ? 0 : d / max;

    if (max == min) {
        h = 0; // achromatic
    } else {
        if (max == rd) {
            h = (gd - bd) / d + (gd < bd ? 6 : 0);
        } else if (max == gd) {
            h = (bd - rd) / d + 2;
        } else if (max == bd) {
            h = (rd - gd) / d + 4;
        }
        h /= 6;
    }

    return h;
}

// функция отрисовки точки по координатам X Y
void drawPixelXY(int8_t x, int8_t y, CRGB color) {
    if (x < 0 || x > WIDTH - 1 || y < 0 || y > HEIGHT - 1) return;
    int thisPixel = getPixelNumber(x, y) * SEGMENTS;
    for (byte i = 0; i < SEGMENTS; i++) {
        leds[thisPixel + i] = color;
    }
}

// функция получения цвета пикселя по его номеру
uint32_t getPixColor(int thisSegm) {
    int thisPixel = thisSegm * SEGMENTS;
    if (thisPixel < 0 || thisPixel > NUM_LEDS - 1) return 0;
    return (((uint32_t) leds[thisPixel].r << 16) | ((long) leds[thisPixel].g << 8) | (long) leds[thisPixel].b);
}

// функция получения цвета пикселя в матрице по его координатам
uint32_t getPixColorXY(int8_t x, int8_t y) {
    return getPixColor(getPixelNumber(x, y));
}

// **************** НАСТРОЙКА МАТРИЦЫ ****************
#if (CONNECTION_ANGLE == 0 && STRIP_DIRECTION == 0)
#define _WIDTH WIDTH
#define THIS_X x
#define THIS_Y y

#elif (CONNECTION_ANGLE == 0 && STRIP_DIRECTION == 1)
#define _WIDTH HEIGHT
#define THIS_X y
#define THIS_Y x

#elif (CONNECTION_ANGLE == 1 && STRIP_DIRECTION == 0)
#define _WIDTH WIDTH
#define THIS_X x
#define THIS_Y (HEIGHT - y - 1)

#elif (CONNECTION_ANGLE == 1 && STRIP_DIRECTION == 3)
#define _WIDTH HEIGHT
#define THIS_X (HEIGHT - y - 1)
#define THIS_Y x

#elif (CONNECTION_ANGLE == 2 && STRIP_DIRECTION == 2)
#define _WIDTH WIDTH
#define THIS_X (WIDTH - x - 1)
#define THIS_Y (HEIGHT - y - 1)

#elif (CONNECTION_ANGLE == 2 && STRIP_DIRECTION == 3)
#define _WIDTH HEIGHT
#define THIS_X (HEIGHT - y - 1)
#define THIS_Y (WIDTH - x - 1)

#elif (CONNECTION_ANGLE == 3 && STRIP_DIRECTION == 2)
#define _WIDTH WIDTH
#define THIS_X (WIDTH - x - 1)
#define THIS_Y y

#elif (CONNECTION_ANGLE == 3 && STRIP_DIRECTION == 1)
#define _WIDTH HEIGHT
#define THIS_X y
#define THIS_Y (WIDTH - x - 1)

#else
#define _WIDTH WIDTH
#define THIS_X x
#define THIS_Y y
#pragma message "Wrong matrix parameters! Set to default"

#endif

// получить номер пикселя в ленте по координатам
uint16_t getPixelNumber(int8_t x, int8_t y) {
    if ((THIS_Y % 2 == 0) || MATRIX_TYPE) {               // если чётная строка
        return (THIS_Y * _WIDTH + THIS_X);
    } else {                                              // если нечётная строка
        return (THIS_Y * _WIDTH + _WIDTH - THIS_X - 1);
    }
}
