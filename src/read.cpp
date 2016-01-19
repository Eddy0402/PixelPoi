#include <cstring>
#include "misc.hpp"
#include "Mode.hpp"
#include "Count.hpp"
#include "Image.hpp"

uint16_t getImg(uint8_t mode, uint8_t id, uint16_t time, uint8_t *buf) {
	Mode m;
	memcpy(&m, imagedata, sizeof(Mode));
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

	Count count;
	memcpy(&count, imagedata + indexAddress, sizeof(Count));
	uint16_t imageAddress;
	memcpy(&imageAddress, imagedata + indexAddress + sizeof(Count) + id*sizeof(uint16_t), sizeof(uint16_t));

	Image img;
	memcpy(&img, imagedata + imageAddress, sizeof(Image));
	uint16_t startAddress = imageAddress + sizeof(Image);

	for (int i = 0; i < 32; ++i)
		memcpy(buf + i * 3, imagedata + startAddress + i * img.width * 3, sizeof(uint8_t) * 3);

	return time % img.width;
}
