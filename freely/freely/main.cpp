#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <orbis/libkernel.h>
#include "graphics.h"

static const int SCREEN_W = 1920;
static const int SCREEN_H = 1080;
static FILE *g_log = NULL;
static unsigned int g_log_count = 0;

struct RGB { uint8_t r, g, b; };
static const RGB BG = {7, 9, 14};
static const RGB PANEL = {15, 19, 28};
static const RGB PANEL2 = {20, 25, 36};
static const RGB WHITE = {239, 243, 250};
static const RGB MUTED = {135, 148, 166};
static const RGB GREEN = {102, 235, 176};
static const RGB YELLOW = {255, 204, 102};
static const RGB BLUE = {80, 190, 255};
static const RGB CYAN = {70, 235, 245};
static const RGB PURPLE = {190, 130, 255};
static const RGB RED = {255, 105, 125};
static const RGB DIM = {65, 82, 105};

static const char *FONT_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789:-./_?";
static const uint8_t FONT[][5] = {
 {0x7e,0x11,0x11,0x11,0x7e},{0x7f,0x49,0x49,0x49,0x36},
 {0x3e,0x41,0x41,0x41,0x22},{0x7f,0x41,0x41,0x22,0x1c},
 {0x7f,0x49,0x49,0x49,0x41},{0x7f,0x09,0x09,0x09,0x01},
 {0x3e,0x41,0x49,0x49,0x7a},{0x7f,0x08,0x08,0x08,0x7f},
 {0x00,0x41,0x7f,0x41,0x00},{0x20,0x40,0x41,0x3f,0x01},
 {0x7f,0x08,0x14,0x22,0x41},{0x7f,0x40,0x40,0x40,0x40},
 {0x7f,0x02,0x0c,0x02,0x7f},{0x7f,0x04,0x08,0x10,0x7f},
 {0x3e,0x41,0x41,0x41,0x3e},{0x7f,0x09,0x09,0x09,0x06},
 {0x3e,0x41,0x51,0x21,0x5e},{0x7f,0x09,0x19,0x29,0x46},
 {0x46,0x49,0x49,0x49,0x31},{0x01,0x01,0x7f,0x01,0x01},
 {0x3f,0x40,0x40,0x40,0x3f},{0x1f,0x20,0x40,0x20,0x1f},
 {0x3f,0x40,0x38,0x40,0x3f},{0x63,0x14,0x08,0x14,0x63},
 {0x07,0x08,0x70,0x08,0x07},{0x61,0x51,0x49,0x45,0x43},
 {0x3e,0x51,0x49,0x45,0x3e},{0x00,0x42,0x7f,0x40,0x00},
 {0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4b,0x31},
 {0x18,0x14,0x12,0x7f,0x10},{0x27,0x45,0x45,0x45,0x39},
 {0x3c,0x4a,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},
 {0x36,0x49,0x49,0x49,0x36},{0x06,0x49,0x49,0x29,0x1e},
 {0x08,0x08,0x08,0x08,0x08},{0x00,0x60,0x60,0x00,0x00},
 {0x40,0x20,0x10,0x08,0x04},{0x08,0x08,0x08,0x08,0x08},
 {0x00,0x00,0x5f,0x00,0x00},{0x02,0x01,0x51,0x09,0x06}
};

