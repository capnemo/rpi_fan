#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <signal.h>

#include "fan_control.h"

uint64_t fan_control::period = 10000000;

const char *pwm_chip = "/sys/class/pwm/pwmchip0/pwm0";
const char *pwm_export = "/sys/class/pwm/pwmchip0/export";
const char *pwm_period = "/sys/class/pwm/pwmchip0/pwm0/period";
const char *pwm_enable = "/sys/class/pwm/pwmchip0/pwm0/enable";
const char *pwm_unexport = "/sys/class/pwm/pwmchip0/unexport";
const char *pwm_duty_cycle = "/sys/class/pwm/pwmchip0/pwm0/duty_cycle";

/*
 * Initialize the class and the pwm hardware.
 * The period is hard coded and is specific to the raspberry pi 3 b pwm.
 */
int8_t fan_control::init()
{
    if (setup_pwm(period, 0) != 0) 
        return -1;

    return 0;
}

/*
 * Initializes the pwn to a specific period and duty cycle.
 * @period:      Period of the pwm hardware or the max duty cycle.
 * @duty_cycle:  Starting duty cycle. Should be 0 to keep the fan still.
 */

int8_t fan_control::setup_pwm(uint64_t period, uint64_t duty_cycle)
{
    struct stat pwm_stat;
    if (stat(pwm_chip, &pwm_stat) != 0) {
        int pwm_fd = open(pwm_export, O_WRONLY);
        if (pwm_fd == -1)
            return -2;
     
        const char* buf = "0";
        if (write(pwm_fd, buf, 1) == -1) {
            perror("first write");
            close(pwm_fd);
            return -3;
        }
        close(pwm_fd);
    }
 
    int period_fd = open(pwm_period, O_WRONLY);
    if (period_fd == -1) 
        return -5;

    char period_str[32];
    sprintf(period_str, "%llu", period);
    if (write(period_fd, period_str, strlen(period_str)) == -1) {
        printf("setting period\n");
        close(period_fd);
        return -6;
    }
    close(period_fd);

    if (set_duty_cycle(duty_cycle) == -1) 
        return -7;

    int enb_fd = open(pwm_enable, O_WRONLY);
    if (enb_fd == -1) 
        return -8;

    const char *enable_str = "1";
    if (write(enb_fd, enable_str, 1) == -1) {
        close(enb_fd);
        return -9;
    }
    close(enb_fd);

    return 0;
}

/*
 *  Disables pwm. This will also cause the fan to turn off.
 *
 */

void fan_control::disable_pwm()
{
    int dsb_fd = open("/sys/class/pwm/pwmchip0/pwm0/enable", O_WRONLY);
    if (dsb_fd == -1)  {
        printf("Error opening enable\n");
        return;
    }

    const char *disable_str = "0";
    write(dsb_fd, disable_str, 1);
    close(dsb_fd);

    int exp_fd = open("/sys/class/pwm/pwmchip0/unexport", O_WRONLY);
    if (exp_fd == -1) {
        printf("Error opening unexport\n");
        return;
    }
     
    const char* buf = "1";
    write(exp_fd, buf, 1);
    close(exp_fd);
}

/*
 * Sets the duty cycle of the pwm hardware and therefore the fan.
 * @duty_cycle: The duty cycle. The fraction duty_cycle/period is
 *              the fraction of max speed of the fan.
 *
 */
int8_t fan_control::set_duty_cycle(uint64_t duty_cycle)
{
    int dc_fd = open("/sys/class/pwm/pwmchip0/pwm0/duty_cycle", O_WRONLY);
    if (dc_fd == -1) 
        return -1;

    uint64_t dc = duty_cycle;
    char dc_str[32];
    sprintf(dc_str, "%llu", dc);
    if (write(dc_fd, dc_str, strlen(dc_str)) == -1) {
        close(dc_fd);
        return -1;
    }

    close(dc_fd);
    return 0;
}
