#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <orbis/libkernel.h>
#include "graphics.h"

static const int SCREEN_W = 1920;
static const int SCREEN_H = 1080;
static FILE *g_log = NULL;
static FILE *g_report = NULL;
static unsigned int g_log_count = 0;
struct RGB { uint8_t r, g, b; };
static const RGB WHITE = {239,243,250};
static const RGB CYAN = {70,235,245};
static const RGB GREEN = {102,235,176};
static const RGB YELLOW = {255,204,102};
static const RGB RED = {255,105,125};
static const RGB DIM = {65,82,105};
struct LogEntry { char level[12]; char message[220]; uint64_t elapsed; };
static LogEntry g_entries[38];
static unsigned int g_entry_count = 0;
static Scene2D *g_scene = NULL;
static bool g_screen_ready = false;

static RGB level_color(const char *level) {
    if (!strcmp(level,"OK") || !strcmp(level,"FOUND") || !strcmp(level,"PASS") || !strcmp(level,"DONE")) return GREEN;
    if (!strcmp(level,"WARN") || !strcmp(level,"UNKNOWN")) return YELLOW;
    if (!strcmp(level,"ERROR") || !strcmp(level,"FAIL")) return RED;
    if (!strcmp(level,"CHECK") || !strcmp(level,"INFO") || !strcmp(level,"SCAN")) return CYAN;
    return WHITE;
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
static void rect(Scene2D *s,int x,int y,int w,int h,RGB c) {
    s->DrawRectangle(x,y,w,h,Color{c.r,c.g,c.b});
}
static void text(Scene2D *s,int x,int y,const char *value,int scale,RGB color) {
    for (size_t n=0; value && value[n]; ++n) {
        char ch=(char)toupper((unsigned char)value[n]);
        if (ch==' ') { x+=4*scale; continue; }
        const char *found=strchr(FONT_CHARS,ch);
        if (!found) { x+=6*scale; continue; }
        const uint8_t *glyph=FONT[(size_t)(found-FONT_CHARS)];
        for (int col=0; col<5; ++col)
            for (int row=0; row<7; ++row)
                if (glyph[col]&(1u<<row)) rect(s,x+col*scale,y+row*scale,scale,scale,color);
        x+=6*scale;
        if (x>SCREEN_W-40) break;
    }
}
static void draw_terminal(Scene2D *s) {
    rect(s,0,0,SCREEN_W,SCREEN_H,{3,5,9});
    text(s,48,28,"FREELY DEV",4,CYAN);
    for (unsigned int i=0; i<g_entry_count; ++i) {
        int y=92+(int)i*25;
        char number[24], elapsed[24];
        snprintf(number,sizeof(number),"[%04u]",g_log_count-g_entry_count+i+1);
        snprintf(elapsed,sizeof(elapsed),"+%08llu",(unsigned long long)(g_entries[i].elapsed/1000));
        text(s,48,y,number,2,DIM);
        text(s,120,y,elapsed,2,DIM);
        text(s,236,y,"[",2,DIM);
        text(s,248,y,g_entries[i].level,2,level_color(g_entries[i].level));
        text(s,330,y,"]",2,DIM);
        text(s,350,y,g_entries[i].message,2,WHITE);
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
static void log_line(const char *level,const char *message) {
    ++g_log_count;
    uint64_t elapsed=sceKernelGetProcessTime();
    unsigned int slot;
    if (g_entry_count<38) slot=g_entry_count++;
    else {
        for (int i=0;i<37;++i) g_entries[i]=g_entries[i+1];
        slot=37;
    }
    strncpy(g_entries[slot].level,level,sizeof(g_entries[slot].level)-1);
    g_entries[slot].level[sizeof(g_entries[slot].level)-1]=0;
    strncpy(g_entries[slot].message,message,sizeof(g_entries[slot].message)-1);
    g_entries[slot].message[sizeof(g_entries[slot].message)-1]=0;
    g_entries[slot].elapsed=elapsed;
    printf("[%04u] [+%llu us] [%s] %s\n",g_log_count,(unsigned long long)elapsed,level,message);
    if (g_log) { fprintf(g_log,"[%04u] [+%llu us] [%s] %s\n",g_log_count,(unsigned long long)elapsed,level,message); fflush(g_log); }
    if (g_report) { fprintf(g_report,"[%04u] [+%llu us] [%s] %s\n",g_log_count,(unsigned long long)elapsed,level,message); fflush(g_report); }
}
static void report_bool(const char *label,int result,const char *yes,const char *no) {
    char line[220];
    if (result==0 || result==1) {
        snprintf(line,sizeof(line),"%s: %s (raw=%d)",label,result?yes:no,result);
        log_line("FOUND",line);
    } else {
        snprintf(line,sizeof(line),"%s query returned %d",label,result);
        log_line("WARN",line);
    }
}
static void run_read_only_checks(void) {
    char line[220];
    log_line("SCAN","Starting full supported read-only scan.");
    refresh_terminal();

    log_line("CHECK","Reading PS4 system software version...");
    OrbisKernelSwVersion sw;
    memset(&sw,0,sizeof(sw));
    int sw_result=sceKernelGetSystemSwVersion(&sw);
    if (sw_result==0 && sw.Version!=0) {
        snprintf(line,sizeof(line),"Firmware: %s | raw=0x%08X",sw.VersionString,sw.Version);
        log_line("FOUND",line);
    } else {
        snprintf(line,sizeof(line),"Firmware query failed: result=%d",sw_result);
        log_line("UNKNOWN",line);
    }
    refresh_terminal();

    log_line("CHECK","Reading CPU frequency...");
    int cpu_frequency=sceKernelGetCpuFrequency();
    if (cpu_frequency>0) {
        snprintf(line,sizeof(line),"CPU frequency API value: %d (SDK raw units)",cpu_frequency);
        log_line("FOUND",line);
    } else {
        snprintf(line,sizeof(line),"CPU frequency query returned %d",cpu_frequency);
        log_line("WARN",line);
    }
    refresh_terminal();

    log_line("CHECK","Reading direct-memory capacity...");
    size_t direct_memory=sceKernelGetDirectMemorySize();
    if (direct_memory>0) {
        snprintf(line,sizeof(line),"Direct-memory capacity: %llu bytes",(unsigned long long)direct_memory);
        log_line("FOUND",line);
    } else log_line("WARN","Direct-memory capacity returned zero.");
    refresh_terminal();

    log_line("CHECK","Reading available flexible memory...");
    size_t flexible_memory=0;
    int flexible_result=sceKernelAvailableFlexibleMemorySize(&flexible_memory);
    if (flexible_result==0) {
        snprintf(line,sizeof(line),"Available flexible memory: %llu bytes",(unsigned long long)flexible_memory);
        log_line("FOUND",line);
    } else {
        snprintf(line,sizeof(line),"Flexible-memory query failed: result=%d",flexible_result);
        log_line("WARN",line);
    }
    refresh_terminal();

    log_line("CHECK","Reading console platform flags...");
    report_bool("CEX retail flag",sceKernelIsCEX(),"YES","NO");
    report_bool("DevKit flag",sceKernelIsDevKit(),"YES","NO");
    report_bool("TestKit flag",sceKernelIsTestKit(),"YES","NO");
    report_bool("Neo hardware flag",sceKernelIsAuthenticNeo(),"YES","NO");
    report_bool("Neo mode flag",sceKernelIsNeoMode(),"YES","NO");
    report_bool("Sandbox flag",sceKernelIsInSandbox(),"YES","NO");
    refresh_terminal();

    log_line("CHECK","Reading process and CPU runtime counters...");
    uint64_t process_time=sceKernelGetProcessTime();
    uint64_t counter=sceKernelGetProcessTimeCounter();
    uint64_t counter_frequency=sceKernelGetProcessTimeCounterFrequency();
    snprintf(line,sizeof(line),"Process time: %llu us",(unsigned long long)process_time);
    log_line("FOUND",line);
    snprintf(line,sizeof(line),"Process time counter: %llu",(unsigned long long)counter);
    log_line("FOUND",line);
    snprintf(line,sizeof(line),"Counter frequency: %llu",(unsigned long long)counter_frequency);
    if (counter_frequency) log_line("FOUND",line); else log_line("WARN","Counter frequency returned zero.");
    snprintf(line,sizeof(line),"Current process ID: %d",(int)getpid());
    log_line("FOUND",line);
    refresh_terminal();

    log_line("CHECK","Verifying /data log persistence...");
    const char *test_path="/data/FREELY-DEV-WRITE-TEST.tmp";
    const char *marker="FREELY_DEV_STORAGE_TEST_OK";
    FILE *test=fopen(test_path,"w");
    if (!test) log_line("FAIL","Could not create /data test file.");
    else {
        fputs(marker,test);
        int write_ok=fflush(test)==0;
        fclose(test);
        char readback[64]={0};
        test=fopen(test_path,"r");
        if (test) {
            size_t n=fread(readback,1,sizeof(readback)-1,test);
            fclose(test);
            if (write_ok && n==strlen(marker) && strcmp(readback,marker)==0)
                log_line("PASS","/data write and read-back verification succeeded.");
            else log_line("FAIL","/data read-back did not match written data.");
        } else log_line("FAIL","Test file could not be reopened.");
        remove(test_path);
    }
    refresh_terminal();

    log_line("UNKNOWN","Exact console model string is not exposed by a verified API in this build.");
    log_line("UNKNOWN","Controller enumeration requires the separate Pad library and is not linked yet.");
    log_line("UNKNOWN","GoldHEN state is not inferred from guessed files or identifiers.");
    log_line("UNKNOWN","GPU temperature, fan speed and physical RAM usage are not queried by this safe API set.");
    log_line("DONE","Full supported read-only scan completed.");
    refresh_terminal();
}
int main(void) {
    setvbuf(stdout,NULL,_IONBF,0);
    g_log=fopen("/data/FREELY-DEV.log","a");
    g_report=fopen("/data/FREELY-DEV-REPORT.txt","w");
    log_line("INFO","Freely Dev booting.");
    log_line("INFO","Mode: read-only diagnostics; no system modifications.");
    if (g_log) log_line("OK","Raw log opened: /data/FREELY-DEV.log.");
    else log_line("FAIL","Could not open /data/FREELY-DEV.log.");
    if (g_report) log_line("OK","Report opened: /data/FREELY-DEV-REPORT.txt.");
    else log_line("FAIL","Could not open /data/FREELY-DEV-REPORT.txt.");

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

    if (g_log) { fflush(g_log); fclose(g_log); g_log=NULL; }
    if (g_report) { fflush(g_report); fclose(g_report); g_report=NULL; }
    for (;;) sceKernelUsleep(1000000);
    return 0;
}
