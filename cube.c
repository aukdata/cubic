#include <stdio.h>
#include <pico/stdlib.h>
#include <class/cdc/cdc_device.h>
#include "fatfs/ff.h"

#define COM_RCLK 4
#define COM_SRCLK 5
#define SS1_SER 6
#define SS2_SER 7
#define SS3_SER 8
#define SS4_SER 9

int main()
{
    stdio_init_all();

    puts("INITIALIZING...");

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_init(COM_RCLK);
    gpio_init(COM_SRCLK);
    gpio_init(SS1_SER);
    gpio_init(SS2_SER);
    gpio_init(SS3_SER);
    gpio_init(SS4_SER);

    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_set_dir(COM_RCLK, GPIO_OUT);
    gpio_set_dir(COM_SRCLK, GPIO_OUT);
    gpio_set_dir(SS1_SER, GPIO_OUT);
    gpio_set_dir(SS2_SER, GPIO_OUT);
    gpio_set_dir(SS3_SER, GPIO_OUT);
    gpio_set_dir(SS4_SER, GPIO_OUT);

    gpio_put(PICO_DEFAULT_LED_PIN, true);
    gpio_put(COM_RCLK, 0);
    gpio_put(COM_SRCLK, 0);
    gpio_put(SS1_SER, 0);
    gpio_put(SS2_SER, 0);
    gpio_put(SS3_SER, 0);
    gpio_put(SS4_SER, 0);

    uint32_t cube[5] = {
        0b1010101010101010101010101,
        0b0101010101010101010101010,
        0b1010101010101010101010101,
        0b0101010101010101010101010,
        0b1010101010101010101010101,
    };

    uint8_t cube_g[5][13] = {
        { 0 | 1 << 4, 2 | 3 << 4, 4 | 5 << 4, 6 | 7 << 4, 8 | 9 << 4, 10 | 11 << 4, 12 | 13 << 4, 14 | 15 << 4 },
        { 0 | 1 << 4, 2 | 3 << 4, 4 | 5 << 4, 6 | 7 << 4, 8 | 9 << 4, 10 | 11 << 4, 12 | 13 << 4, 14 | 15 << 4 },
        { 0 | 1 << 4, 2 | 3 << 4, 4 | 5 << 4, 6 | 7 << 4, 8 | 9 << 4, 10 | 11 << 4, 12 | 13 << 4, 14 | 15 << 4 },
        { 0 | 1 << 4, 2 | 3 << 4, 4 | 5 << 4, 6 | 7 << 4, 8 | 9 << 4, 10 | 11 << 4, 12 | 13 << 4, 14 | 15 << 4 },
        { 0 | 1 << 4, 2 | 3 << 4, 4 | 5 << 4, 6 | 7 << 4, 8 | 9 << 4, 10 | 11 << 4, 12 | 13 << 4, 14 | 15 << 4 },
    };

    while (true)
    {
        for (int c = 0; c <= 200; c++)
        {
            for (int j = 0; j < 16; j++)
            {
                for (int layer = 0; layer < 5; layer++)
                {
                    gpio_put(COM_RCLK, 0);
                    sleep_us(1);

                    for (int i = 7; i >= 0; i--)
                    {
                        gpio_put(COM_SRCLK, 0);
                        sleep_us(1);
                        
                        gpio_put(SS1_SER, !(cube[layer] >> ( 0 + i) & 1));
                        gpio_put(SS2_SER, !(cube[layer] >> ( 8 + i) & 1));
                        gpio_put(SS3_SER, !(cube[layer] >> (16 + i) & 1));

                        if (i == 0)
                        {
                            gpio_put(SS4_SER, !(cube[layer] >> 24));
                            puts("i = 0");
                        }
                        else
                        {
                            gpio_put(SS4_SER, (i - 1) == layer);
                            printf("i = %d, layer = %d, cond = %d\n", i, layer, (i - 1) == layer);
                        }

                        sleep_us(1);

                        gpio_put(COM_SRCLK, 1);
                        sleep_us(1);
                    }

                    gpio_put(COM_RCLK, 1);
                    sleep_us(1000);
                }
            }
        }

        for (int i = 0; i < 5; i++)
        {
            cube[i] = ~cube[i];
        }
    }

    return 0;
}
