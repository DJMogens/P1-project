#include <stdio.h>
#include <time.h>
#include <string.h>

#define DATA_FILE "newdata.csv"
#define CONFIG_FILE "settings.conf"
#define SEC_PER_MIN 60
#define SEC_PER_HOUR 3600
#define SEC_PER_DAY 86400
#define SEC_PER_WEEK 604800
#define SEC_PER_FOUR_WEEKS 2419200

// Struct til hver måling
typedef struct {
    long time_unix;
    int water;
} measurement;

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
int time_since_zero(measurement measurements[], int length);
void print_alarm(int time);
int format_time(long time_unix, char time_UTC[]);

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
    time_since_zero(measurements, length);
    fclose(fp);
    return 0;
}

void calc_start_of_time(measurement first, int results[3]) {
    int m, h, d, w,
        input_s, input_m, input_h, input_d;
    char time_UTC[26];
    char input_d_char[3];
    char weekdays[7][3] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    // Formatterer tiden læseligt
    format_time(first.time_unix, time_UTC);
    printf("%s\n", time_UTC);
    // Lægger starttiden ind i sekunder, minutter, timer
    input_s = 10 * (int)(time_UTC[21] - 48) + (int)(time_UTC[22] - 48);
    input_m = 10 * (int)(time_UTC[18] - 48) + (int)(time_UTC[19] - 48);
    input_h = 10 * (int)(time_UTC[15] - 48) + (int)(time_UTC[16] - 48);
    // Lægger dagen fra formatteret tid over i ny array
    for(int i = 0; i < 3; i++) {
        input_d_char[i] = time_UTC[i];
    }
    // Lægger dag over i et integer (mandag = 0, tirsdag = 1...)
    for(int i = 0; i < 7; i++) {
        if(strncmp(weekdays[i], input_d_char, 3) == 0) {
            input_d = i;
            break;
        }
    }
    printf("input_d is %d\n", input_d);
    // Calculating number of seconds until next full minute, hour, day, week
    m = (60 - input_s) % 60;
    h = (60 - input_m) % 60 * SEC_PER_MIN - input_s;
    if(h < 0) { // Above can return a negative value for h. Fixed by this
        h += SEC_PER_HOUR;
    }
    printf("time until next whole hour is %d\n", h);
    d = (24 - input_h) % 24 * SEC_PER_HOUR - input_s - input_m * SEC_PER_MIN;
    if(d < 0) {
        d += SEC_PER_DAY;
    }
    printf("time until next whole day is %d\n", d);
    w = (7 - input_d) % 7 * SEC_PER_DAY - input_s - input_m * SEC_PER_MIN - input_h * SEC_PER_HOUR;
    if(w < 0) {
        w += SEC_PER_WEEK;
    }
    printf("time until next whole week is %d\n", w);
    results[0] = h;
    results[1] = d;
    results[2] = w;
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

void output_to_files(measurement measurements[], int length, int start_times[]) {
    printf("output_to_files running ...\n");
    printf("%s\n", output[0].file_path);
    for(int i = 0; i < 4; i++) { // FOR HOURS, DAYS, WEEKS, FOUR WEEKS
        FILE *outp = fopen(output[i].file_path, "w");
        for(int j = 0; j < length; j++) {   // FOR SOMETHING ?????
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

int water_per_x(measurement measurements[], int length, int output_num) { // Funktion til at beregne forbrug sidste time, day, uge eller 4 uger
    double ave;
    int i,
        end_water,
        start_water;
    long end_time,
         start_time;
    start_time = output[output_num].time_unix;
    end_time = start_time + output[output_num].sec_per_unit;
    for(i = 0; i < length; i++) {
        if(start_time > measurements[length - 1].time_unix) {
            printf("ERROR\n");
            return 1;
        }
        if(measurements[i].time_unix >= start_time) {
            start_water = measurements[i].water;
            break;
        }
    }
    for(i += 1; i < length; i++) {
        if(measurements[i].time_unix >= end_time + output[output_num].sec_per_unit / 2) { // Safety measure in case of not often enough measurements, max is 1,5 of unit (eg. 1,5 hours)
            end_water = start_water;
            printf("ERROR 2\n ");
            break;
        }
        if(measurements[i].time_unix >= end_time) {
            end_water = measurements[i].water;
            break;
        }
        if(i == length - 1) {
            end_water = measurements[length - 1].water;
        }
    }
    output[output_num].water_diff = end_water - start_water;
    format_time(start_time, output[output_num].timestamp);
    return 0;
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
