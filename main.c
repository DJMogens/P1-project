#include <stdio.h>
#include <conio.h>
#include <string.h>

#define DATA_FILE 'text.csv'


int main(void) {
    FILE *fp;
    enum header{time, water};


    fp = fopen("text.csv","r");
    if(!fp) {printf("Cannot open file");} // Kontrollerer, at filen kan åbnes
    else {
        char buffer[1024];
        int row = 0;
        int column = 0;
        while (fgets(buffer, 1024, fp)) {
            column = 0;
            row++;
            char* value = strtok(buffer, ",");
            while (value) {
                //do something for every value in row
                printf("%c", value);
                column++;
            }
        }
    }

    return 0;
}