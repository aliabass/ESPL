#include <stdio.h>
#include <stdbool.h>


void toggleDebugMode();
void setFileName();
void setUnitSize();
void loadIntoMemory();
void toggleDisplayMode();
void memoryDisplay();
void saveIntoFile();
void memoryModify();
void quit();


static char* hex_formats[] = {"%#hhx\n", "%#hx\n", "No such unit", "%#x\n"};
static char* dec_formats[] = {"%#hhd\n", "%#hd\n", "No such unit", "%#d\n"};

typedef struct {
  char debug_mode;
  char file_name[128]; //hexeditplus
  int unit_size; 
  unsigned char mem_buf[10000];
  size_t mem_count;
  char display_flag;
  /*
   .
   .
   Any additional fields you deem necessary
  */
} state;

struct Option {
    const char *name;
    void (*function)();
};

struct Option options[] = {
    {"Toggle Debug Mode", toggleDebugMode},
    {"Set File Name", setFileName},
    {"Set Unit Size", setUnitSize},
    {"Load Into Memory", loadIntoMemory},
    {"Toggle Display Mode", toggleDisplayMode},
    {"Memory Display", memoryDisplay},
    {"Save Into File", saveIntoFile},
    {"Memory Modify", memoryModify},
    {"Quit", quit},
    {NULL , NULL}
};

int main() {
    int choice;
    state s;
    s.debug_mode = 0;
    s.unit_size = 1;
    s.mem_count = 0;
    s.display_flag = 0;
    while (true) {
         if (s.debug_mode) {
            fprintf(stderr,"unit_size: %d\n", s.unit_size);
            fprintf(stderr,"file_name: %s\n", s.file_name);
            fprintf(stderr,"mem_count: %zu\n", s.mem_count);
        }
        printf("Choose action:\n");
        for (int i = 0; i < sizeof(options) / sizeof(options[0]) - 1; i++) {
            printf("%d-%s\n", i, options[i].name);
        }

        scanf("%d", &choice);

        if (choice >= 0 && choice < sizeof(options) / sizeof(options[0])) {
            options[choice].function(&s); // Call the function corresponding to the user's choice
        } else {
            fprintf(stderr,"Invalid choice. Please choose a valid action.\n");
        }
    }

    return 0;
}


void toggleDebugMode(state* s){
    s->debug_mode = !s->debug_mode;
    if (s->debug_mode) {
        printf("Debug flag now on\n");
    } else {
        printf("Debug flag now off\n");
    }
}
void setFileName(state* s){
    printf("Enter file name: ");
    scanf("%s", s->file_name);
    if(s->debug_mode) fprintf(stderr, "Debug: File name set to: %s\n", s->file_name);
}

void setUnitSize(state* s){
    int size;
    printf("Enter unit size (1, 2, or 4): ");
    scanf("%d", &size);
    if (size == 1 || size == 2 || size == 4) {
        s->unit_size = size;
        if (s->debug_mode) {
            fprintf(stderr, "Debug: set size to %d\n", size);
        }
    } else {
        fprintf(stderr, "Invalid unit size. Please enter 1, 2, or 4.\n");
    }
}
void loadIntoMemory(state* s){
    if (strcmp(s->file_name, "") == 0) {
        fprintf(stderr,"Error: File name is empty.\n");
        return;
    }
    FILE *file = fopen(s->file_name, "rb");
    if (file == NULL) {
        fprintf(stderr,"Error: Failed to open file %s\n", s->file_name);
        return;
    }
    unsigned int location;
    size_t length;
    printf("Please enter <location> <length>");
    scanf("%x %zu", &location , &length);
    if (s->debug_mode) {
        fprintf(stderr, "Debug: File Name: %s, Location: 0x%x, Length: %zu\n", s->file_name,location, length);
    }
    size_t bytes_to_read = length * s->unit_size;
    fseek(file, location, SEEK_SET);
    size_t bytes_read = fread(s->mem_buf, s->unit_size, length, file);
    fclose(file);
    if (bytes_read != length) {
        fprintf(stderr, "Error: Failed to read %zu bytes from file.%zu\n", bytes_to_read,bytes_read);
        return;
    }ftell(file);
    s->mem_count=length*s->unit_size;
}

void toggleDisplayMode(state* s){
    s->display_flag = !s->display_flag;

    if (s->display_flag) {
        printf("Display flag now on, hexadecimal representation\n");
    } else {
        printf("Display flag now off, decimal representation\n");
    }
}


void memoryDisplay(state* s){
    size_t u;
    unsigned int addr;
    printf("Enter address and length\n");
    scanf("%x %zu", &addr ,&u);
    if (addr == 0) {
        addr = (unsigned int)s->mem_buf;
    }
    if (s->display_flag) {
        printf("Hexadecimal\n===========\n");
        for (size_t i = 0; i < u; i++) {
            printf(hex_formats[s->unit_size - 1] ,*((unsigned int *)(addr + i * s->unit_size)));
        }
    } else {
        printf("Decimal\n=======\n");
        for (size_t i = 0; i < u; i++) {
            printf(dec_formats[s->unit_size - 1], *((unsigned int *)(addr + i * s->unit_size)));
        }
    }
}


void saveIntoFile(state* s){
    if (strcmp(s->file_name, "") == 0) {
        fprintf(stderr,"Error: File name is empty.\n");
        return;
    }
    FILE *file = fopen(s->file_name, "r+b");
    if (file == NULL) {
        fprintf(stderr,"Error: Failed to open file %s for writing.\n", s->file_name);
        return;
    }
    unsigned int source_address;
    unsigned int target_location;
    size_t length;

    printf("Please enter <source-address> <target-location> <length>: ");
    scanf("%x %x %zu", &source_address, &target_location, &length);

    if (s->debug_mode) {
        fprintf(stderr, "Debug: Source Address: 0x%x, Target Location: 0x%x, Length: %zu\n", source_address, target_location, length);
    }
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    if (target_location >= file_size) {
        fprintf(stderr, "Error: Target location is beyond the file size.\n");
        fclose(file);
        return;
    }
    fseek(file, target_location, SEEK_SET);
    fwrite(s->mem_buf + source_address, s->unit_size, length, file);
    fclose(file);
    printf("Data saved into file.\n");
}


void memoryModify(state* s){
    unsigned int location;
    unsigned int val;

    printf("Please enter <location> <val>: ");
    scanf("%x %x", &location, &val);

    if (s->debug_mode) {
        fprintf(stderr, "Debug: Location: 0x%x, Val: 0x%x\n", location, val);
    }
    if (location >= 0 && location < sizeof(s->mem_buf)) {
        *(unsigned int *)(s->mem_buf + location) = val;
        printf("Memory modified at location 0x%x with value 0x%x\n", location, val);
    } else {
        fprintf(stderr,"Error: Invalid location specified.\n");
    }
}

void quit(state* s){
    if (s->debug_mode) {
        fprintf(stderr, "quitting\n");
    }
    exit(0);
}