static void rect(Scene2D *s, int x, int y, int w, int h, RGB c) {
    s->DrawRectangle(x, y, w, h, Color{c.r,c.g,c.b});
}
static void text(Scene2D *s, int x, int y, const char *value, int scale, RGB color) {
    int origin = x;
    for (size_t n=0; value && value[n]; ++n) {
        char ch = (char)toupper((unsigned char)value[n]);
        if (ch == ' ') { x += 4 * scale; continue; }
        const char *found = strchr(FONT_CHARS, ch);
        if (!found) { x += 6 * scale; continue; }
        const uint8_t *glyph = FONT[(size_t)(found - FONT_CHARS)];
        for (int col=0; col<5; ++col)
            for (int row=0; row<7; ++row)
                if (glyph[col] & (1u << row))
                    rect(s, x + col*scale, y + row*scale, scale, scale, color);
        x += 6 * scale;
        if (x > SCREEN_W - 40) break;
    }
    (void)origin;
}
static void log_line(const char *level, const char *message) {
    ++g_log_count;
    printf("[%04u] [%s] %s\n", g_log_count, level, message);
    if (g_log) {
        fprintf(g_log, "[%04u] [%s] %s\n", g_log_count, level, message);
        fflush(g_log);
    }
}
static void terminal_line(Scene2D *s, int y, const char *value, RGB color) {
    text(s, 76, y, value, 2, color);
}
static void term_prompt(Scene2D *s, int y, const char *tag, const char *msg, RGB tag_color, RGB msg_color) {
    text(s,76,y,"[",2,DIM);
    text(s,88,y,tag,2,tag_color);
    text(s,148,y,"]",2,DIM);
    text(s,166,y,msg,2,msg_color);
}
static void draw_terminal(Scene2D *s, bool log_ok) {
    rect(s,0,0,SCREEN_W,SCREEN_H,{5,7,13});
    rect(s,0,0,SCREEN_W,4,CYAN);
    text(s,64,30,"FREELY DEV",5,CYAN);
    text(s,64,78,"PS4 SYSTEM RECON - DEVELOPER CONSOLE",2,PURPLE);
    text(s,1540,42,"BUILD 0.3.0",2,GREEN);
    rect(s,64,116,1792,2,{28,43,62});

    text(s,76,142,"  ______ ____  ______ ______ _  __",2,BLUE);
    text(s,76,160," / ____// __ \\/ ____// ____// |/ /",2,BLUE);
    text(s,76,178,"/ /_   / /_/ / __/  / __/  /    /",2,BLUE);
    text(s,76,196,"\\__ /  \\____/____/ /_____/ /|_/ ",2,BLUE);
    text(s,76,214,"  /_/       DEVELOPER TERMINAL",2,PURPLE);

    rect(s,64,250,1792,2,{28,43,62});
    term_prompt(s,274,"BOOT","Freely Dev diagnostic environment started",GREEN,WHITE);
    term_prompt(s,306,"INFO","Target: PlayStation 4  |  Mode: READ ONLY",CYAN,WHITE);
    term_prompt(s,338,"OK",log_ok?"Log file opened: /data/FREELY-DEV.log":"Screen logging active; file log unavailable",GREEN,log_ok?GREEN:YELLOW);
    term_prompt(s,370,"SCAN","Starting system inventory...",PURPLE,WHITE);
    term_prompt(s,418,"CHECK","Video output interface ............. ACTIVE",CYAN,GREEN);
    term_prompt(s,450,"CHECK","Application runtime ................ ACTIVE",CYAN,GREEN);
    term_prompt(s,482,"WAIT","Firmware information ............... NOT READ",YELLOW,YELLOW);
    term_prompt(s,514,"WAIT","Kernel information ................. NOT READ",YELLOW,YELLOW);
    term_prompt(s,546,"WAIT","Console model ...................... NOT READ",YELLOW,YELLOW);
    term_prompt(s,578,"WAIT","GoldHEN status ..................... NOT VERIFIED",YELLOW,YELLOW);
    term_prompt(s,610,"TODO","CPU / memory telemetry ............. PENDING",PURPLE,MUTED);
    term_prompt(s,642,"TODO","Controller enumeration ............. PENDING",PURPLE,MUTED);
    term_prompt(s,674,"TODO","System module inventory ............ PENDING",PURPLE,MUTED);

    rect(s,64,720,1792,2,{28,43,62});
    text(s,76,742,"[ INSPECTOR STATUS ]",2,CYAN);
    text(s,76,778,"[",2,DIM); text(s,88,778,"PARTIAL",2,YELLOW); text(s,172,778,"]",2,DIM);
    text(s,196,778,"Read-only checks only. Unknown values are never guessed.",2,WHITE);
    text(s,76,826,"[ SAFETY ]",2,GREEN);
    text(s,196,826,"No kernel patches, spoofing, or system modifications executed.",2,MUTED);
    rect(s,64,882,1792,2,{28,43,62});
    text(s,76,908,"root@freely-dev:~#",2,GREEN);
    text(s,310,908,"waiting for next diagnostic module_",2,WHITE);
    text(s,76,1004,"FREELY DEV  /  SYSTEM INSPECTOR  /  TERMINAL UI",2,DIM);
    text(s,1570,1004,"PS4 HOMEbrew",2,PURPLE);
    rect(s,64,1048,1792,3,{20,31,47});
    rect(s,64,1048,520,3,CYAN);
}
int main(void) {
    setvbuf(stdout, NULL, _IONBF, 0);
    log_line("INFO","Freely Dev boot sequence started.");

    g_log = fopen("/data/FREELY-DEV.log","a");
    if (g_log) log_line("OK","Opened /data/FREELY-DEV.log.");
    else log_line("WARN","File logging unavailable; screen diagnostics remain active.");

    log_line("INFO","Initializing PS4 video output.");
    Scene2D scene(SCREEN_W,SCREEN_H,4);
    if (!scene.Init(0x02000000,2)) {
        log_line("ERROR","Video output initialization failed.");
        for (;;) sceKernelUsleep(1000000);
    }

    scene.SetActiveFrameBuffer(0);
    draw_terminal(&scene,g_log != NULL);
    scene.SubmitFlip(0);
    scene.FrameWait(0);
    scene.FrameBufferSwap();

    draw_terminal(&scene,g_log != NULL);
    scene.SubmitFlip(1);
    scene.FrameWait(1);
    scene.FrameBufferSwap();

    log_line("OK","Freely Dev screen is active.");
    log_line("INFO","Read-only diagnostics are running.");
    for (;;) sceKernelUsleep(1000000);
    return 0;
}
