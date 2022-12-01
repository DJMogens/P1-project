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
#define SEC_PER_MONTH 2419200

// Struct til hver måling fra data-fil
typedef struct {
    long time_unix;
    int water;
} measurement;

// Struct til output (+ beregninger)
typedef struct {
    struct {
        int time_unix;
        int water_diff;
        int sec_per_unit;
    } calc;
    struct {
        char timestamp[26];
        char file_path[30];
    } txt;
    struct {
        char name[12];
        char time_fmt[10];
        char time_unit[10];
    } graph;
} data;

data output[4] = {
    {0, 0, SEC_PER_HOUR, "", "./output/hours/hours.txt", "hours", "\"\%H:\%M\"", "HH:MM"},
    {0, 0, SEC_PER_DAY, "", "./output/days/days.txt", "days", "\"\%m-\%d\"", "mm-dd"},
    {0, 0, SEC_PER_WEEK, "", "./output/weeks.txt", "weeks", "\"\%m-\%d\"", "mm-dd"},
    {0, 0, SEC_PER_MONTH, "", "./output/month.txt", "month", "\"\%Y-\%m\"", "yyyy-mm"}
};

char month_names[12][3] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

int read_config(int* alarm_time);
int get_length(FILE* fp);
int get_data(FILE* fp, measurement measurements[], int length);
void calc_sec_in_month(long int time_unix);
void calc_start_of_time(measurement first, int results[3]);
int check_leap_year(int year);
void output_to_files(measurement measurements[], int length, int start_times[]);
void create_graph(int output_num);
int water_per_x(measurement measurements[], int length, int output_num, long graph_start, int correct_for_dst);
int time_since_zero(measurement measurements[], int length, int alarm_time);
void print_alarm(int time, int alarm_time);
int format_time(long time_unix, char time_UTC[], struct tm *time_struct);
int check_dst(long time_unix);

