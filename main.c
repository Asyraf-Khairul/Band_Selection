#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"

// ============================================================
//   OLED CONFIG – SSD1306
// ============================================================
#define I2C_PORT i2c1
#define I2C_SDA 18
#define I2C_SCL 19
#define OLED_ADDR 0x3C   // typical for SSD1306
#define WIDTH  128
#define HEIGHT 64
#define PAGES  (HEIGHT / 8)

static uint8_t framebuffer[PAGES][WIDTH];

// ============================================================
//   ENCODER & BUTTON PINS
// ============================================================
#define ENC_CLK 21
#define ENC_DT  20
#define ENC_SW  5

// ============================================================
//   LPF CONTROL PINS
// ============================================================
#define LPF_S2 2
#define LPF_S1 3
#define LPF_S0 4

// ============================================================
//   LED
// ============================================================
#define LED_PIN 25

// ============================================================
//   OLED FUNCTIONS
// ============================================================
void oled_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};
    i2c_write_blocking(I2C_PORT, OLED_ADDR, buf, 2, false);
}

void oled_data(const uint8_t *data, size_t len) {
    uint8_t packet[129];
    packet[0] = 0x40;
    for (size_t off = 0; off < len; off += 128) {
        size_t chunk = len - off;
        if (chunk > 128) chunk = 128;
        memcpy(&packet[1], &data[off], chunk);
        i2c_write_blocking(I2C_PORT, OLED_ADDR, packet, chunk + 1, false);
    }
}

void oled_init(void) {
    sleep_ms(100);
    oled_cmd(0xAE);  // Display OFF
    oled_cmd(0xD5); oled_cmd(0x80);
    oled_cmd(0xA8); oled_cmd(0x3F);
    oled_cmd(0xD3); oled_cmd(0x00);
    oled_cmd(0x40);
    oled_cmd(0x8D); oled_cmd(0x14);  // SSD1306 charge pump
    oled_cmd(0xA1);
    oled_cmd(0xC8);
    oled_cmd(0xDA); oled_cmd(0x12);
    oled_cmd(0x81); oled_cmd(0x7F);
    oled_cmd(0xD9); oled_cmd(0x22);
    oled_cmd(0xDB); oled_cmd(0x40);
    oled_cmd(0xAF);  // Display ON
}

void oled_clear(void) {
    memset(framebuffer, 0, sizeof(framebuffer));
}

void oled_update(void) {
    for (int page = 0; page < PAGES; page++) {
        oled_cmd(0xB0 + page);
        oled_cmd(0x00);       // SSD1306 column start = 0
        oled_cmd(0x10);
        oled_data(framebuffer[page], WIDTH);
    }
}

void oled_pixel(int x, int y, bool on) {
    if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) return;
    int page = y / 8;
    int bit  = y % 8;
    if (on) framebuffer[page][x] |=  (1 << bit);
    else    framebuffer[page][x] &= ~(1 << bit);
}

// ----- Font (A-Z, 0-9, space) -----
static const uint8_t font[][5] = {
    {0x7E,0x11,0x11,0x11,0x7E}, // A
    {0x7F,0x49,0x49,0x49,0x36}, // B
    {0x3E,0x41,0x41,0x41,0x22}, // C
    {0x7F,0x41,0x41,0x41,0x3E}, // D
    {0x7F,0x49,0x49,0x49,0x41}, // E
    {0x7F,0x09,0x09,0x09,0x01}, // F
    {0x3E,0x41,0x49,0x49,0x7A}, // G
    {0x7F,0x08,0x08,0x08,0x7F}, // H
    {0x41,0x41,0x7F,0x41,0x41}, // I
    {0x20,0x40,0x41,0x3F,0x01}, // J
    {0x7F,0x08,0x14,0x22,0x41}, // K
    {0x7F,0x40,0x40,0x40,0x40}, // L
    {0x7F,0x02,0x04,0x02,0x7F}, // M
    {0x7F,0x02,0x04,0x08,0x7F}, // N
    {0x3E,0x41,0x41,0x41,0x3E}, // O
    {0x7F,0x09,0x09,0x09,0x06}, // P
    {0x3E,0x41,0x51,0x21,0x5E}, // Q
    {0x7F,0x09,0x19,0x29,0x46}, // R
    {0x46,0x49,0x49,0x49,0x31}, // S
    {0x01,0x01,0x7F,0x01,0x01}, // T
    {0x3F,0x40,0x40,0x40,0x3F}, // U
    {0x1F,0x20,0x40,0x20,0x1F}, // V
    {0x3F,0x40,0x38,0x40,0x3F}, // W
    {0x63,0x14,0x08,0x14,0x63}, // X
    {0x07,0x08,0x70,0x08,0x07}, // Y
    {0x61,0x51,0x49,0x45,0x43}, // Z
    {0x3E,0x51,0x49,0x45,0x3E}, // 0
    {0x00,0x42,0x7F,0x40,0x00}, // 1
    {0x62,0x51,0x49,0x49,0x46}, // 2
    {0x22,0x41,0x49,0x49,0x36}, // 3
    {0x1C,0x12,0x11,0x7F,0x10}, // 4
    {0x27,0x45,0x45,0x45,0x39}, // 5
    {0x3E,0x49,0x49,0x49,0x32}, // 6
    {0x01,0x01,0x71,0x09,0x07}, // 7
    {0x36,0x49,0x49,0x49,0x36}, // 8
    {0x26,0x49,0x49,0x49,0x3E}, // 9
    {0x00,0x00,0x00,0x00,0x00}, // space (index 36)
};

