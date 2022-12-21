#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define START_TIME 1665697169
#define END_TIME 1686692369
#define INTERVAL 600
#define START_WATER 53041

int main(void) {
    FILE* fp;
    fp = fopen("newdata.csv", "w");
    long new_time;
    int new_water = 53041,
        num,
        i;
    srand(time(0));
    for(new_time = 	START_TIME; new_time < 	END_TIME; new_time += INTERVAL) {
        fprintf(fp, "%ld,", new_time);
        for(i = 10; i <= 10000000; i *= 10) {
            if(new_water < i) {
                fprintf(fp, "0");
            }
        }
        fprintf(fp, "%d\n", new_water);
        num = rand() % 20;
        new_water += num;
    }
    fclose(fp);
    return 0;
}