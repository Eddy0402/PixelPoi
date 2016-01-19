#include <cstring>
#include "misc.hpp"
#include "Mode.hpp"
#include "Count.hpp"
#include "Image.hpp"

const int map[] = {
    20, 16, 21, 17, 18, 22, 19, 24, 28, 25, 29, 30, 26, 31, 27, 23,7, 11, 15, 10, 14, 13, 9, 12, 8, 3, 6, 2, 1, 5, 0, 4,};

void getImgRangeSpecial(uint8_t mode, uint8_t id, uint16_t time, uint16_t *buf, uint8_t start, uint8_t end) {
    time = time % 32;

    for(int i = 0;i < 32; ++i){
        int r = map[i];
        if(r >= start && r < end){
            int c = 0;
            buf[(r - start) * 3 + c] = 65535;//((i + 32 - time) % 32) > 16 ? 65535 : 0;
            c = 1;
            buf[(r - start) * 3 + c] = 65535;//((i + 48 - time) % 32) > 16 ? 65535 : 0;
            c = 2;
            buf[(r - start) * 3 + c] = 65535;//((i + 40 - time) % 32) > 16 ? 65535 : 0;
        }
    }
}
void getImgRange(uint8_t mode, uint8_t id, uint16_t time, uint16_t *buf, uint8_t start, uint8_t end) {
    if(mode == 3){
        getImgRangeSpecial(mode,id,time,buf,start,end);
        //return;
    }
    const Mode &m = *reinterpret_cast<const Mode *>(imagedata);
    uint8_t indexAddress;
    switch (mode) {
        case 0:
            indexAddress = m.imageIndexAddress;
            break;
        case 1:
            indexAddress = m.lineIndexAddress;
            break;
        case 2:
            indexAddress = m.brakeIndexAddress;
            break;
    }

    const Count &count = *reinterpret_cast<const Count *>(imagedata + indexAddress);
    const uint16_t imageAddress = *reinterpret_cast<const uint16_t*>(imagedata + indexAddress + sizeof(Count) + id * sizeof(uint16_t));

    const Image &img = *reinterpret_cast<const Image *>(imagedata + imageAddress);
    time = time % img.width;
    uint16_t startAddress = imageAddress + sizeof(Image) + time * img.height * 3;
    const uint8_t *raw = imagedata + startAddress;

    for(int i = 0; i < 32;++i){
        int r = map[i];
        if(r >= start && r < end){
            for(int k = 0;k < 3;++k){
                buf[(r - start) * 3 + k] = ((uint16_t)raw[r * 3 + k]) << 8;
            }
        }
    }
}
