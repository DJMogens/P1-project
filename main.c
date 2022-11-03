#include <stdio.h>
#include <conio.h>
#include <string.h>

typedef struct {
    char time_utc[26];
    long time_unix;
    int water;
} measurement;

int calc_hour(measurement measurements[], int length);

int main(void) {
    FILE *fp;
    measurement measurements[100];
    fp = fopen("data.csv","r");
    if(!fp) {
        perror("Cannot open file");
        return 1;
    } // Kontrollerer, at filen kan åbnes
    int line = 0; // variabel til at tælle linjetallet
    do {
        fscanf(fp, "%ld,%d", &measurements[line].time_unix, &measurements[line].water);
        printf("%ld\n", measurements[line].time_unix);
        printf("%d\n", measurements[line].water);
        line++;
    } while (!feof(fp));
    int hour = calc_hour(measurements, line);

    printf("difference last hour is %d", hour);
    fclose(fp);
    return 0;
}

int calc_hour(measurement measurements[], int length) {
    double ave;
    int i,
        current_water,
        start_water,
        diff;
    long current_time,
         start_time;
    current_water = measurements[length - 1].water;
    current_time = measurements[length - 1].time_unix;
    start_time = current_time - 3600;
    for(i = length - 1; i >= 0 && measurements[i - 1].time_unix >= start_time; i--);
    start_water = measurements[i + 1].water;
    diff = current_water - start_water;

    return diff;
}
