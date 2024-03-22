#include <stdio.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }
    FILE *file = fopen(argv[1], "rb");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }
    int byte;
    while ((byte = fgetc(file)) != EOF) {
        printf("%02X ", byte);
    }
    printf("\n");
    fclose(file);

}