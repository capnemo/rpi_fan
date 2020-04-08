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

void data_collector::monitor_until_steady()
{
    uint32_t total_samples = monitor_time * samples_per_second;
    for (uint32_t i = 0; i < total_samples; i++) {
        temp_vec.push_back(get_cpu_temp());
        usleep(1000000 / samples_per_second);
        if ((i > total_samples/3) && (is_invariant(i - 10, 10))) {
            steady_state_temp = temp_vec[i - 10];
            break;
        }
    }
}

void data_collector::monitor_temperature()
{
    for (uint32_t i = 0; i < monitor_time * samples_per_second; i++) {
        temp_vec.push_back(get_cpu_temp());
        usleep(1000000 / samples_per_second);
    }
}

int32_t data_collector::get_constant_temperature()
{
    int32_t sum = 0;
    for (auto m:temp_vec) 
        sum += m;

    return sum/temp_vec.size();
}

/*
int32_t data_collector::monitor_temperature(int32_t mon_time, 
                                         int32_t sampling_rate)
{
    std::vector<int32_t> t_vec;
    uint32_t total_samples = mon_time * sampling_rate;
    for (uint32_t i = 0; i < total_samples; i++) {
        t_vec.push_back(get_cpu_temp());
        usleep(1000000 / sampling_rate);
    }

    int32_t sum = 0;
    for (auto m:t_vec) 
        sum += m;

    return sum/t_vec.size();
}
*/
bool data_collector::is_invariant(uint32_t start, uint32_t length)
{
    std::unordered_set<int32_t> uniq;
    for (uint32_t i = start; i < start + length; i++)
        uniq.insert(temp_vec[i]);

    if (uniq.size() == 1)
        return true;

    std::map<int32_t, int32_t> ranks;
    for (auto m:uniq)
        ranks.insert(std::pair<uint32_t, uint32_t>(uniq.count(m), m));

    std::map<int32_t, int32_t>::iterator hi,nx;
    hi = ranks.end();
    hi--;
    nx = hi;
    nx--;

    if ((std::abs(hi->second - nx->second) <= 1.2 * sensor_error) &&
       ((hi->first + nx->first) > 0.7 * length))
        return true;

    return false;
}

int32_t data_collector::find_sensor_error() //set sensor_error
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
    printf("%d\n", sensor_error);
    return sensor_error;
}

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
