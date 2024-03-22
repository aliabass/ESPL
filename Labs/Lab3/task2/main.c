#include "util.h"
#define SYS_WRITE 4
#define STDOUT 1
#define SYS_OPEN 5
#define SYS_SEEK 19
#define SEEK_SET 0

#define O_DIRECTORY 00200000
#define BufferSize 8192

extern int system_call();
extern void infection();
extern void infector();

int main (int argc , char* argv[], char* envp[])
{
    int virus = 0;
    char *fileN;
    int fd, bytes_read;
    char buffer[BufferSize];
     for(int i = 1; i<argc; i++){   
         if(strncmp(argv[i], "-a", 2) == 0){
             fileN = argv[i] + 2;
             virus = 1;
         }else{
             fileN = argv[i];
         }
     }
    fd = system_call(5, (int)fileN, 0, 0);
    if (fd < 0) {
        system_call(1,0x55);
    }
    bytes_read = system_call(3, fd, (int)buffer, sizeof(buffer));
    if (bytes_read < 0) {
        system_call(6, fd, 0, 0);
        system_call(1,0x55);
    }

     system_call(4, 1, (int)buffer, bytes_read);
     system_call(4, 1, "\n", 1);
     system_call(6, fd, 0, 0);
    if(virus == 0){
        system_call(SYS_WRITE, STDOUT, "", 1);
    }else {
        system_call(SYS_WRITE, STDOUT, "VIRUS ATTACHED to ", 19);
        system_call(SYS_WRITE, STDOUT, fileN, strlen(fileN));
        system_call(SYS_WRITE, STDOUT, "\n", 1);
        infector(fileN);
        infection();
    }
    return 0;
}