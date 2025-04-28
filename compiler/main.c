#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define OUTPUT "src/program.h"

// Write a single byte (and format nicely every 12 bytes)
static void write_byte(FILE *out, uint8_t byte, int *col) {
    fprintf(out, "0x%02X,", byte);
    (*col)++;
    if ((*col % 12) == 0) {
        fprintf(out, "\n");
    }
}

int main(void) {
    FILE *in = fopen("compiler/input.txt", "r");
    if (!in) {
        perror("Failed to open compiler/input.txt");
        return 1;
    }

    FILE *out = fopen(OUTPUT, "w");
    if (!out) {
        perror("Failed to open " OUTPUT);
        fclose(in);
        return 1;
    }

    fprintf(out, "const unsigned char program[] = {\n");

    char line[256];
    int col = 0;
    int in_irq = 0;
    int in_loop = 0;

    while (fgets(line, sizeof(line), in)) {
        // --- Trim leading whitespace ---
        char *s = line;
        while (*s == ' ' || *s == '\t') s++;

        // --- Skip blank lines and comments ---
        if (*s == '\0' || *s == '\r' || *s == '\n' ||
            *s == '#' || (s[0] == '/' && s[1] == '/')) {
            continue;
        }

        // --- Parse each command ---
        if (strncmp(s, "set ", 4) == 0) {
            char port; int pin, val;
            sscanf(s, "set P%c%d %d", &port, &pin, &val);
            write_byte(out, 0x01, &col);
            write_byte(out, pin, &col);
            write_byte(out, val ? 1 : 0, &col);
        }
        else if (strncmp(s, "wait ", 5) == 0) {
            int ms;
            sscanf(s, "wait %d", &ms);
            write_byte(out, 0x02, &col);
            write_byte(out, (ms >> 8) & 0xFF, &col);
            write_byte(out, ms & 0xFF, &col);
        }
        else if (strncmp(s, "toggle ", 7) == 0) {
            char port; int pin;
            sscanf(s, "toggle P%c%d", &port, &pin);
            write_byte(out, 0x03, &col);
            write_byte(out, pin, &col);
        }
        else if (strncmp(s, "if ", 3) == 0) {
            char pin[8];
            int value;
            if (sscanf(s + 3, "%s == %d then", pin, &value) == 2) {
                if (strcmp(pin, "PC13") == 0) {
                    write_byte(out, 0x04, &col);
                    write_byte(out, 13, &col);
                    write_byte(out, value, &col);
                } else {
                    fprintf(stderr, "[ERROR] Unsupported pin in if: %s\n", pin);
                }
            } else {
                fprintf(stderr, "[ERROR] Invalid 'if' syntax: %s\n", s);
            }
        }
        else if (strncmp(s, "else", 4) == 0) {
            write_byte(out, 0x0C, &col);
        }
        else if (strncmp(s, "print ", 6) == 0) {
            char *msg = s + 6;
            size_t len = strlen(msg);
            if (len > 0 && msg[len - 1] == '\n') msg[--len] = '\0';
            write_byte(out, 0x05, &col);
            write_byte(out, (uint8_t)len, &col);
            for (size_t i = 0; i < len; i++) {
                write_byte(out, (uint8_t)msg[i], &col);
            }
        }
        else if (strncmp(s, "printNum ", 9) == 0) {
            int num;
            sscanf(s, "printNum %d", &num);
            char buf[16];
            int len = snprintf(buf, sizeof(buf), "%d", num);
            write_byte(out, 0x0A, &col);
            write_byte(out, (uint8_t)len, &col);
            for (int i = 0; i < len; i++) {
                write_byte(out, (uint8_t)buf[i], &col);
            }
        }
        else if (strncmp(s, "printCount", 10) == 0) {
            write_byte(out, 0x0B, &col);
        }
        else if (strncmp(s, "loop ", 5) == 0) {
            int count;
            sscanf(s, "loop %d", &count);
            write_byte(out, 0x08, &col);
            write_byte(out, (uint8_t)count, &col);
            in_loop = 1;
        }
        else if (strncmp(s, "on PC13 falling then", 20) == 0 ||
                 strncmp(s, "on PC13 rising then", 19) == 0) {
            write_byte(out, 0x06, &col);
            in_irq = 1;
        }
        else if (strncmp(s, "end", 3) == 0) {
            if (in_irq) {
                write_byte(out, 0x07, &col);
                in_irq = 0;
                break; // Interrupt block ends program
            }
            else if (in_loop) {
                write_byte(out, 0x09, &col);
                in_loop = 0;
            }
            else {
                write_byte(out, 0x07, &col); // End of normal block
            }
        }
        else {
            fprintf(stderr, "[ERROR] Unknown command: %s", s);
        }
    }

    fprintf(out, "\n};\n");
    fclose(in);
    fclose(out);

    printf("[✅] Compiled script successfully to %s\n", OUTPUT);
    return 0;
}
