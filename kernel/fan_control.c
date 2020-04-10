#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/pwm.h>
#include <linux/thermal.h>
#include <linux/of.h>


static struct timer_list pid_timer;
static int period = 1;
struct thermal_zone_device *thermal_zone;
struct device_node *dev_node;
struct pwm_state pstate;
static struct pwm_device *pdevice = NULL;
static const u32 default_max_duty_cycle = 10000000;

static u32 prop_constant = 0;
module_param(prop_constant, uint, 0644);
MODULE_PARM_DESC(prop_constant, "Proportional Constant");

static u32 set_point = 0;
module_param(set_point, uint, 0644);
MODULE_PARM_DESC(set_point, "Set Point");

static int sensor_error = 538;
module_param(sensor_error, uint, 0644);
MODULE_PARM_DESC(sensor_error, "Sensor error");

/*
 * init_pwm_control
 * Initializes the pwm hardware device and gains control. The duty cycle is 
 * set to 0 and the period to default_max_duty_cycle.
 */
static int init_pwm_control(void)
{
    struct device_node *dev_node;
    int rc;

    dev_node = of_find_node_by_phandle(0x69);
    if (IS_ERR(dev_node)) {
        pr_info("Error getting device tree node for the pwm chip\n");
        return -ENODEV;
    }
    pr_info("dt node name is %s\n", dev_node->name);
    pr_info("dt node full name is %s\n", dev_node->full_name);

    pdevice = of_pwm_get(dev_node, NULL);
    if (IS_ERR(pdevice)) {
        pr_info("Error getting pwm chip\n");
        return -ENODEV;
    }

    pwm_get_state(pdevice, &pstate);
    pr_info("pwm period is %u\n", pstate.period);
    pr_info("pwm dc is %u\n", pstate.duty_cycle);
    pr_info("pwm config enabled\n");

    pstate.period = default_max_duty_cycle;
    pstate.duty_cycle = 0;
    pstate.enabled = true;
    rc = pwm_apply_state(pdevice, &pstate);
    if (rc != 0) {
        pr_info("Cannot set pwm state\n");
        return rc;
    }

    return 0;
}

/*
 * deinit_pwm_control
 * Deinitializes the pwm hardware and returns control.
 */
static void deinit_pwm_control(void)
{
    if (pdevice != NULL) {
        pwm_disable(pdevice);
        pwm_put(pdevice);
    }
}

/*
 * set_fan_half_speed
 * Sets the fan to half speed. Used for debugging.
 */
void set_fan_half_speed(void)
{
    pstate.duty_cycle = default_max_duty_cycle/2;
    if (pwm_apply_state(pdevice, &pstate))
        pr_info("Error setting pwm state\n");
    pr_info("set fan to half max speed\n");
}

/*
 * set_fan_speed
 * Sets the pwm duty cycle and therefore the speed of the fan.
 * The fraction duty cycle divided by period is the fraction of 
 * the max speed of the fan.
 */
void set_fan_speed(int dc)
{
    pstate.duty_cycle = dc;
    if (pwm_apply_state(pdevice, &pstate))
        pr_info("Error setting pwm state\n");
}

/*
 * get_temperature
 * Reads and returns the cpu temperature. Reads the temperature thrice
 * in succession and picks the best value.
 */
static int get_temperature(void)
{
    int temp_array[3];
    bool read_err = false;
    int temp;
    int i;

    for (i = 0; i < 3; i++) {
        if (thermal_zone_get_temp(thermal_zone, temp_array + i) != 0) {
            pr_info("Cannot get current temperature\n");
            read_err = true;
            break;
        }
    }

    if (read_err) 
        return 0;

    if ((temp_array[0] == temp_array[1]) || (temp_array[0] == temp_array[2]))
        temp = temp_array[0];
    else if (temp_array[1] == temp_array[2]) 
        temp = temp_array[1];
    else 
        temp = (temp_array[0] + temp_array[1] + temp_array[2])/3;

    return temp;
}
/*
 * Raspberry pi's temperature sensor has a large error of 
 * +/- 2 * 538 millicentigrade. This error manifests itself intermittently 
 * even at steady state at no load and no cooling. This is why we read
 * the temperature thrice in get_temperature and there is an adjustment
 * to the error in pid_control. However even with this adjustments, 
 * there is a degree of flutter in the fan speed when the pid control is 
 * in operation. The two adjustments merely reduce the magnitude of 
 * the flutter
 */

/*
 * pid_control 
 * Calculates the appropriate fan speed based on the difference between
 * the current temperature and the set point.
 */
static void pid_control(void)
{
    static int last_error = 0;
    static int error_sum = 0;
    static int total_time = 0;
    static int prev_dc;
    int current_temp;
    int error = 0;
    int current_dc;
    int nominal_error = 2 * sensor_error + sensor_error/10;

    if (set_point == 0)
        return;

    total_time += period;

    if ((current_temp = get_temperature()) == 0)
        return;

    pr_info("current temperature is %d\n", current_temp);
    if (last_error == 0) {
        last_error = error;
    }
    
    error = current_temp - set_point;
    error_sum += error;
    if (error <= 0)
        return;

    if (abs(error) < nominal_error)
        error = 0;

    current_dc = prop_constant * (error + 
                                 (error_sum / total_time) + 
                                 period * (error - last_error));

    if (current_dc > default_max_duty_cycle)
        current_dc = default_max_duty_cycle;
 
    if (abs(current_dc - prev_dc) > (5 * default_max_duty_cycle)/100)   
        set_fan_speed(current_dc);

    prev_dc = current_dc;
    last_error = error;

    pr_info("fan speed is %d\n", current_dc);
}

/*
 * timer_func
 * @exp_timer: the timer struct.
 * Adds a timer to the list of kernel timers. When called, updates the 
 * expiry time.
 */

static void timer_func(struct timer_list *exp_timer)
{
    pid_control();
    exp_timer->expires += period * HZ;
    add_timer(exp_timer);
}

/* init_pid_timer
 * Initialzes the timer.
 */
static void init_pid_timer(void)
{
    pid_timer.expires = jiffies + period * HZ;
    pid_timer.function = timer_func;
    add_timer(&pid_timer);   
}

/*
 * init_fan_control
 * Module entry point.
 */
static int init_fan_control(void)
{
    thermal_zone = thermal_zone_get_zone_by_name("cpu-thermal");
    if (IS_ERR(thermal_zone)) {
        pr_info("Error getting the thermal zone\n");
        return -ENODEV;
    }

    init_pid_timer();

    if (init_pwm_control() != 0) {
        del_timer(&pid_timer);
        return -ENODEV;
    }

    return 0;
}

/*
 * exit_fan_control
 * Module exit point.
 */
static void exit_fan_control(void)
{
    del_timer(&pid_timer);
    deinit_pwm_control();
    pr_info("exit fan_control\n");
}

module_init(init_fan_control);
module_exit(exit_fan_control);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chandra");
MODULE_DESCRIPTION("Fan control module");

