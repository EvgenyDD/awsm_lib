#include <stdint.h>

uint8_t crc8(uint8_t *pdata, int count)
{
	uint8_t crc = 0x00;
	int i = 0;
	while(i < count)
	{
		crc ^= *pdata;
		for(int bit = 0; bit < 8; bit++)
		{
			if((crc & 0x80) != 0x00)
				crc = (crc << 1) ^ 0x07;
			else
				crc <<= 1;
		}
		pdata++;
		i++;
	}
	return crc;
}