void oled_char(int x, int y, char c) {
    int idx;
    if (c >= 'A' && c <= 'Z') idx = c - 'A';
    else if (c >= '0' && c <= '9') idx = c - '0' + 26;
    else if (c == ' ') idx = 36;
    else return;
    for (int col = 0; col < 5; col++) {
        uint8_t data = font[idx][col];
        for (int row = 0; row < 8; row++) {
            if (data & (1 << row))
                oled_pixel(x + col, y + row, true);
        }
    }
}

void oled_char_scaled(int x, int y, char c, int scale) {
    int idx;
    if (c >= 'A' && c <= 'Z') idx = c - 'A';
    else if (c >= '0' && c <= '9') idx = c - '0' + 26;
    else if (c == ' ') idx = 36;
    else return;

    for (int col = 0; col < 5; col++) {
        uint8_t data = font[idx][col];
        for (int row = 0; row < 8; row++) {
            if (data & (1 << row)) {
                for (int dx = 0; dx < scale; dx++) {
                    for (int dy = 0; dy < scale; dy++) {
                        oled_pixel(x + col * scale + dx, y + row * scale + dy, true);
                    }
                }
            }
        }
    }
}

void oled_string(const char *str, int x, int y) {
    while (*str) {
        oled_char(x, y, *str);
        x += 6;
        str++;
    }
}

void oled_string_scaled(const char *str, int x, int y, int scale) {
    while (*str) {
        oled_char_scaled(x, y, *str, scale);
        x += 6 * scale;
        str++;
    }
}

// ============================================================
//   ENCODER & BUTTON
// ============================================================
volatile int encoder_value = 0;
volatile bool encoder_changed = false;
volatile bool button_pressed = false;
volatile int encoder_steps = 0;

void encoder_callback(uint gpio, uint32_t events) {
    static int last_state = -1;
    static int transition_count = 0;
    static const int8_t transition_table[16] = {
         0, -1,  1,  0,
         1,  0,  0, -1,
        -1,  0,  0,  1,
         0,  1, -1,  0
    };

    int state_now = (gpio_get(ENC_CLK) << 1) | gpio_get(ENC_DT);
    if (last_state < 0) {
        last_state = state_now;
        return;
    }

    int movement = transition_table[(last_state << 2) | state_now];
    if (movement != 0) {
        transition_count += movement;
        if (transition_count >= 4) {
            encoder_steps++;
            transition_count = 0;
            encoder_changed = true;
        } else if (transition_count <= -4) {
            encoder_steps--;
            transition_count = 0;
            encoder_changed = true;
        }
    }

    last_state = state_now;
}

bool button_debounced(void) {
    static uint32_t last_time = 0;
    static bool was_pressed = false;
    static bool press_handled = false;
    uint32_t now = to_ms_since_boot(get_absolute_time());
    bool is_pressed = !gpio_get(ENC_SW);

    if (is_pressed != was_pressed) {
        last_time = now;
        was_pressed = is_pressed;
        if (!is_pressed) press_handled = false;
    }

    if (is_pressed && !press_handled && now - last_time >= 30) {
        press_handled = true;
        return true;
    }

    return false;
}

// ============================================================
//   LPF CONTROL
// ============================================================
// Filter | S2 (GP2) | S1 (GP3) | S0 (GP4)
// LPF 1  (0-2 MHz)   | 5V (1) | GND (0) | GND (0)
// LPF 2  (2-4 MHz)   | GND (0)| 5V (1) | 5V (1)
// LPF 3  (4-8 MHz)   | GND (0)| 5V (1) | GND (0)
// LPF 4  (8-16 MHz)  | GND (0)| GND (0)| 5V (1)
// LPF 5  (16-30 MHz) | GND (0)| GND (0)| GND (0)

typedef struct {
    const char *name;
    int s2;
    int s1;
    int s0;
} lpf_band_t;

lpf_band_t bands[] = {
    {"   0 to 2 MHZ",    1, 0, 0},
    {"   2 to 4 MHZ",    0, 1, 1},
    {"   4 to 8 MHZ",    0, 1, 0},
    {"   8 to 16 MHZ",   0, 0, 1},
    {"  16 to 30 MHZ",  0, 0, 0}
};

#define NUM_BANDS (sizeof(bands) / sizeof(bands[0]))

