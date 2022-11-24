#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#define DATA_FILE "newdata.csv"
#define CONFIG_FILE "settings.conf"
#define SEC_PER_MIN 60
#define SEC_PER_HOUR 3600
#define SEC_PER_DAY 86400
#define SEC_PER_WEEK 604800
#define SEC_PER_FOUR_WEEKS 2419200

// Struct til hver måling fra data-fil
typedef struct {
    long time_unix;
    int water;
} measurement;

// Struct til output (+ beregninger)
typedef struct {
        char timestamp[26];
        int time_unix;
        int water_diff;
        char file_path[26];
        int sec_per_unit;
} data_for_output;

data_for_output output[4] = {
    {"", 0, 0, "./output/hours.txt", SEC_PER_HOUR},
    {"", 0, 0, "./output/days.txt", SEC_PER_DAY},
    {"", 0, 0, "./output/weeks.txt", SEC_PER_WEEK},
    {"", 0, 0, "./output/four_weeks.txt", SEC_PER_FOUR_WEEKS}
};

int read_config(int* alarm_time);
int get_length(FILE* fp);
int get_data(FILE* fp, measurement measurements[], int length);
void calc_start_of_time(measurement first, int results[3]);
void output_to_files(measurement measurements[], int length, int start_times[]);
int water_per_x(measurement measurements[], int length, int output_num);
int time_since_zero(measurement measurements[], int length, int alarm_time);
void print_alarm(int time, int alarm_time);
int format_time(long time_unix, char time_UTC[], struct tm *time_struct);

int main(void) {
    //Reading configuration
    int alarm_time;
    int start_times[3];
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
    calc_start_of_time(measurements[0], start_times);
    output_to_files(measurements, length, start_times);
    time_since_zero(measurements, length, alarm_time);
    fclose(fp);
    return 0;
}

void calc_start_of_time(measurement first, int results[3]) { // Beregner, hvor lang tid, der går, indtil ny hel time, dag og uge
    printf("calc_start_of_time running ...\n");
    int m, h, d, w;
    char time_UTC[26];
    struct tm time_struct;
    format_time(first.time_unix, time_UTC, &time_struct);
    time_struct.tm_wday--;
    // Calculating number of seconds until next full minute, hour, day, week
    m = (60 - time_struct.tm_sec) % 60;
    h = (60 - time_struct.tm_min) % 60 * SEC_PER_MIN - time_struct.tm_sec;
    if(h < 0) { // Above can return a negative value for h. Fixed by this
        h += SEC_PER_HOUR;
    }
    d = (24 - time_struct.tm_hour) % 24 * SEC_PER_HOUR - time_struct.tm_sec - time_struct.tm_min * SEC_PER_MIN;
    if(d < 0) {
        d += SEC_PER_DAY;
    }
    w = (7 - time_struct.tm_wday) % 7 * SEC_PER_DAY - time_struct.tm_sec - time_struct.tm_min * SEC_PER_MIN - time_struct.tm_hour * SEC_PER_HOUR;
    if(w < 0) {
        w += SEC_PER_WEEK;
    }
    results[0] = h;
    results[1] = d;
    results[2] = w;
}

int read_config(int* alarm_time) { // Læser konfiguration fra config-fil
    printf("Reading config ...\n");
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
        factor = SEC_PER_WEEK;
    }
    else if(unit == 'd') {
        factor = SEC_PER_DAY;
    }
    else if(unit == 'h') {
        factor = SEC_PER_HOUR;
    }
    else {
        perror("Error with input. See file: settings.conf");
        return 1;
    }
    *alarm_time = num * factor;
    fclose(cp);
    return 0;
}

int get_length(FILE* fp) { // Tæller antallet af datasæt
    printf("Getting length of data ...\n");
    int sz;
    int length;
    fseek(fp, 0L, SEEK_END);
    sz = ftell(fp);
    rewind(fp);
    length = (sz + 2)/21; //OBS: skal rettes, hvis dataen fylder mere/mindre per linje
    return length;
}

int get_data(FILE* fp, measurement measurements[], int length) { //Indlæser data fra fil
    printf("Getting data ...\n");
    int line = 0; // variabel til at tælle linjetallet
    do {
        fscanf(fp, "%ld,%d", &measurements[line].time_unix, &measurements[line].water);
        line++;
    } while (!feof(fp));
}

