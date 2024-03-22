#include <stdio.h>
int digit_cnt(const char *str) {
    int count = 0;
    while (*str != '\0') {
        if (*str >= '0' && *str <= '9') {
            count++;
        }
        str++;
    }
    return count;
}

int main() {
    char input[] = "Hello123World456";
    int numDigits = digit_cnt(input);
    printf("Number of digits in '%s' is: %d\n", input, numDigits);
    return 0;
}