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
                sleep_ms(5);
            }
        }
    }
}

bool mount_sd(const TCHAR* drive, FATFS *fsp)
{
    if (gpio_get(SD_DETECT))
    {
        puts("SD card is not set");
        return false;
    }

    puts("SD card is set.");

    puts("Mounting SD card...");

    FRESULT res = f_mount(fsp, drive, 1);
    if (res != FR_OK || fsp->fs_type == 0)
    {
        printf("Failed to mount SD card (result = %d, filesystem type = %d)\n", res, fsp->fs_type);
        return false;
    }
    
    printf("Successfully mounted SD card (filesystem type = %d)\n", fsp->fs_type);
    
    return true;
}

bool open_file(const TCHAR* path, FIL *fpp)
{
    printf("Opening file %s\n", path);

    FRESULT res = f_open(fpp, path, FA_READ);
    if (res != FR_OK)
    {
        printf("Failed to open %s: %d\n", path, res);
        return false;
    }
    else
    {
        printf("Successfully opened %s\n", path);
    }

    return true;
}

bool verify_signature(FIL *fp)
{
    uint8_t signature[4] = {};
    size_t read_size;
    FRESULT res = f_read(fp, signature, 4, &read_size);

    if (res == FR_OK && read_size != 4)
    {
        printf("Failed to read signature: %d\n", res);
        return false;
    }

    if (signature[0] != 0x43 && signature[1] != 0x0d && signature[2] != 0x0a && signature[3] != 0x00)
    {
        printf("signature is invalid: %x, %x, %x, %x\n", signature[0], signature[1], signature[2], signature[3]);
        return false;
    }

    return true;
}

void dump_bytes(FIL *fpp, const size_t size)
{
    printf("Dumping the first %d bytes...\n", size);
    puts("\\/\\/\\/\\/ DUMP OF CUBE.CBI \\/\\/\\/\\/");

    // back up the current position
    const size_t current_pos = f_tell(fpp);

    f_lseek(fpp, 0);

    printf("   0  1  2  3  4  5  6  7   8  9  a  b  c  d  e  f \n");
    printf("  -------------------------------------------------");

    for (size_t i = 0; i < size; i++)
    {
        if (i % 16 == 0)
        {
            puts("");
            printf("%02X|", i);
        }
        else if (i % 8 == 0)
        {
            printf(" ");
        }

        uint8_t byte;
        size_t read_size;
        FRESULT res = f_read(fpp, &byte, 1, &read_size);

        if (res == FR_OK && read_size != 1)
        {
            break;
        }

        printf("%02X ", byte);
    }

    puts("");
    puts("/\\/\\/\\/\\ DUMP OF CUBE.CBI /\\/\\/\\/\\");

    // restore the position
    f_lseek(fpp, current_pos);
}

bool update_frame(repeating_timer_t *prt)
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
    // Baud rate: 115200
    stdio_init_all();

    stdio

    sleep_ms(5000);

    puts("INITIALIZING...");
    printf("Build timestamp: %s %s\n", __DATE__, __TIME__);

    initialize_gpio();

    // Open patterns from SD card
    FATFS fs = {};
    FIL fp = {};

    {
        bool suc_mount = mount_sd("0:", &fs);
        bool suc_open = suc_mount ? open_file("entry.cbi", &fp) : false;
        if (!suc_mount || !suc_open)
        {
            puts("Error: No cube patterns");
            set_error_cube_pattern_and_halt();
        }

        puts("Using cube patterns from SD card");
    }

    // Verify signature
    {
        bool suc_verify = verify_signature(&fp);
        if (!suc_verify)
        {
            set_error_cube_pattern_and_halt();
        }

        puts("Successfully verified signature");
    }

    // Read frame count
    int32_t frame_max = 0;
    {
        size_t read_size;
        FRESULT res = f_read(&fp, &frame_max, 4, &read_size);

        if (res == FR_OK && read_size != 4)
        {
            puts("Failed to read frame count");
            set_error_cube_pattern_and_halt();
        }

        printf("frame count = %ld\n", frame_max);
    }

    // Initialize repeating timer
    static repeating_timer_t rt = {};
    {
        bool suc_timer = add_repeating_timer_us(16, &update_frame, NULL, &rt);
        if (!suc_timer)
        {
            puts("Error: No slot for timer");
            set_error_cube_pattern_and_halt();
        }

        puts("Successfully registered a repeating timer");
    }

    puts("Successfully initialized!");
    
    dump_bytes(&fp, 256);

    // Main loop
    int32_t frame_count = 0;
    while (true)
    {
        // Read the next frame from fp
        {
            cube_t temp;
            size_t read_size1, read_size2;
            FRESULT res1, res2;

            res1 = f_read(&fp, &temp.frame, sizeof(temp.frame), &read_size1);
            res2 = f_read(&fp, &temp.duration, sizeof(temp.duration), &read_size2);
            if (res1 != FR_OK || res2 != FR_OK || read_size1 != sizeof(temp.frame) || read_size2 != sizeof(temp.duration))
            {
                printf("Error on reading frame: res1=%d, read_size1=%d, res2=%d, read_size2=%d\n", res1, read_size1, res2, read_size2);
                set_error_cube_pattern_and_halt();
            }

            cube = temp; 
        }

        printf("Successfully updated: count=%ld/%ld, duration=%ld\n", frame_count, frame_max, cube.duration);

        sleep_ms(cube.duration);

        // Count up and return to 0
        frame_count++;
        if (frame_max <= frame_count)
        {
            frame_count = 0;
            f_lseek(&fp, 8);
        }
    }

    return 0;
}
