#include <cstdint>

class fan_control {

public:
    fan_control() {}
    int8_t init();
    int8_t set_duty_cycle(uint64_t dc);
    int8_t stop_fan() {return set_duty_cycle(0);}
    int8_t run_fan_max() {return set_duty_cycle(period);}
    static uint64_t get_period() { return period; }
    void disable_pwm(void);

private:
    int8_t setup_pwm(uint64_t period, uint64_t duty_cycle);
    static uint64_t period;
};
