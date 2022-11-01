#include <stdio.h>
#include <conio.h>
#include <string.h>

int main(void) {
    FILE *fp;
    typedef struct {
        char time[21];
        int water;
    } measurement;

    fp = fopen("data.csv","r");
    if(!fp) {
        perror("Cannot open file");
        return 1;
    } // Kontrollerer, at filen kan åbnes
    measurement measurements[100]; //Laver en array af enkelte målinger (lige nu plads til 100 målinger)
    int line = 0; // variabel til at tælle linjetallet
    do {
        fscanf(fp, "%20[^,],%d", measurements[line].time, &measurements[line].water);
        printf("%s\n", measurements[line].time);
        printf("%d", measurements[line].water);
        line++;
    } while (!feof(fp));

    fclose(fp);
    return 0;
}