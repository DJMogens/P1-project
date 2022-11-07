#include <stdio.h>
#include <conio.h>
#include <string.h>

typedef struct {
    char time_utc[26];
    long time_unix;
    int water;
} measurement;


int get_length(FILE* fp);
int get_data(FILE* fp, measurement measurements[], int length);
void calc_consumption(measurement measurements[], int length);
int water_per(measurement measurements[], int length, int time);


int main(void) {
    FILE *fp;
    int length;
    fp = fopen("data.csv", "r");
    // Kontrollerer, at filen kan åbnes:
    if(!fp) {
        perror("Cannot open file");
        return 1;
    } 
    length = get_length(fp); // Finder linjeantal i fil
    measurement measurements[length]; // Laver en array til målinger 
    get_data(fp, measurements, length);
    calc_consumption(measurements, length);

    fclose(fp);
    return 0;
}

int get_length(FILE* fp) { //Funktion til at tælle antallet af datasæt
    int sz;
    int length;
    fseek(fp, 0L, SEEK_END);
    sz = ftell(fp);
    rewind(fp);
    length = (sz + 2)/21; //OBS: skal rettes, hvis dataen fylder mere/mindre per linje
    printf("length of file is %d\n", length);
    return length;
}

int get_data(FILE* fp, measurement measurements[], int length) { //Funktion til at indlæse data fra fil
    int line = 0; // variabel til at tælle linjetallet
    do {
        fscanf(fp, "%ld,%d", &measurements[line].time_unix, &measurements[line].water);
        printf("%ld  ", measurements[line].time_unix);
        printf("%d\n", measurements[line].water);
        line++;
    } while (!feof(fp));
}

void calc_consumption(measurement measurements[], int length) {
    int hour,
        day,
        week,
        four_weeks;
    hour = water_per(measurements, length, 3600);
    day = water_per(measurements, length, 86400);
    week = water_per(measurements, length, 604800);
    four_weeks = water_per(measurements, length, 2419200);
    printf("last hour consumption was %d litres\n", hour);
    printf("last day consumption was %d litres\n", day);
    printf("last week consumption was %d litres\n", week);
    printf("last four weeks consumption was %d litres\n", four_weeks);
}

int water_per(measurement measurements[], int length, int time) { // Funktion til at beregne forbrug sidste time
    double ave;
    int i,
        current_water,
        start_water,
        diff;
    long current_time,
         start_time;
    // Finder sidst målte værdi:
    current_water = measurements[length - 1].water;
    current_time = measurements[length - 1].time_unix;
    // Finder først målte værdi:
    start_time = current_time - time;
    for(i = length - 1; i >= 0 && measurements[i - 1].time_unix >= start_time; i--);
    start_water = measurements[i].water;
    // Beregner forskel:
    diff = current_water - start_water;

    return diff;
}
