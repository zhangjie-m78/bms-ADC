#ifndef __DS18B20_H
#define __DS18B20_H

#include "stm32f4xx_hal.h"

void DS18B20_Init(void);
float DS18B20_ReadTemp(void);

#endif