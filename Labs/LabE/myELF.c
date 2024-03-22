#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <elf.h>
#include <sys/stat.h>

// Structure for menu options
struct menu_option {
    int number;
    char description[256];
};

// Structure to store symbol information
struct symbol_info {
    int index;
    Elf32_Addr value;
    int section_index;
    char section_name[256];
    char symbol_name[256];
};

// Function prototypes
void toggleDebugMode(int *debugMode);
void examineELFFile();
void printSectionNames();
void printSymbols();
void checkFilesForMerge();
void mergeELFFiles();
void quitProgram();

// Global variables
int debugMode = 0;
int fd1 = -1;               // File descriptor for the first ELF file
int fd2 = -1;               // File descriptor for the second ELF file
void *mapped_file1 = NULL;  // Mapped file for the first ELF file
void *mapped_file2 = NULL;  // Mapped file for the second ELF file
off_t file_size1;           // Size of the first ELF file
off_t file_size2;           // Size of the second ELF file
char file_name1[256];       // Name of the first ELF file
char file_name2[256];       // Name of the second ELF file
char ex[256];
int examine_count = 0;      // Count of examine calls

// Array of menu options
struct menu_option menu[] = {
    {0, "Toggle Debug Mode"},
    {1, "Examine ELF File"},
    {2, "Print Section Names"},
    {3, "Print Symbols"},
    {4, "Check Files for Merge"},
    {5, "Merge ELF Files"},
    {6, "Quit"}
};

// Main function
int main() {
    int option;
    
    while (1) {
        printf("Choose action:\n");
        for (int i = 0; i < sizeof(menu) / sizeof(menu[0]); i++) {
            printf("%d-%s\n", menu[i].number, menu[i].description);
        }
        printf("Option: ");
        scanf("%d", &option);
        getchar();  // Consume newline character

        switch (option) {
            case 0:
                toggleDebugMode(&debugMode);
                break;
            case 1:
                if (examine_count >= 2) {
                    printf("Error: Already examined two ELF files. Cannot examine more.\n");
                } else {
                    examineELFFile();
                }
                break;
            case 2:
                printSectionNames();
                break;
            case 3:
                printSymbols();
                break;
            case 4:
                checkFilesForMerge();
                break;
            case 5:
                mergeELFFiles();
                break;
            case 6:
                quitProgram();
                return 0;
            default:
                printf("Invalid option. Please choose a valid option.\n");
        }

        if (debugMode) {
            printf("Debug mode is ON\n");
            printf("fd1: %d, fd2: %d\n", fd1, fd2);
            printf("mapped_file1: %p, mapped_file2: %p\n", mapped_file1, mapped_file2);
            printf("file_size1: %ld, file_size2: %ld\n", file_size1, file_size2);
            printf("file_name1: %s, file_name2: %s\n", file_name1, file_name2);
        }
        printf("\n");
    }
}

// Function to toggle debug mode
void toggleDebugMode(int *debugMode) {
    *debugMode = !(*debugMode);
    printf("Debug mode %s\n", *debugMode ? "ON" : "OFF");
}

// Function to examine ELF file
void examineELFFile() {
    if (examine_count >= 2) {
        printf("Error: Already examined two ELF files. Cannot examine more.\n");
        return;
    }

    printf("Enter the name of the ELF file: ");
    fgets(ex, sizeof(ex), stdin);
    ex[strcspn(ex, "\n")] = '\0';  // Remove trailing newline character

    // Open the file for reading
    int fd = open(ex, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return;
    }
    // Map the entire file into memory
    struct stat sb;
    if (fstat(fd, &sb) < 0) {
        perror("fstat");
        close(fd);
        return;
    }
    if (examine_count == 0) {
        strcpy(file_name1, ex);
        mapped_file1 = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (mapped_file1 == MAP_FAILED) {
            perror("mmap");
            close(fd);
            return;
        }
        file_size1 = sb.st_size;
        fd1 = fd;
    } else {
        strcpy(file_name2, ex);
        mapped_file2 = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (mapped_file2 == MAP_FAILED) {
            perror("mmap");
            close(fd);
            return;
        }
        file_size2 = sb.st_size;
        fd2 = fd;
    }

    // Examine the ELF file header
    Elf32_Ehdr *elf_header = (Elf32_Ehdr *)mapped_file1;

    // Check if the magic number is consistent with an ELF file
    if (memcmp(elf_header->e_ident, ELFMAG, SELFMAG) != 0) {
        printf("Error: Not a valid ELF file.\n");
        munmap(mapped_file1, sb.st_size);
        close(fd);
        mapped_file1 = NULL;
        fd1 = -1;
        return;
    }

    // Print information from the ELF header
    printf("Bytes 1, 2, 3 of the magic number (in ASCII): %c%c%c\n",
           elf_header->e_ident[EI_MAG1], elf_header->e_ident[EI_MAG2], elf_header->e_ident[EI_MAG3]);
    printf("Data encoding scheme of the object file: %s\n",
           (elf_header->e_ident[EI_DATA] == ELFDATA2LSB) ? "Little Endian" : "Big Endian");
    printf("Entry point (hexadecimal address): 0x%x\n", elf_header->e_entry);
    printf("File offset of the section header table: 0x%x\n", elf_header->e_shoff);
    printf("Number of section header entries: %d\n", elf_header->e_shnum);
    printf("Size of each section header entry: %d bytes\n", elf_header->e_shentsize);
    printf("File offset of the program header table: 0x%x\n", elf_header->e_phoff);
    printf("Number of program header entries: %d\n", elf_header->e_phnum);
    printf("Size of each program header entry: %d bytes\n", elf_header->e_phentsize);

    examine_count++;
}

