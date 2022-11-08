#include <stdio.h>
#include <time.h>

#define DATA_FILE "newdata.csv"
#define CONFIG_FILE "settings.conf"

typedef struct {
    long time_unix;
    int water;
} measurement;

int read_config(int* alarm_time);
int get_length(FILE* fp);
int get_data(FILE* fp, measurement measurements[], int length);
void calc_consumption(measurement measurements[], int length);
int water_per(measurement measurements[], int length, int time);
int time_since_zero(measurement measurements[], int length);
void print_alarm(int time);
int format_time(long time_unix, char time_UTC[]);

int main(void) {
    //Reading configuration
    int alarm_time;
    read_config(&alarm_time);
    FILE* fp;
    int length;
    fp = fopen(DATA_FILE, "r");
    // Kontrollerer, at filen kan åbnes:
    if(!fp) {
        perror("Cannot open file");
        return 1;
    } 
    length = get_length(fp); // Finder linjeantal i fil
    measurement measurements[length]; // Laver en array til målinger 
    get_data(fp, measurements, length);
    calc_consumption(measurements, length);
    time_since_zero(measurements, length);
    fclose(fp);
    return 0;
}

int read_config(int* alarm_time) {
    FILE* cp;
    int input = 1;
    cp = fopen(CONFIG_FILE, "r");
    if(!cp) {
        perror("Couldn't open config file.");
        return 1;
    }
    int num,
        factor;
    char unit;
    fscanf(cp, "alarm_time = %d %c", &num, &unit);
    if(unit == 'w') {
        factor = 604800;
    }
    else if(unit == 'd') {
        factor = 86400;
    }
    else if(unit == 'h') {
        factor = 3600;
    }
    else {
        perror("Error with input. See file: settings.conf");
        return 1;
    }
    *alarm_time = num * factor;
    fclose(cp);
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
    char time_UTC[26];
    format_time(measurements[length - 1].time_unix, time_UTC);
    printf("last measurement was %s\n", time_UTC);
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

int time_since_zero(measurement measurements[], int length) {
    int time = 0;
    for(int i = length - 1; i >= 0; i--) {
        if(measurements[i].water == measurements[i - 1].water) {
            printf("%d\n", i);
            time = measurements[length - 1].time_unix - measurements[i].time_unix;
            if(time == 0) {
                time = 1;
            }
            break;
        }
    }
    print_alarm(time);
}

void print_alarm(int time) {
    int weeks = time / 604800,
        days = (time % 604800) / 86400,
        hours = (time % 86400) / 3600,
        minutes = (time % 3600) / 60,
        seconds = (time % 60);
    if(time > 0) {
        printf("It has been ");
        if(weeks > 0) {
            printf("%d weeks,", weeks);
        }
        if(days > 0) {
            printf("%d days, ", days);
        }
        if(hours > 0) {
            printf("%d hours, ", hours);
        }
        if(minutes > 0) {
            printf("%d minutes, ", minutes);
        }
        if(seconds > 0) {
            printf("%d seconds, ", seconds);
        }
        printf("since the last time there was no change between measurements.");
    }
    else {
        printf("There was no null value of change found in the data.");
    }
}

int format_time(long time_unix, char time_UTC[]) {
    struct tm time_struct;
    time_t rawtime = time_unix;
    time_struct = *localtime(&rawtime);
    strftime(time_UTC, 26, "%a %Y-%m-%d %H:%M:%S", &time_struct);
    return 0;
}