int main(void) {
    //Reading configuration
    int alarm_time;
    int start_times[4];
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

void calc_start_of_time(measurement first, int results[4]) { // Beregner, hvor lang tid, der går, indtil ny hel time, dag og uge
    printf("calc_start_of_time running ...\n");
    int m, h, d, w, mon;
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
    calc_sec_in_month(first.time_unix);
    mon = (output[3].calc.sec_per_unit - time_struct.tm_mday + 1) % output[3].calc.sec_per_unit * SEC_PER_DAY * output[3].calc.sec_per_unit - time_struct.tm_sec - time_struct.tm_min * SEC_PER_MIN - time_struct.tm_hour * SEC_PER_HOUR;
    if(mon < 0) {
        mon += output[3].calc.sec_per_unit;
    }
    results[0] = h;
    results[1] = d;
    results[2] = w;
    results[3] = mon;
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
    for(int i = 0; i < 4; i++) { // FOR HOURS, DAYS, WEEKS, MONTH
        output[i].calc.time_unix = measurements[0].time_unix + start_times[i];
        long graph_start;
        int fail;
        do {
            calc_sec_in_month(output[i].calc.time_unix);
            int is_dst_start = check_dst(output[i].calc.time_unix);
            int correct_for_dst = 0;
            if(i == 0) {
                struct tm graph_tm;
                char time_UTC[26];
                char filepath[26];
                format_time(output[i].calc.time_unix, time_UTC, &graph_tm);
                snprintf(output[i].txt.file_path, 29, ".\\output\\hours\\%.3s-%d.txt", month_names[graph_tm.tm_mon], graph_tm.tm_mday);
                snprintf(output[i].graph.name, 11, "%.3s-%d", month_names[graph_tm.tm_mon], graph_tm.tm_mday);
            }
            if(i == 1) {
                struct tm graph_tm;
                char time_UTC[26];
                char filepath[26];
                format_time(output[i].calc.time_unix, time_UTC, &graph_tm);
                snprintf(output[i].txt.file_path, 29, ".\\output\\days\\%.3s.txt", month_names[graph_tm.tm_mon]);
                snprintf(output[i].graph.name, 11, "%.3s", month_names[graph_tm.tm_mon]);
            }
            graph_start = output[i].calc.time_unix;
            FILE *outp = fopen(output[i].txt.file_path, "w");
            do {
                int is_dst_now = check_dst(output[i].calc.time_unix);
                if(is_dst_start ^ is_dst_now) { // XOR is_dst_start (was DST when this output started) and if DST at start of this measurement
                    correct_for_dst = is_dst_start - is_dst_now; // Will give 1 if DST switched on, -1 if DST switched off
                    is_dst_start = is_dst_now;
                }
                if(i) {
                    output[i].calc.time_unix += correct_for_dst * SEC_PER_HOUR;
                    correct_for_dst = 0;
                }
                fail = water_per_x(measurements, length, i, graph_start, correct_for_dst);
                if(!fail) {
                    fprintf(outp, "%s; %d\n", output[i].txt.timestamp, output[i].calc.water_diff);
                    output[i].calc.time_unix += output[i].calc.sec_per_unit;
                }
            } while (!fail);
            create_graph(i);
            fclose(outp);
        } while(i < 2 && fail == 2);
    }   
}

void create_graph(int output_num) {
    const int num_commands = 9;
    char* commandsForGnuplot[] = {
        "set datafile separator \";\"",
        "set ylabel 'water (litres)'",
        "set xdata time",
        "set xrange [*:*]",
        "set yrange [0:*]",
        "set timefmt '%a %Y-%m-%d %H:%M:%S'",
        "set grid",
        "set terminal png size 2000,1000 enhanced font \"Arial,20\"",
        "set style fill solid 1.00 border 0"
    };
    FILE* gnuplotPipe = popen("gnuplot -persistent", "w");
    for(int i = 0; i < num_commands; i++) {
        fprintf(gnuplotPipe, "%s \n", commandsForGnuplot[i]);
    }
    fprintf(gnuplotPipe, "set xlabel 'time (%s)' \n", output[output_num].graph.time_unit);
    fprintf(gnuplotPipe, "set title '%s' \n", output[output_num].graph.name);
    fprintf(gnuplotPipe, "set format x %s \n",  output[output_num].graph.time_fmt);
    if(output_num == 0) {
        fprintf(gnuplotPipe, "set output '.\\output\\hours\\%s.png' \n", output[output_num].graph.name);
    }
    else if(output_num == 1) {
        fprintf(gnuplotPipe, "set output '.\\output\\days\\%s.png' \n", output[output_num].graph.name);
    }
    else {
        fprintf(gnuplotPipe, "set output '.\\output\\%s.png' \n", output[output_num].graph.name);
    }
    fprintf(gnuplotPipe, "plot '%s' using 1:2 with boxes linecolor rgb \"red\" \n", output[output_num].txt.file_path);
}

void calc_sec_in_month(long int time_unix) {
    int days_in_each_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    struct tm time_struct;
    char irr_time[26];
    format_time(time_unix, irr_time, &time_struct);
    int days_in_month = days_in_each_month[time_struct.tm_mon];
    if(time_struct.tm_mon == 1 && check_leap_year(time_struct.tm_year + 1900)) {
        days_in_month = 29; // Kontrollerer for skudår
    }
    output[3].calc.sec_per_unit = SEC_PER_DAY * days_in_month;
}

int check_leap_year(int year) {
    if(year % 400 == 0) return 1;
    if(year % 100 == 0) return 0;
    if(year % 4 == 0) return 1;
    return 0;
}

int water_per_x(measurement measurements[], int length, int output_num, long graph_start, int correct_for_dst) { // Beregner forbrug sidste time, day, uge eller 4 uger
    double ave;
    int i,
        end_water,
        start_water;
    long end_time,
         start_time;
    start_time = output[output_num].calc.time_unix;
    end_time = start_time + output[output_num].calc.sec_per_unit;
    if(output_num == 3) {
        calc_sec_in_month(start_time);
    }
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
            if(measurements[i].time_unix >= end_time + output[output_num].calc.sec_per_unit / 2) {
                end_water = start_water;
                printf("Warning: Not often enough measurements\n ");
                break;
            }
            // Returns 2 if calculating for days and going to next month, or calculating for hours and going to next day
            if((output_num == 0 && measurements[i].time_unix > graph_start + output[output_num + 1].calc.sec_per_unit + correct_for_dst * SEC_PER_HOUR) || (output_num == 1 && measurements[i].time_unix > graph_start + output[output_num + 2].calc.sec_per_unit)) {
                return 2;
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
    output[output_num].calc.water_diff = end_water - start_water;
    struct tm time_struct;
    format_time(start_time, output[output_num].txt.timestamp, &time_struct);
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

int check_dst(long time_unix) {
    char time_UTC[20];
    struct tm time_struct;
    format_time(time_unix, time_UTC, &time_struct);
    int is_dst = time_struct.tm_isdst;
    return is_dst;
}