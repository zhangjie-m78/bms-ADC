#include "ds18b20.h"
#include "main.h"
#include "stm32f4xx_hal.h"

#define DQ_PORT GPIOA
#define DQ_PIN  GPIO_PIN_1

static void DWT_Init_local(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static void delay_us_local(uint32_t us) {
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (SystemCoreClock / 1000000);
    while ((DWT->CYCCNT - start) < ticks);
}

static void DQ_Output(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DQ_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DQ_PORT, &GPIO_InitStruct);
}

static void DQ_Input(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DQ_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DQ_PORT, &GPIO_InitStruct);
}

static uint8_t DS18B20_Reset(void) {
    uint8_t presence;
    DQ_Output();
    HAL_GPIO_WritePin(DQ_PORT, DQ_PIN, GPIO_PIN_RESET);
    delay_us_local(480);
    HAL_GPIO_WritePin(DQ_PORT, DQ_PIN, GPIO_PIN_SET);
    DQ_Input();
    delay_us_local(70);
    presence = HAL_GPIO_ReadPin(DQ_PORT, DQ_PIN);
    delay_us_local(410);
    return presence;
}

static void DS18B20_WriteBit(uint8_t bit) {
    DQ_Output();
    HAL_GPIO_WritePin(DQ_PORT, DQ_PIN, GPIO_PIN_RESET);
    if (bit) {
        delay_us_local(2);
        HAL_GPIO_WritePin(DQ_PORT, DQ_PIN, GPIO_PIN_SET);
        delay_us_local(60);
    } else {
        delay_us_local(60);
        HAL_GPIO_WritePin(DQ_PORT, DQ_PIN, GPIO_PIN_SET);
        delay_us_local(2);
    }
}

static uint8_t DS18B20_ReadBit(void) {
    uint8_t bit = 0;
    DQ_Output();
    HAL_GPIO_WritePin(DQ_PORT, DQ_PIN, GPIO_PIN_RESET);
    delay_us_local(2);
    HAL_GPIO_WritePin(DQ_PORT, DQ_PIN, GPIO_PIN_SET);
    DQ_Input();
    delay_us_local(12);
    if (HAL_GPIO_ReadPin(DQ_PORT, DQ_PIN) == GPIO_PIN_SET) {
        bit = 1;
    }
    delay_us_local(50);
    return bit;
}

static void DS18B20_WriteByte(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        DS18B20_WriteBit(data & 0x01);
        data >>= 1;
    }
}

static uint8_t DS18B20_ReadByte(void) {
    uint8_t data = 0;
    for (int i = 0; i < 8; i++) {
        data |= (DS18B20_ReadBit() << i);
    }
    return data;
}

void DS18B20_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    DWT_Init_local();
    DQ_Output();
    HAL_GPIO_WritePin(DQ_PORT, DQ_PIN, GPIO_PIN_SET);
}

float DS18B20_ReadTemp(void) {
    uint8_t tl, th;
    int16_t raw;

    DS18B20_Reset();
    DS18B20_WriteByte(0xCC);
    DS18B20_WriteByte(0x44);
    HAL_Delay(750);

    DS18B20_Reset();
    DS18B20_WriteByte(0xCC);
    DS18B20_WriteByte(0xBE);

    tl = DS18B20_ReadByte();
    th = DS18B20_ReadByte();

        raw = (th << 8) | tl;

    if (raw & 0x8000) {
        raw = (~raw) + 1;
        return -(float)raw / 16.0f;
    }
    return (float)raw / 16.0f;
}