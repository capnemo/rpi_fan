#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <limits.h>
#include <time.h>
#include <thread>
#include <vector>
#include <set>
#include <unordered_set>
#include <map>
#include <cstdlib>

#include "data_collector.h"


/* 
 * monitor_temperature()
 * Monitors the temperature of the cpu for the specified time and at the
 * specified sample rate.
 */
void data_collector::monitor_temperature()
{
    for (uint32_t i = 0; i < monitor_time * samples_per_second; i++) {
        temp_vec.push_back(get_cpu_temp());
        usleep(1000000 / samples_per_second);
    }
}

/* 
 * get_constant_temperature()
 * Returns the average of the monitored temperatures.
 * Has to be called after monitor_temperature()
 */
int32_t data_collector::get_constant_temperature()
{
    int32_t sum = 0;
    for (auto m:temp_vec) 
        sum += m;

    return sum/temp_vec.size();
}

/*
 * find_sensor_error()
 * Finds the temperature sensor error.
 * Returns the difference between the most common and the second most common
 * values. Assumes no load and 0 fan speed.
 * Has to be called after monitor_temperature()
 */

int32_t data_collector::find_sensor_error()
{
    std::map<uint32_t, uint32_t> ranks;

    for (auto m:temp_vec) {
        std::map<uint32_t, uint32_t>::iterator mpIt = ranks.find(m);
        if (mpIt == ranks.end())
            ranks.insert(std::pair<uint32_t, uint32_t>(m, 1));
        else
            mpIt->second++;
    }

    std::map<int32_t, int32_t> sorted;
    for (auto m:ranks)
        sorted.insert(std::pair<int32_t, int32_t>(m.second, m.first));

    std::map<int32_t, int32_t>::iterator mpIt = sorted.end();
    mpIt--;
    sensor_error = std::abs(mpIt->second - (--mpIt)->second);
    return sensor_error;
}

/*
 * get_cpu_temp
 * Returns the cpu temperature read from sysfs. The unit is millicentigrade.
 *
 */

int32_t data_collector::get_cpu_temp()
{
    int temp_fd;

    temp_fd = open("/sys/class/thermal/thermal_zone0/temp", O_RDONLY);
    if (temp_fd == -1)
        return -1;

    char temp_val[16];
    if (read(temp_fd, temp_val, 16) == -1) {
        perror("CPU temp read:");
        close(temp_fd);
        return -1;
    }

    int cpu_temp = strtol(temp_val, 0, 10);
    if ((cpu_temp == LONG_MIN) || (cpu_temp == LONG_MAX))  {
        close(temp_fd);
        return -1;
    }

    close(temp_fd);
    return cpu_temp;
}
