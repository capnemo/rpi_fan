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
    void init() { find_sensor_error(); } //HAS TO BE CALLED FIRST
    void monitor_until_steady();
    //static int32_t monitor_temperature(int32_t mon_time,int32_t sampling_rate);
    void monitor_temperature();
    int32_t get_constant_temperature();
    void get_time_record(std::vector<int>& record) { record = temp_vec; }
    static int32_t get_cpu_temp();
    int32_t get_sensor_error() { return sensor_error;}
    int32_t get_steady_temp() { return steady_state_temp; }

    private:
    bool is_invariant(uint32_t start, uint32_t length);
    int32_t find_sensor_error();

    int32_t sensor_error;
    int32_t steady_state_temp = 0;
    uint32_t monitor_time;
    const uint32_t samples_per_second;
    std::vector<int32_t> temp_vec;
};
