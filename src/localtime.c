/*
 * Copyright (c) 2018, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <time.h>

/* This is a safe version of localtime() which contains no locks and is
 * fork() friendly. Even the _r version of localtime() cannot be used safely
 * in Redis. Another thread may be calling localtime() while the main thread
 * forks(). Later when the child process calls localtime() again, for instance
 * in order to log something to the Redis log, it may deadlock: in the copy
 * of the address space of the forked process the lock will never be released.
 *
 * This function takes the timezone 'tz' as argument, and the 'dst' flag is
 * used to check if daylight saving time is currently in effect. The caller
 * of this function should obtain such information calling tzset() ASAP in the
 * main() function to obtain the timezone offset from the 'timezone' global
 * variable. To obtain the daylight information, if it is currently active or not,
 * one trick is to call localtime() in main() ASAP as well, and get the
 * information from the tm_isdst field of the tm structure. However the daylight
 * time may switch in the future for long running processes, so this information
 * should be refreshed at safe times.
 *
 * Note that this function does not work for dates < 1/1/1970, it is solely
 * designed to work with what time(NULL) may return, and to support Redis
 * logging of the dates, it's not really a complete implementation. */
// 判断是否为闰年
static int is_leap_year(time_t year) {
    if (year % 4) return 0;         /* A year not divisible by 4 is not leap. */
    else if (year % 100) return 1;  /* If div by 4 and not 100 is surely leap. */
    else if (year % 400) return 0;  /* If div by 100 *and* 400 is not leap. */
    else return 1;                  /* If div by 100 and not by 400 is leap. */
}

// 这是localtime()的安全版本，它不包含锁，而且fork()友好。即使是localtime()的_r版本也不能在Redis中安全使用。
// 当主线程forks()的时候，另一个线程可能正在调用localtime()。然后，当子进程再次调用localtime()时，例如为了将一些东西记录到Redis日志中，它可能会死锁:
// 在分支进程的地址空间的副本中，锁永远不会被释放。

// 此函数以'tz'为参数，'dst'标志用于检查夏时制当前是否有效。
// 这个函数的调用者应该在main()函数中调用tzset()尽快获得这样的信息，以便从“timezone”全局变量中获得时区偏移量。
// 要获取夏令时信息(如果它当前是活动的或非活动的)，一个技巧是也尽快调用main()中的localtime()，并从tm结构的tm_isdst字段获取信息。
// 然而，夏令时时间将来可能会为长时间运行的进程切换，因此这些信息应该在安全的时间刷新。

// 注意，这个函数使用的日期不能< 1/1/1970，它只是用于time(null)可能的返回，并支持Redis日志的日期，这不是一个真正完整的实现。
void nolocks_localtime(struct tm *tmp, time_t t, time_t tz, int dst) {
    const time_t secs_min = 60;     // 一分钟的秒数
    const time_t secs_hour = 3600;  // 一小时的秒数
    const time_t secs_day = 3600*24;// 一天的秒数

    t -= tz;                            /* Adjust for timezone. */  // 调整时区
    t += 3600*dst;                      /* Adjust for daylight time. */ // 调整夏令时
    time_t days = t / secs_day;         /* Days passed since epoch. */  // 过去的天数
    time_t seconds = t % secs_day;      /* Remaining seconds. */    // 过去的不满一天的秒数

    tmp->tm_isdst = dst;
    tmp->tm_hour = seconds / secs_hour; // 当天已过去的小时数
    tmp->tm_min = (seconds % secs_hour) / secs_min; // 当前小时已过去的分钟数
    tmp->tm_sec = (seconds % secs_hour) % secs_min; // 当前分钟已过去的秒数

    /* 1/1/1970 was a Thursday, that is, day 4 from the POV of the tm structure
     * where sunday = 0, so to calculate the day of the week we have to add 4
     * and take the modulo by 7. */
    // 1970年1月1日是星期四，也就是tm结构的POV的第4天，其中星期日= 0，所以要计算星期几，我们必须把4加起来，然后取7的模。
    tmp->tm_wday = (days+4)%7;

    /* Calculate the current year. */
    // 通过天数计算年份
    tmp->tm_year = 1970;
    while(1) {
        /* Leap years have one day more. */
        time_t days_this_year = 365 + is_leap_year(tmp->tm_year);   // 判断是否为闰年
        if (days_this_year > days) break;
        days -= days_this_year;
        tmp->tm_year++;
    }
    tmp->tm_yday = days;  /* Number of day of the current year. */

    /* We need to calculate in which month and day of the month we are. To do
     * so we need to skip days according to how many days there are in each
     * month, and adjust for the leap year that has one more day in February. */
    // 我们需要计算我们是在哪个月和哪个月的哪一天。为了做到这一点，我们需要根据每个月有多少天来跳过几天，并针对二月份多一天的闰年进行调整。
    int mdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    mdays[1] += is_leap_year(tmp->tm_year);

    tmp->tm_mon = 0;
    while(days >= mdays[tmp->tm_mon]) {
        days -= mdays[tmp->tm_mon];
        tmp->tm_mon++;
    }

    tmp->tm_mday = days+1;  /* Add 1 since our 'days' is zero-based. */
    tmp->tm_year -= 1900;   /* Surprisingly tm_year is year-1900. */
}

#ifdef LOCALTIME_TEST_MAIN
#include <stdio.h>

int main(void) {
    /* Obtain timezone and daylight info. */
    tzset(); /* Now 'timezome' global is populated. */
    time_t t = time(NULL);
    struct tm *aux = localtime(&t);
    int daylight_active = aux->tm_isdst;

    struct tm tm;
    char buf[1024];

    nolocks_localtime(&tm,t,timezone,daylight_active);
    strftime(buf,sizeof(buf),"%d %b %H:%M:%S",&tm);
    printf("[timezone: %d, dl: %d] %s\n", (int)timezone, (int)daylight_active, buf);
}
#endif
