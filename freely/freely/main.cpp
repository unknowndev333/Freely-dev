#include <stdio.h>
#include <time.h>
#include <orbis/libkernel.h>

static FILE* g_log = NULL;

static void log_line(const char* level, const char* message)
{
    time_t now = time(NULL);
    const char* stamp = ctime(&now);
    char clean_stamp[32] = "time-unavailable";

    if (stamp != NULL) {
        size_t i = 0;
        while (stamp[i] != '\0' && stamp[i] != '\n' && i < sizeof(clean_stamp) - 1) {
            clean_stamp[i] = stamp[i];
            ++i;
        }
        clean_stamp[i] = '\0';
    }

    printf("[%s] [%s] %s\n", clean_stamp, level, message);
    if (g_log != NULL) {
        fprintf(g_log, "[%s] [%s] %s\n", clean_stamp, level, message);
        fflush(g_log);
    }
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);

    printf("\n");
    printf("███████╗ ██████╗ ███████╗ ███████╗ ██╗      ██╗   ██╗\n");
    printf("██╔════╝ ██╔══██╗██╔════╝ ██╔════╝ ██║      ╚██╗ ██╔╝\n");
    printf("█████╗   ██████╔╝█████╗   █████╗   ██║       ╚████╔╝\n");
    printf("██╔══╝   ██╔══██╗██╔══╝   ██╔══╝   ██║        ╚██╔╝\n");
    printf("██║      ██║  ██║███████╗ ███████╗ ███████╗    ██║\n");
    printf("╚═╝      ╚═╝  ╚═╝╚══════╝ ╚══════╝ ╚══════╝    ╚═╝\n");
    printf("\nFREELY - DEVELOPER TEST\n");
    printf("Read-only diagnostics prototype\n");
    printf("==============================================\n");

    g_log = fopen("/data/FREELY-dev.log", "a");
    if (g_log != NULL) {
        log_line("OK", "Opened /data/FREELY-dev.log for writing.");
    } else {
        log_line("WARN", "Could not open /data/FREELY-dev.log. File logging is unavailable.");
    }

    log_line("INFO", "Diagnostic session started.");
    log_line("INFO", "Platform: PlayStation 4 homebrew target (OpenOrbis build).");
    log_line("N/A", "Firmware version detection is not implemented.");
    log_line("N/A", "GoldHEN version detection is not implemented.");
    log_line("N/A", "CPU and memory telemetry are not implemented.");
    log_line("N/A", "Controller and video-output checks are not implemented.");
    log_line("INFO", "No firmware, kernel, or system settings were modified.");
    log_line("INFO", "Session will remain visible for 30 seconds.");

    sceKernelUsleep(30 * 1000000);

    log_line("INFO", "Diagnostic session finished.");
    if (g_log != NULL) {
        fclose(g_log);
        g_log = NULL;
    }

    return 0;
}
