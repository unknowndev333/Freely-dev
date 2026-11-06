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
static const RGB BLUE = {110, 168, 255};

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
static void panel(Scene2D *s, int x, int y, int w, int h, const char *heading) {
    rect(s,x,y,w,h,PANEL);
    rect(s,x,y,w,3,PANEL2);
    text(s,x+28,y+24,heading,3,MUTED);
}
static void draw_screen(Scene2D *s, bool log_ok) {
    rect(s,0,0,SCREEN_W,SCREEN_H,BG);
    rect(s,0,0,SCREEN_W,7,GREEN);
    text(s,92,72,"FREELY DEV",8,WHITE);
    text(s,96,154,"PLAYSTATION 4  /  DEVELOPER DIAGNOSTICS",3,MUTED);
    rect(s,1510,82,310,76,PANEL);
    rect(s,1534,108,14,14,GREEN);
    text(s,1564,103,"RUNNING",3,GREEN);
    rect(s,92,220,1736,2,PANEL2);

    panel(s,92,270,540,190,"SYSTEM");
    text(s,124,340,"PLATFORM",3,MUTED);
    text(s,124,385,"PLAYSTATION 4",4,WHITE);
    text(s,124,430,"RUNTIME ACTIVE",3,GREEN);

    panel(s,660,270,540,190,"SECURITY");
    text(s,692,340,"GOLDHEN",3,MUTED);
    text(s,692,385,"NOT DETECTED",4,YELLOW);
    text(s,692,430,"NO PATCHES APPLIED",3,GREEN);

    panel(s,1228,270,600,190,"FIRMWARE");
    text(s,1260,340,"FIRMWARE VERSION",3,MUTED);
    text(s,1260,385,"NOT AVAILABLE",4,YELLOW);
    text(s,1260,430,"READ ONLY",3,GREEN);

    panel(s,92,490,840,360,"DIAGNOSTIC CHECKS");
    text(s,128,560,"VIDEO OUTPUT",3,WHITE);
    text(s,720,560,"INITIALIZED",3,GREEN);
    rect(s,128,602,768,2,PANEL2);
    text(s,128,630,"LOCAL LOG FILE",3,WHITE);
    text(s,720,630,log_ok ? "WRITABLE" : "UNAVAILABLE",3,log_ok?GREEN:YELLOW);
    rect(s,128,672,768,2,PANEL2);
    text(s,128,700,"CPU / MEMORY TELEMETRY",3,WHITE);
    text(s,720,700,"NOT IMPLEMENTED",3,YELLOW);
    rect(s,128,742,768,2,PANEL2);
    text(s,128,770,"CONTROLLER STATUS",3,WHITE);
    text(s,720,770,"NOT IMPLEMENTED",3,YELLOW);

    panel(s,960,490,868,360,"SESSION LOG");
    text(s,996,560,"[OK] VIDEO OUTPUT INITIALIZED",3,GREEN);
    text(s,996,615,log_ok?"[OK] LOG FILE OPENED":"[WARN] FILE LOGGING OFF",3,log_ok?GREEN:YELLOW);
    text(s,996,670,"[INFO] DIAGNOSTIC SESSION STARTED",3,WHITE);
    text(s,996,725,"[N/A] FIRMWARE / GOLDHEN API",3,MUTED);
    text(s,996,780,"[INFO] READ ONLY - NO SYSTEM CHANGES",3,WHITE);

    rect(s,92,900,1736,2,PANEL2);
    text(s,96,944,"FREELY DEV  /  BUILD 0.2.0",3,MUTED);
    text(s,96,1000,"DIAGNOSTIC MODE ACTIVE",3,GREEN);
    text(s,1450,1000,"SAFE  -  READ ONLY",3,MUTED);
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
    draw_screen(&scene,g_log != NULL);
    scene.SubmitFlip(0);
    scene.FrameWait(0);
    scene.FrameBufferSwap();

    draw_screen(&scene,g_log != NULL);
    scene.SubmitFlip(1);
    scene.FrameWait(1);
    scene.FrameBufferSwap();

    log_line("OK","Freely Dev screen is active.");
    log_line("INFO","Read-only diagnostics are running.");
    for (;;) sceKernelUsleep(1000000);
    return 0;
}