// Function to print section names
void printSectionNames() {
     if (fd1 == -1 && fd2 == -1) {
        printf("Error: no ELF files have been opened.\n");
        return;
    }

    // Print section names for the first ELF file
    if(fd1 != -1){
    Elf32_Ehdr *elf_header1 = (Elf32_Ehdr *)mapped_file1;
    Elf32_Shdr *section_header_table1 = (Elf32_Shdr *)((uintptr_t)mapped_file1 + elf_header1->e_shoff);
    char *string_table1 = (char *)((uintptr_t)mapped_file1 + section_header_table1[elf_header1->e_shstrndx].sh_offset);

    printf("File %s\n", file_name1);
    for (int i = 0; i < elf_header1->e_shnum; i++) {
        printf("[%d] %s 0x%x 0x%x 0x%x %d\n", i, &string_table1[section_header_table1[i].sh_name],
               section_header_table1[i].sh_addr, section_header_table1[i].sh_offset,
               section_header_table1[i].sh_size, section_header_table1[i].sh_type);
    }
    printf("---------------\n");
    }
    if(fd2 != -1){
    // Print section names for the second ELF file
    Elf32_Ehdr *elf_header2 = (Elf32_Ehdr *)mapped_file2;
    Elf32_Shdr *section_header_table2 = (Elf32_Shdr *)((uintptr_t)mapped_file2 + elf_header2->e_shoff);
    char *string_table2 = (char *)((uintptr_t)mapped_file2 + section_header_table2[elf_header2->e_shstrndx].sh_offset);

    printf("File %s\n", file_name2);
    for (int i = 0; i < elf_header2->e_shnum; i++) {
        printf("[%d] %s 0x%x 0x%x 0x%x %d\n", i, &string_table2[section_header_table2[i].sh_name],
               section_header_table2[i].sh_addr, section_header_table2[i].sh_offset,
               section_header_table2[i].sh_size, section_header_table2[i].sh_type);
    }
    printf("---------------\n");
    }
}

