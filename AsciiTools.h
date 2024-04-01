#ifndef _ASCII_TOOLS_H
#define _ASCII_TOOLS_H

#include <stdint.h>


uint8_t AsciiHexToByte(uint8_t *ascii) {
    uint8_t ret;
    if(ascii[0] >= 'A')
        ret = (ascii[0] - 'A' + 10) << 4;
    else
        ret = (ascii[0] - '0') << 4;
    
    if(ascii[1] >= 'A')
        ret += (ascii[1] - 'A' + 10);
    else
        ret += (ascii[1] - '0');

    return ret;
}

void AsciiHexToBytes(uint8_t *ascii, uint8_t asciiLen, uint8_t *out_bytes) {
    for(uint8_t i = 0; i < asciiLen-1; i+=2)
        out_bytes[i>>1] = AsciiHexToByte(ascii+i);
}

#endif