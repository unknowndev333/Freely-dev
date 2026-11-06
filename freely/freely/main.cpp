#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <orbis/libkernel.h>
#include <sys/utsname.h>
#include <sys/statvfs.h>
#include <unistd.h>
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

struct LogEntry { char level[12]; char message[220]; };
static LogEntry g_entries[38];
static unsigned int g_entry_count = 0;
static Scene2D *g_scene = NULL;
static bool g_screen_ready = false;
static FILE *g_report = NULL;

static RGB level_color(const char *level) {
    if (!strcmp(level,"OK") || !strcmp(level,"FOUND") || !strcmp(level,"DONE")) return GREEN;
    if (!strcmp(level,"WARN") || !strcmp(level,"UNKNOWN")) return YELLOW;
    if (!strcmp(level,"ERROR") || !strcmp(level,"FAIL")) return RED;
    if (!strcmp(level,"CHECK") || !strcmp(level,"INFO") || !strcmp(level,"SCAN")) return CYAN;
    return MUTED;
}

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
    unsigned int slot;
    if (g_entry_count < 38) slot = g_entry_count++;
    else {
        for (int i=0; i<37; ++i) g_entries[i] = g_entries[i+1];
        slot = 37;
    }
    strncpy(g_entries[slot].level, level, sizeof(g_entries[slot].level)-1);
    g_entries[slot].level[sizeof(g_entries[slot].level)-1] = 0;
    strncpy(g_entries[slot].message, message, sizeof(g_entries[slot].message)-1);
    g_entries[slot].message[sizeof(g_entries[slot].message)-1] = 0;
    printf("[%04u] [%s] %s\n", g_log_count, level, message);
    if (g_log) { fprintf(g_log, "[%04u] [%s] %s\n", g_log_count, level, message); fflush(g_log); }
    if (g_report) { fprintf(g_report, "[%04u] [%s] %s\n", g_log_count, level, message); fflush(g_report); }
}
static void draw_terminal(Scene2D *s) {
    rect(s,0,0,SCREEN_W,SCREEN_H,{3,5,9});
    text(s,48,28,"FREELY DEV",4,CYAN);
    text(s,48,78,"PS4 READ-ONLY DIAGNOSTIC LOG",2,DIM);
    rect(s,48,108,1824,2,{25,44,60});
    for (unsigned int i=0; i<g_entry_count; ++i) {
        int y = 132 + (int)i*24;
        char number[16];
        snprintf(number,sizeof(number),"[%04u]",g_log_count-g_entry_count+i+1);
        text(s,52,y,number,2,DIM);
        text(s,132,y,"[",2,DIM);
        text(s,144,y,g_entries[i].level,2,level_color(g_entries[i].level));
        text(s,228,y,"]",2,DIM);
        text(s,248,y,g_entries[i].message,2,WHITE);
    }
}
static void refresh_terminal(void) {
    if (!g_screen_ready || !g_scene) return;
    g_scene->SetActiveFrameBuffer(0);
    draw_terminal(g_scene);
    g_scene->SubmitFlip(0);
    g_scene->FrameWait(0);
    g_scene->FrameBufferSwap();
    g_scene->SetActiveFrameBuffer(1);
    draw_terminal(g_scene);
    g_scene->SubmitFlip(1);
    g_scene->FrameWait(1);
    g_scene->FrameBufferSwap();
}
static void run_read_only_checks(void) {
    struct utsname system_info;
    log_line("CHECK","Querying OS runtime information...");
    refresh_terminal();
    if (uname(&system_info)==0) {
        char line[220];
        snprintf(line,sizeof(line),"OS: %.50s | RELEASE: %.50s",system_info.sysname,system_info.release);
        log_line("FOUND",line);
        snprintf(line,sizeof(line),"MACHINE: %.80s",system_info.machine);
        log_line("FOUND",line);
    } else {
        log_line("WARN","uname() could not provide runtime information.");
    }
    refresh_terminal();

    struct statvfs storage;
    log_line("CHECK","Inspecting writable data filesystem...");
    refresh_terminal();
    if (statvfs("/data",&storage)==0) {
        unsigned long long free_bytes=(unsigned long long)storage.f_bavail*(unsigned long long)storage.f_frsize;
        char line[220];
        snprintf(line,sizeof(line),"/data available space: %llu bytes",free_bytes);
        log_line("FOUND",line);
    } else {
        log_line("WARN","Could not query /data filesystem statistics.");
    }
    refresh_terminal();

    log_line("CHECK","Testing log persistence with write/read verification...");
    refresh_terminal();
    const char *test_path="/data/FREELY-DEV-WRITE-TEST.tmp";
    const char *marker="FREELY_DEV_STORAGE_TEST_OK";
    FILE *test=fopen(test_path,"w");
    if (!test) {
        log_line("FAIL","Could not create a test file in /data.");
    } else {
        fputs(marker,test);
        int write_ok=(fflush(test)==0);
        fclose(test);
        char readback[64]={0};
        test=fopen(test_path,"r");
        if (test) {
            size_t n=fread(readback,1,sizeof(readback)-1,test);
            fclose(test);
            if (write_ok && n==strlen(marker) && strcmp(readback,marker)==0)
                log_line("PASS","/data write and read-back verification succeeded.");
            else
                log_line("FAIL","/data test file did not match written contents.");
        } else {
            log_line("FAIL","Test file was written but could not be reopened.");
        }
        remove(test_path);
    }
    refresh_terminal();
}

int main(void) {
    setvbuf(stdout,NULL,_IONBF,0);
    g_log=fopen("/data/FREELY-DEV.log","a");
    g_report=fopen("/data/FREELY-DEV-REPORT.txt","w");
    log_line("INFO","Freely Dev starting.");
    log_line("INFO","Mode: read-only diagnostics.");
    if (!g_log) log_line("FAIL","Could not open /data/FREELY-DEV.log.");
    else log_line("OK","Raw log opened: /data/FREELY-DEV.log.");
    if (!g_report) log_line("FAIL","Could not open /data/FREELY-DEV-REPORT.txt.");
    else log_line("OK","Report opened: /data/FREELY-DEV-REPORT.txt.");

    Scene2D scene(SCREEN_W,SCREEN_H,4);
    if (!scene.Init(0x02000000,2)) {
        log_line("ERROR","Video output initialization failed.");
        if (g_log) fclose(g_log);
        if (g_report) fclose(g_report);
        for (;;) sceKernelUsleep(1000000);
    }
    g_scene=&scene;
    g_screen_ready=true;
    log_line("OK","Video output initialized.");
    refresh_terminal();

    run_read_only_checks();

    log_line("UNKNOWN","PS4 firmware version query is not implemented.");
    log_line("UNKNOWN","Console model query is not implemented.");
    log_line("UNKNOWN","GoldHEN detection is not implemented.");
    log_line("UNKNOWN","Controller enumeration is not implemented.");
    log_line("DONE","Implemented read-only checks completed.");
    refresh_terminal();

    if (g_log) { fflush(g_log); fclose(g_log); g_log=NULL; }
    if (g_report) { fflush(g_report); fclose(g_report); g_report=NULL; }
    for (;;) sceKernelUsleep(1000000);
    return 0;
}
