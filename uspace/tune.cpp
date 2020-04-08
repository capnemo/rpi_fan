#include <iostream>

#include "fan_control.h"
#include "data_collector.h"

/*
 * Runs the fan for a period of 60 seconds at half the max. duty cycle. 
 * Calculates the proportional constant from  the formula
 * pc = bump/temperature difference.
 *
 */

int main(int argc, char* argv[])
{
    if ((getuid() != 0) || (geteuid()  != 0)) {
        printf("Error, not root\n");
        return -1;
    }

    int32_t bump_duty_cycle = fan_control::get_period()/2;

    data_collector init_temp(5, 4);
    init_temp.monitor_temperature();
    uint32_t start_temp = init_temp.get_constant_temperature();

    fan_control fctrl;
    if (fctrl.init() != 0)  {
        std::cerr << "Error initializing the fan" << std::endl;
        return -1;
    }

    std::cout << "starting the fan" << std::endl;
    if (fctrl.set_duty_cycle(bump_duty_cycle) == -1) {
        std::cerr << "Cannot set the speed" << std::endl;
        return -1;
    }
    sleep(60);
    fctrl.stop_fan();
    std::cout << "stopping the fan" << std::endl;

    data_collector end_temp(2, 10);
    end_temp.monitor_temperature();
    uint32_t post_cooling_temp = end_temp.get_constant_temperature();
    
    float prop_constant = bump_duty_cycle/
                          (float)(start_temp - post_cooling_temp);  

    fctrl.disable_pwm();
    std::cout << "start and stop temperatures are:" << start_temp << ","
              << post_cooling_temp << std::endl;
    std::cout << "Bump is " << bump_duty_cycle << std::endl;
    std::cout << "Proportional constant is " << prop_constant << std::endl;
}
