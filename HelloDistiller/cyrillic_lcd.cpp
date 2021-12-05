// Encoding translation for 2004 LCD with hardcoded Cyrillic alphabit
// Based on the LiquidCrystalRus library
// https://github.com/mk90/LiquidCrystalRus/blob/master/LiquidCrystalRus.cpp

#include "configuration.h"
#include "declarations.h"

#if USE_CYRILLIC_DISPLAY

// it is a russian alphabet translation
// except 0401 --> 0xa2 = ╗, 0451 --> 0xb5
const int8_t utf_recode[] PROGMEM = {
    0x70, 0x63, 0xbf, 0x79, 0xe4, 0x78, 0xe5, 0xc0, 0xc1, 0xe6, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
    0x41, 0xa0, 0x42, 0xa1, 0xe0, 0x45, 0xa3, 0xa4, 0xa5, 0xa6, 0x4b, 0xa7, 0x4d, 0x48, 0x4f,
    0xa8, 0x50, 0x43, 0x54, 0xa9, 0xaa, 0x58, 0xe1, 0xab, 0xac, 0xe2, 0xad, 0xae, 0x62, 0xaf, 0xb0, 0xb1,
    0x61, 0xb2, 0xb3, 0xb4, 0xe3, 0x65, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0x6f, 0xbe
};

char* utf8rus(char* str)
{
    const uint8_t k = strlen(str);
    uint8_t n;
    uint8_t* buf = (uint8_t*)malloc(k);
    memset(buf, '\0', k);

    uint8_t i, j;

    for (i = 0, j = 0; i < k; i++, j++) {
        n = str[i];
        if (n >= 0xd0 && n < 0xd2) {
            // Cyrillic letters processing
            const uint8_t value = str[i + 1];

            if ((n == 0xD0) && (value == 0x81)) {
                n = 0xa2; // Ё
            } else if ((n == 0xD1) && (value == 0x91)) {
                n = 0xb5; // ё
            } else {
                if (value > 0x7F && value < 0xC0)
                    n = pgm_read_byte(utf_recode + value - 0x80);
                else
                    n = 0x2a;
            }
            i++;
        }
        buf[j] = n;
    }
    memcpy(str, buf, k);
    free(buf);
    return (char*)str;
}

#endif //  USE_CYRILLIC_DISPLAY
