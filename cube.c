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

#define SD_CS 16
#define SD_DI 17
#define SD_SCLK 18
#define SD_DO 19
#define SD_DETECT 20

typedef struct
{
    uint8_t frame[5][13];
    int32_t duration;
} cube_t;

void initialize_gpio()
{
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_init(COM_RCLK);
    gpio_init(COM_SRCLK);
    gpio_init(SS1_SER);
    gpio_init(SS2_SER);
    gpio_init(SS3_SER);
    gpio_init(SS4_SER);
    gpio_init(SD_DETECT);

    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_set_dir(COM_RCLK, GPIO_OUT);
    gpio_set_dir(COM_SRCLK, GPIO_OUT);
    gpio_set_dir(SS1_SER, GPIO_OUT);
    gpio_set_dir(SS2_SER, GPIO_OUT);
    gpio_set_dir(SS3_SER, GPIO_OUT);
    gpio_set_dir(SS4_SER, GPIO_OUT);
    gpio_set_dir(SD_DETECT, GPIO_IN);

    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    gpio_put(COM_RCLK, 0);
    gpio_put(COM_SRCLK, 0);
    gpio_put(SS1_SER, 0);
    gpio_put(SS2_SER, 0);
    gpio_put(SS3_SER, 0);
    gpio_put(SS4_SER, 0);

    gpio_pull_up(SD_DETECT);
}

bool update_frame(repeating_timer_t *rt)
{
    cube_t *cube = rt->user_data;
    static int brightness;

    brightness = (brightness + 1) & 0x0f;
    printf("update frame: brn=%d\n", brightness);

    for (int i = 0; i < 5; i++)
    {
        for (int j = 0; j < 13; j++)
        {
            cube->frame[i][j] = brightness | brightness << 4;
        }
    }

    return true;
}

int main()
{
    // initialize serial communication
    stdio_init_all();

    puts("INITIALIZING...");

    initialize_gpio();

    FRESULT res;
    FATFS fs = {};
    res = f_mount(&fs, "0:", 0);

    FIL fp = {};
    res = f_open(&fp, "0:test.txt", FA_READ);

    cube_t cube = {};

    repeating_timer_t rt;
    add_repeating_timer_ms(-1000, update_frame, &cube, &rt);

    while (true)
    {
        for (int j = 0; j < 16; j++)
        {
            for (int layer = 0; layer < 5; layer++)
            {
                gpio_put(COM_RCLK, 0);

                for (int q_num = 7; q_num >= 0; q_num--)
                {
                    gpio_put(COM_SRCLK, 0);
                    
                    size_t odd_shift = 4 * (q_num % 2);
                    gpio_put(SS1_SER, !(j < (cube.frame[layer][( 0 + q_num) / 2] >> odd_shift & 0x0f)));
                    gpio_put(SS2_SER, !(j < (cube.frame[layer][( 8 + q_num) / 2] >> odd_shift & 0x0f)));
                    gpio_put(SS3_SER, !(j < (cube.frame[layer][(16 + q_num) / 2] >> odd_shift & 0x0f)));

                    if (q_num == 0)
                    {
                        gpio_put(SS4_SER, !(j < (cube.frame[layer][(24 + q_num) / 2] >> odd_shift & 0x0f)));
                    }
                    else
                    {
                        gpio_put(SS4_SER, (q_num - 1) == layer);
                    }

                    gpio_put(COM_SRCLK, 1);
                }

                gpio_put(COM_RCLK, 1);
                sleep_us(16 + 8 * j);
            }
        }
    }

    return 0;
}