// Function to print symbols
void printSymbols() {
    if (fd1 == -1) {
        printf("Error: No ELF file has been examined yet.\n");
        return;
    }
    Elf32_Ehdr *elf_header = (Elf32_Ehdr *)mapped_file1;
    Elf32_Shdr *section_header_table = (Elf32_Shdr *)((uintptr_t)mapped_file1 + elf_header->e_shoff);
    char *string_table = (char *)((uintptr_t)mapped_file1 + section_header_table[elf_header->e_shstrndx].sh_offset);

    printf("Printing symbols...\n");
    printf("File %s\n", file_name1);

    // Iterate through the section header table to find the symbol table
    for (int i = 0; i < elf_header->e_shnum; i++) {
        if (section_header_table[i].sh_type == SHT_SYMTAB) {
            Elf32_Sym *symbol_table = (Elf32_Sym *)((uintptr_t)mapped_file1 + section_header_table[i].sh_offset);
            int symbol_count = section_header_table[i].sh_size / sizeof(Elf32_Sym);

            // Iterate through the symbol table
            for (int j = 0; j < symbol_count; j++) {
                printf("[%d] 0x%x %d %s %s\n", j, symbol_table[j].st_value, section_header_table[i].sh_info,
                       &string_table[symbol_table[j].st_name], &string_table[section_header_table[section_header_table[i].sh_link].sh_name]);
            }
        }
    }
    if(fd2 != -1){
    Elf32_Ehdr *elf_header = (Elf32_Ehdr *)mapped_file2;
    Elf32_Shdr *section_header_table = (Elf32_Shdr *)((uintptr_t)mapped_file2 + elf_header->e_shoff);
    char *string_table = (char *)((uintptr_t)mapped_file2 + section_header_table[elf_header->e_shstrndx].sh_offset);

    printf("Printing symbols...\n");
    printf("File %s\n", file_name2);

    // Iterate through the section header table to find the symbol table
    for (int i = 0; i < elf_header->e_shnum; i++) {
        if (section_header_table[i].sh_type == SHT_SYMTAB) {
            Elf32_Sym *symbol_table = (Elf32_Sym *)((uintptr_t)mapped_file2 + section_header_table[i].sh_offset);
            int symbol_count = section_header_table[i].sh_size / sizeof(Elf32_Sym);

            // Iterate through the symbol table
            for (int j = 0; j < symbol_count; j++) {
                printf("[%d] 0x%x %d %s %s\n", j, symbol_table[j].st_value, section_header_table[i].sh_info,
                       &string_table[symbol_table[j].st_name], &string_table[section_header_table[section_header_table[i].sh_link].sh_name]);
                }
            }
        }
    }
    printf("Symbols printed.\n");
}
// Stub function for checking files for merge
void checkFilesForMerge() {
      if (fd1 == -1 || fd2 == -1 || mapped_file1 == NULL || mapped_file2 == NULL) {
        printf("Error: Two ELF files must be opened and mapped for checking merge.\n");
        return;
    }

    // Assume each ELF file contains exactly one symbol table
    Elf32_Ehdr *elf_header1 = (Elf32_Ehdr *)mapped_file1;
    Elf32_Ehdr *elf_header2 = (Elf32_Ehdr *)mapped_file2;
    Elf32_Shdr *section_header_table1 = (Elf32_Shdr *)((uintptr_t)mapped_file1 + elf_header1->e_shoff);
    Elf32_Shdr *section_header_table2 = (Elf32_Shdr *)((uintptr_t)mapped_file2 + elf_header2->e_shoff);
    int symtab_count1 = 0;
    int symtab_count2 = 0;

    // Count symbol tables in each ELF file
    for (int i = 0; i < elf_header1->e_shnum; i++) {
        if (section_header_table1[i].sh_type == SHT_SYMTAB) {
            symtab_count1++;
        }
    }

    for (int i = 0; i < elf_header2->e_shnum; i++) {
        if (section_header_table2[i].sh_type == SHT_SYMTAB) {
            symtab_count2++;
        }
    }

    if (symtab_count1 != 1 || symtab_count2 != 1) {
        printf("Error: Each ELF file must contain exactly one symbol table.\n");
        return;
    }

    // Locate symbol tables
    Elf32_Sym *symbol_table1 = NULL;
    Elf32_Sym *symbol_table2 = NULL;
    Elf32_Shdr *symtab1 = NULL;
    Elf32_Shdr *symtab2 = NULL;


    for (int i = 0; i < elf_header1->e_shnum; i++) {
        if (section_header_table1[i].sh_type == SHT_SYMTAB) {
            symbol_table1 = (Elf32_Sym *)((uintptr_t)mapped_file1 + section_header_table1[i].sh_offset);
            symtab1 = &section_header_table1[i];
            break;
        }
    }

    for (int i = 0; i < elf_header2->e_shnum; i++) {
        if (section_header_table2[i].sh_type == SHT_SYMTAB) {
            symbol_table2 = (Elf32_Sym *)((uintptr_t)mapped_file2 + section_header_table2[i].sh_offset);
            symtab2 = &section_header_table2[i];
            break;
        }
    }

    // Loop over symbols in the first symbol table
    for (int i = 1; i < symtab1->sh_size / sizeof(Elf32_Sym); i++) {
        Elf32_Sym *symbol1 = &symbol_table1[i];

        // If the symbol is UNDEFINED, search for it in the second symbol table
        if (ELF32_ST_TYPE(symbol1->st_info) == STT_NOTYPE) {
            for (int j = 1; j < symtab2->sh_size / sizeof(Elf32_Sym); j++) {
                Elf32_Sym *symbol2 = &symbol_table2[j];
                if (strcmp((char *)((uintptr_t)mapped_file1 + symtab1->sh_link + symbol1->st_name),
                           (char *)((uintptr_t)mapped_file2 + symtab2->sh_link + symbol2->st_name)) == 0 &&
                    ELF32_ST_TYPE(symbol2->st_info) != STT_NOTYPE) {
                    printf("Symbol %s undefined.\n",
                           (char *)((uintptr_t)mapped_file1 + symtab1->sh_link + symbol1->st_name));
                    break;
                }
            }
        } 
        // If the symbol is defined, search for it in the second symbol table
        else if (ELF32_ST_TYPE(symbol1->st_info) != STT_NOTYPE && symbol1->st_shndx != SHN_UNDEF) {
            for (int j = 1; j < symtab2->sh_size / sizeof(Elf32_Sym); j++) {
                Elf32_Sym *symbol2 = &symbol_table2[j];
                if (strcmp((char *)((uintptr_t)mapped_file1 + symtab1->sh_link + symbol1->st_name),
                           (char *)((uintptr_t)mapped_file2 + symtab2->sh_link + symbol2->st_name)) == 0 &&
                    symbol2->st_shndx != SHN_UNDEF) {
                    printf("Symbol %s multiply defined.\n",
                           (char *)((uintptr_t)mapped_file1 + symtab1->sh_link + symbol1->st_name));
                    break;
                }
            }
        }
    }
}

// Stub function for merging ELF files
void mergeELFFiles() {
    printf("Merging ELF files... (Not implemented yet)\n");
}
// Function to quit the program
void quitProgram() {
    if (mapped_file1 != NULL) {
        munmap(mapped_file1, file_size1);
        close(fd1);
        printf("Unmapped and closed first ELF file.\n");
    }
    if (mapped_file2 != NULL) {
        munmap(mapped_file2, file_size2);
        close(fd2);
        printf("Unmapped and closed second ELF file.\n");
    }
    printf("Exiting program.\n");
}
        