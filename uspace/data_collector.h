#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <limits.h>
#include <time.h>
#include <thread>
#include <vector>
#include <set>
#include <map>
#include <cstdlib>

class data_collector {
 
    public:   
    data_collector(uint32_t collect_time, uint32_t sps): 
                   monitor_time(collect_time), samples_per_second(sps) {}
    void monitor_temperature();
    int32_t get_constant_temperature();
    static int32_t get_cpu_temp();
    int32_t find_sensor_error();

    private:
    uint32_t monitor_time;
    uint32_t sensor_error = 0;
    const uint32_t samples_per_second;
    std::vector<int32_t> temp_vec;
};