void set_lpf_band(int index) {
    if (index < 0 || index >= NUM_BANDS) return;
    gpio_put(LPF_S2, bands[index].s2);
    gpio_put(LPF_S1, bands[index].s1);
    gpio_put(LPF_S0, bands[index].s0);
}

// ============================================================
//   MENU STATES
// ============================================================
typedef enum {
    STATE_IDLE,           // "Click to select LPF band"
    STATE_SELECTING,      // "Select band: LPF X"
    STATE_CONFIRMED       // "Band set! Click to change"
} menu_state_t;

menu_state_t state = STATE_IDLE;
int selected_band = 0;

// ============================================================
//   DRAW FUNCTIONS
// ============================================================
void draw_idle_screen(void) {
    oled_clear();
    oled_string("BAND SELECTOR", 25, 0);
    oled_string_scaled("CLICK", 34, 18, 2);
    oled_string("TO SELECT BAND", 20, 44);
    oled_update();
}

void draw_selecting_screen(void) {
    oled_clear();
    oled_string("SELECT BAND", 32, 0);
    char band_title[6];
    sprintf(band_title, "LPF %d", selected_band + 1);
    oled_string_scaled(band_title, 34, 16, 2);
    oled_string(bands[selected_band].name, 16, 38);
    oled_string("CLICK TO SET", 28, 56);
    oled_update();
}

void draw_confirmed_screen(void) {
    oled_clear();
    oled_string_scaled("SET", 44, 6, 2);
    int x = (WIDTH - strlen(bands[selected_band].name) * 6) / 2;
    oled_string(bands[selected_band].name, 17, 28);
    oled_string("CLICK TO CHANGE", 20, 50);
    oled_update();
}

// ============================================================
//   MAIN
// ============================================================
int main() {
    stdio_init_all();
    sleep_ms(500);

    // LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // Startup blink
    for (int i = 0; i < 3; i++) {
        gpio_put(LED_PIN, 1);
        sleep_ms(100);
        gpio_put(LED_PIN, 0);
        sleep_ms(100);
    }

    // I2C
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // OLED
    oled_init();
    oled_clear();
    draw_idle_screen();

    // LPF control pins
    gpio_init(LPF_S2);
    gpio_init(LPF_S1);
    gpio_init(LPF_S0);
    gpio_set_dir(LPF_S2, GPIO_OUT);
    gpio_set_dir(LPF_S1, GPIO_OUT);
    gpio_set_dir(LPF_S0, GPIO_OUT);
    // Default to LPF 1
    set_lpf_band(0);

    // Encoder pins
    gpio_init(ENC_CLK);
    gpio_init(ENC_DT);
    gpio_init(ENC_SW);
    gpio_set_dir(ENC_CLK, GPIO_IN);
    gpio_set_dir(ENC_DT, GPIO_IN);
    gpio_set_dir(ENC_SW, GPIO_IN);
    gpio_pull_up(ENC_CLK);
    gpio_pull_up(ENC_DT);
    gpio_pull_up(ENC_SW);

    // Encoder interrupt
    gpio_set_irq_enabled_with_callback(ENC_CLK, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &encoder_callback);
    gpio_set_irq_enabled(ENC_DT, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);

    bool redraw = true;
    bool led_on = false;
    uint32_t last_led_toggle = to_ms_since_boot(get_absolute_time());

    while (1) {
        // ---- Handle button press ----
        if (button_debounced()) {
            button_pressed = true;
        }

        if (button_pressed) {
            button_pressed = false;

            switch (state) {
                case STATE_IDLE:
                    state = STATE_SELECTING;
                    selected_band = 0;
                    redraw = true;
                    break;

                case STATE_SELECTING:
                    // Confirm selection – set the LPF
                    set_lpf_band(selected_band);
                    state = STATE_CONFIRMED;
                    redraw = true;
                    break;

                case STATE_CONFIRMED:
                    state = STATE_SELECTING;
                    encoder_value = selected_band;
                    redraw = true;
                    break;
            }
        }

        // ---- Handle encoder rotation ----
        if (encoder_changed) {
            encoder_changed = false;

            if (state == STATE_SELECTING) {
                int steps = encoder_steps;
                encoder_steps = 0;
                selected_band += steps;

                // Clamp to valid range
                if (selected_band < 0) selected_band = 0;
                if (selected_band >= NUM_BANDS) selected_band = NUM_BANDS - 1;
                encoder_value = selected_band;
                redraw = true;
            }
        }

        // ---- Redraw if needed ----
        if (redraw) {
            switch (state) {
                case STATE_IDLE:
                    draw_idle_screen();
                    break;
                case STATE_SELECTING:
                    draw_selecting_screen();
                    break;
                case STATE_CONFIRMED:
                    draw_confirmed_screen();
                    break;
            }
            redraw = false;
        }

        // ---- Blink LED alive without blocking button reads ----
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - last_led_toggle >= 200) {
            led_on = !led_on;
            gpio_put(LED_PIN, led_on);
            last_led_toggle = now;
        }

        sleep_ms(5);
    }
}
