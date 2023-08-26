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

#if 0
#define debug_puts(str) puts((str))
#else
#define debug_puts(_)
#endif

typedef struct
{
    uint8_t frame[5][13];
    int32_t duration;
} cube_t;

volatile cube_t cube;

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

void set_cube_value(const size_t x, const size_t y, const size_t z, const int8_t v)
{
    const size_t odd_shift = 4 * ((5 * x + z) % 2);
    cube.frame[y][(5 * x + z) / 2] &= ~(0x0f << odd_shift);
    cube.frame[y][(5 * x + z) / 2] |= (v & 0x0f) << odd_shift;
}

void fill_cube_with_value(const int8_t v)
{
    for (size_t i=0; i < 5; i++)
    {
        for (size_t j=0; j < 13; j++)
        {
            cube.frame[i][j] = (v & 0x0f) | ((v & 0x0f) << 4);
        }
    }
}

void set_error_cube_pattern_and_halt()
{
    fill_cube_with_value(0);
    while (true)
    {

        for (size_t i=0; i < 125; i++)
        {
            const size_t y = i / 25;
            const size_t x = (i % 25) / 5;
            const size_t z = i % 5;

            const size_t i2 = (i + 1) % 125;
            const size_t y2 = i2 / 25;
            const size_t x2 = (i2 % 25) / 5;
            const size_t z2 = i2 % 5;

            for (size_t v=0; v < 16; v++)
            {
                set_cube_value(x, y, z, 15 - v);
                set_cube_value(x2, y2, z2, v);

                sleep_ms(25);
            }
        }
    }
}

bool open_file(const TCHAR* path, FIL *fpp)
{
    FRESULT res;
    FATFS fs = {};

    if (gpio_get(SD_DETECT))
    {
        puts("SD card is not set");
        return false;
    }
    else
    {
        puts("SD card is set.");
    }

    const TCHAR drive[3] = { path[0], path[1], '\0' };
    res = f_mount(&fs, "0:", 1);
    if (res != FR_OK || fs.fs_type == 0)
    {
        printf("Failed to mount SD card (result = %d, filesystem type = %d)\n", res, fs.fs_type);
        return false;
    }
    else
    {
        printf("Successfully mounted SD card (filesystem type = %d)\n", fs.fs_type);
    }

    res = f_open(fpp, path, FA_READ);
    if (res != FR_OK)
    {
        printf("Failed to open test.cbi: %d\n", res);
        return false;
    }
    else
    {
        puts("Successfully opened test.cbi");
    }

    return true;
}

bool update_frame(repeating_timer_t *rtp)
{
    static int64_t c;
    gpio_put(PICO_DEFAULT_LED_PIN, c / 10000);
    if (c++ == 20000) c = 0;

    static size_t i;
    static size_t j;
    static int layer;
    // printf("i = %d, j = %d, layer = %d\n", i, j, layer);

    debug_puts("a");

    if (j + 2 <= i)
    {
        debug_puts("a'1");
        i = 0;
    }
    else
    {
        debug_puts("a'2");
        i++;
        return true;
    }
    debug_puts("b");

    gpio_put(COM_RCLK, 0);
    debug_puts("c");

    for (int q_num = 7; q_num >= 0; q_num--)
    {
        gpio_put(COM_SRCLK, 0);
        debug_puts("c'");
        
        if (5 <= layer) printf("layer = %d", layer);
        if (13 <= ( 0 + q_num) / 2) printf("( 0 + q_num) / 2 = %d\n", ( 0 + q_num) / 2);
        if (13 <= ( 8 + q_num) / 2) printf("( 8 + q_num) / 2 = %d\n", ( 8 + q_num) / 2);
        if (13 <= (16 + q_num) / 2) printf("(16 + q_num) / 2 = %d\n", (16 + q_num) / 2);

        const size_t odd_shift = 4 * (q_num % 2);
        gpio_put(SS1_SER, (cube.frame[layer][( 0 + q_num) / 2] >> odd_shift & 0x0f) <= j);
        gpio_put(SS2_SER, (cube.frame[layer][( 8 + q_num) / 2] >> odd_shift & 0x0f) <= j);
        gpio_put(SS3_SER, (cube.frame[layer][(16 + q_num) / 2] >> odd_shift & 0x0f) <= j);

        if (q_num == 0)
        {
            gpio_put(SS4_SER, (cube.frame[layer][12] >> odd_shift & 0x0f) <= j);
        }
        else
        {
            gpio_put(SS4_SER, (q_num - 1) == 4 - layer);
        }

        gpio_put(COM_SRCLK, 1);
    }
    debug_puts("d");

    gpio_put(COM_RCLK, 1);

    layer++;
    if (5 <= layer)
    {
        layer = 0;

        j++;
        if (16 <= j)
        {
            j = 0;
        }
    }

    return true;
}

int main()
{
    // initialize serial communication
    stdio_init_all();

    sleep_ms(1000);

    puts("INITIALIZING...");
    printf("Build timestamp: %s %s\n", __DATE__, __TIME__);

    initialize_gpio();

    FIL fp = {};
    bool suc_open = true;//open_file("0:test.cbi", &fp);
    if (suc_open)
    {
        puts("Using cube patterns from SD card");
    }
    else
    {
        puts("No cube patterns");
    }

    static repeating_timer_t rt = {};
    bool suc_timer = add_repeating_timer_us(16, &update_frame, NULL, &rt);
    if (suc_timer) {
        puts("Successfully registered a repeating timer");
    }
    else
    {
        puts("No slot for timer");
    }

    if (!suc_open || !suc_timer)
    {
        set_error_cube_pattern_and_halt();
    }

    puts("Successfully initialized!");

    set_error_cube_pattern_and_halt();

    while (true)
    {
        size_t read_size1, read_size2;
        cube_t temp;
        FRESULT res1 = f_read(&fp, &temp.frame, 5 * 13, &read_size1);
        FRESULT res2 = f_read(&fp, &temp.duration, 4, &read_size2);
        cube = temp;

        if (res1 == FR_OK && read_size1 == 5 * 13 && res2 == FR_OK && read_size2 == 4)
        {
            printf("Successfully updated: duration = %d\n", cube.duration);

            sleep_ms(cube.duration);
        }
        else
        {
            puts("Error on reading or EOF");

            for (int i = 0; i < 5; i++)
            {
                for (int j = 0; j < 13; j++)
                {
                    cube.frame[i][j] = 0xff;
                }
            }

            sleep_ms(1000);
        }
    }

    return 0;
}