void output_to_files(measurement measurements[], int length, int start_times[]) { // Sender output til filer (hours.txt, days.txt etc.)
    printf("output_to_files running ...\n");
    for(int i = 0; i < 4; i++) { // FOR HOURS, DAYS, WEEKS, FOUR WEEKS
        FILE *outp = fopen(output[i].file_path, "w");
        for(int j = 0; j < length; j++) {
            output[i].time_unix = measurements[0].time_unix + start_times[i] + j * output[i].sec_per_unit;
            int fail = water_per_x(measurements, length, i);
            if (fail) {
                break;
            }
            fprintf(outp, "%s: %d litres\n", output[i].timestamp, output[i].water_diff);
        }
        fclose(outp);
    }
}

int water_per_x(measurement measurements[], int length, int output_num) { // Beregner forbrug sidste time, day, uge eller 4 uger
    double ave;
    int i,
        end_water,
        start_water;
    long end_time,
         start_time;
    start_time = output[output_num].time_unix;
    end_time = start_time + output[output_num].sec_per_unit;
    char looking_for = 's';
    for(i = 0; i < length; i++) {
        if(looking_for == 's') { // Looking for the start value
            // Stops loop here and inner loop in output_to_files, because the start time is later than the last measurement
            if(start_time > measurements[length - 1].time_unix) {
                return 1;
            }
            // This is the value we are looking for
            if(measurements[i].time_unix >= start_time) { // 
                start_water = measurements[i].water;
                looking_for = 'e';
                continue;
            }
        }
        if(looking_for == 'e') { // Looking for the end value
            // Safety measure in case of not often enough measurements, max is 1,5 of unit (eg. 1,5 hours)
            if(measurements[i].time_unix >= end_time + output[output_num].sec_per_unit / 2) {
                end_water = start_water;
                printf("Warning: Not often enough measurements\n ");
                break;
            }
            // This is the value we are looking for in most cases
            if(measurements[i].time_unix >= end_time) {
                end_water = measurements[i].water;
                break;
            }
            //If it is the last measurement, then the end water is the last water value. Eg., if hour started at 12:00, it is now 12:30, the result is the water used between 12:00 and 12:30.
            if(i == length - 1) { 
                end_water = measurements[length - 1].water;
            }
        }
    }
    output[output_num].water_diff = end_water - start_water;
    struct tm time_struct;
    format_time(start_time, output[output_num].timestamp, &time_struct);
    return 0;
}

int time_since_zero(measurement measurements[], int length, int alarm_time) { // Beregner, hvor lang tid der er gået siden sidst, der ikke var ændring mellem målinger
    printf("time_since_zero running ...\n");
    int time = 0;
    for(int i = length - 1; i >= 0; i--) {
        if(measurements[i].water == measurements[i - 1].water) {
            time = measurements[length - 1].time_unix - measurements[i].time_unix;
            if(time == 0) {
                time = 1;
            }
            break;
        }
    }
    print_alarm(time, alarm_time);
}

void print_alarm(int time, int alarm_time) { // Printer alarm, hvis der er gået for lang tid siden sidste nul-ændring mellem målinger
    printf("print_alarm running ...\n");
    int weeks = time / SEC_PER_WEEK,
        days = (time % SEC_PER_WEEK) / SEC_PER_DAY,
        hours = (time % SEC_PER_DAY) / SEC_PER_HOUR,
        minutes = (time % SEC_PER_HOUR) / SEC_PER_MIN,
        seconds = (time % SEC_PER_MIN);
    if(time == 0) {
        printf("There was no null value of change found in the data!");
    }
    if(time > alarm_time) {
        printf("BEEP BEEP!\n");
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
        printf("since the last time there was no change between measurements.\n");
    }
}

int format_time(long time_unix, char time_UTC[], struct tm *time_struct) { // Formatterer unix-tid til læseligt format
    time_t rawtime = time_unix;
    *time_struct = *localtime(&rawtime);
    strftime(time_UTC, 26, "%a %Y-%m-%d %H:%M:%S", time_struct);
    return 0;
}
