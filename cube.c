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

typedef struct
{
    FIL fp;
    uint64_t size;
    cube_t cube;
} shared_t;

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
    shared_t *shared = rt->user_data;
    static int32_t counter;

    if (counter <= 0)
    {
        size_t read_size1, read_size2;
        FRESULT res1 = f_read(&shared->fp, &shared->cube.frame, 5 * 13, &read_size1);
        FRESULT res2 = f_read(&shared->fp, &shared->cube.duration, 4, &read_size2);

        if (res1 == FR_OK && read_size1 == 5 * 13 && res2 == FR_OK && read_size2 == 4)
        {
            printf("successfully updated: duration = %d\n", shared->cube.duration);

            counter = shared->cube.duration;
        }
        else
        {
            puts("error on reading or eof");

            counter = 1000;

            for (int i = 0; i < 5; i++)
            {
                for (int j = 0; j < 13; j++)
                {
                    shared->cube.frame[i][j] = 0xff;
                }
            }
        }
    }
    else
    {
        counter--;
    }

    return true;
}

int main()
{
    // initialize serial communication
    stdio_init_all();

    sleep_ms(5000);

    puts("INITIALIZING...");
    printf("build timestamp: %s %s\n", __DATE__, __TIME__);

    initialize_gpio();

    if (false && gpio_get(SD_DETECT))
    {
        while (true)
        {
            puts("sd card is not set");
            sleep_ms(1000);
        }
    }
    else
    {
        puts("sd card is set");
    }

    FRESULT res;
    FATFS fs = {};
    res = f_mount(&fs, "0:", 1);
    if (res != FR_OK || fs.fs_type == 0)
    {
        while (true)
        {
            printf("failed to mount sd card: res = %d, type = %d\n", res, fs.fs_type);
            sleep_ms(1000);
        }
    }
    else
    {
        printf("successfully mounted sd card: type = %d", fs.fs_type);
    }

    shared_t shared = {};
    
    res = f_open(&shared.fp, "0:test.cbi", FA_READ);
    if (res != FR_OK)
    {
        while (true)
        {
            printf("failed to open test.cbi: %d\n", res);
            sleep_ms(1000);
        }
    }
    else
    {
        puts("successfully opened test.cbi");
    }

    repeating_timer_t rt;
    add_repeating_timer_us(-1000, update_frame, &shared, &rt);

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
                    gpio_put(SS1_SER, !(j < (shared.cube.frame[layer][( 0 + q_num) / 2] >> odd_shift & 0x0f)));
                    gpio_put(SS2_SER, !(j < (shared.cube.frame[layer][( 8 + q_num) / 2] >> odd_shift & 0x0f)));
                    gpio_put(SS3_SER, !(j < (shared.cube.frame[layer][(16 + q_num) / 2] >> odd_shift & 0x0f)));

                    if (q_num == 0)
                    {
                        gpio_put(SS4_SER, !(j < (shared.cube.frame[layer][(24 + q_num) / 2] >> odd_shift & 0x0f)));
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
