#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/i2c.h"
#include "gesture.h"

// ── Pin config ────────────────────────────────────────────────
#define I2C_PORT    i2c0
#define SDA_PIN     4
#define SCL_PIN     5

// ── APDS-9960 register addresses ─────────────────────────────
#define APDS_ADDR   0x39

#define REG_ENABLE  0x80
#define REG_ID      0x92
#define REG_GPENTH  0xA0
#define REG_GEXTH   0xA1
#define REG_GCONF1  0xA2
#define REG_GCONF2  0xA3
#define REG_GPULSE  0xA6
#define REG_GCONF3  0xAA
#define REG_GCONF4  0xAB
#define REG_GFLVL   0xAE
#define REG_GSTATUS 0xAF
#define REG_GFIFO_U 0xFC
#define REG_PPULSE  0x8E
#define REG_CONFIG2 0x90
#define REG_WTIME   0x83

#define APDS_ID_1   0xAB
#define APDS_ID_2   0x9C

// ── I2C helpers ───────────────────────────────────────────────
static void reg_write(uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    i2c_write_blocking(I2C_PORT, APDS_ADDR, buf, 2, false);
}

static bool reg_read(uint8_t reg, uint8_t *buf, uint8_t len) {
    int ret = i2c_write_blocking(I2C_PORT, APDS_ADDR, &reg, 1, true);
    if (ret < 0) return false;
    ret = i2c_read_blocking(I2C_PORT, APDS_ADDR, buf, len, false);
    return ret == len;
}

// ── Sensor init ───────────────────────────────────────────────
bool apds_init(void) {
    // I2C hardware init
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
    printf("[INFO]  I2C initialized at 400kHz\n");

    // Verify device ID
    uint8_t id;
    if (!reg_read(REG_ID, &id, 1)) {
        printf("[ERROR] I2C read failed — check wiring\n");
        return false;
    }
    printf("[INFO]  Device ID: 0x%02X", id);
    if (id == APDS_ID_1 || id == APDS_ID_2) {
        printf("  ✓ correct\n");
    } else {
        printf("  ✗ unexpected (expected 0xAB or 0x9C)\n");
        return false;
    }

    // Configure sensor
    reg_write(REG_ENABLE,  0x00);             // disable everything first
    reg_write(REG_PPULSE,  0x89);             // 16us, 10 proximity pulses
    reg_write(REG_GCONF1,  0x40);             // GFIFOTH=4 datasets
    reg_write(REG_GCONF2,  0x41);             // GGAIN=4x, GWTIME=2.8ms
    reg_write(REG_GPULSE,  0xC9);             // 32us, 10 gesture pulses
    reg_write(REG_GCONF3,  0x00);             // all 4 photodiodes active
    reg_write(REG_GPENTH,  150);              // enter gesture at proximity 150
    reg_write(REG_GEXTH,   150);              // exit  gesture at proximity 150
    reg_write(REG_GCONF4,  0x00);             // let hardware manage GMODE via GPENTH/GEXTH
    reg_write(REG_WTIME,   0xFF);             // max wait time between proximity cycles
    reg_write(REG_CONFIG2, 0x01 | (3 << 4)); // LED boost 300%
    reg_write(REG_ENABLE,  0x01 | 0x08 | 0x04 | 0x40); // PON + WEN + PEN + GEN

    printf("[INFO]  Sensor initialized\n");
    return true;
}

// ── Gesture read (stateful FIFO accumulator) ──────────────────
const char *read_gesture(void) {
    static int up=0, down=0, left=0, right=0;
    static bool accumulating = false;

    uint8_t gstatus;
    if (!reg_read(REG_GSTATUS, &gstatus, 1))
        return NULL;

    if (gstatus & 0x01) {
        // GVALID=1: gesture in progress — drain FIFO and accumulate
        uint8_t level;
        if (!reg_read(REG_GFLVL, &level, 1) || level == 0)
            return NULL;

        uint8_t data[128];
        if (!reg_read(REG_GFIFO_U, data, level * 4))
            return NULL;

        for (int i = 0; i < level; i++) {
            up    += data[i*4 + 0];
            down  += data[i*4 + 1];
            left  += data[i*4 + 2];
            right += data[i*4 + 3];
        }
        accumulating = true;
        return NULL;  // gesture not complete yet

    } else if (accumulating) {
        // GVALID=0: gesture ended — compute direction from all accumulated data
        accumulating = false;

        int ud = up - down;
        int lr = left - right;

        // printf("  [raw] U=%-4d D=%-4d L=%-4d R=%-4d  ud=%d lr=%d\n",
        //        up, down, left, right, ud, lr);

        up = down = left = right = 0;  // reset for next gesture

        int abs_ud = ud < 0 ? -ud : ud;
        int abs_lr = lr < 0 ? -lr : lr;
        if (abs_ud > abs_lr) {
            if (ud >  100) return "DOWN";
            if (ud < -100) return "UP";
        } else {
            if (lr >  100) return "RIGHT";
            if (lr < -100) return "LEFT";
        }
    }

    return NULL;
}

// ── Core 1 entry point ────────────────────────────────────────
static uint32_t gesture_to_int(const char *g) {
    if (g[0] == 'U') return DIR_UP;
    if (g[0] == 'D') return DIR_DOWN;
    if (g[0] == 'L') return DIR_LEFT;
    if (g[0] == 'R') return DIR_RIGHT;
    return 0;
}
