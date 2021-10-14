#include <application.h>

// Service mode interval defines how much time
#define SERVICE_MODE_INTERVAL (15 * 60 * 1000)
#define BATTERY_UPDATE_INTERVAL (60 * 60 * 1000)
#define ACCELEROMETER_UPDATE_NORMAL_INTERVAL (100)


twr_lis2dh12_t lis2dh12;
twr_lis2dh12_result_g_t result;

twr_lis2dh12_alarm_t alarm;

twr_led_t led;

float magnitude;


twr_tick_t startSeconds = 0;
twr_tick_t endSeconds = 0;

bool falling = false;

// This function dispatches battery events
void battery_event_handler(twr_module_battery_event_t event, void *event_param)
{
    // Update event?
    if (event == TWR_MODULE_BATTERY_EVENT_UPDATE)
    {
        float voltage;

        // Read battery voltage
        if (twr_module_battery_get_voltage(&voltage))
        {
            twr_log_info("APP: Battery voltage = %.2f", voltage);

            // Publish battery voltage
            twr_radio_pub_battery(&voltage);
        }
    }
}

// This function dispatches accelerometer events
void lis2dh12_event_handler(twr_lis2dh12_t *self, twr_lis2dh12_event_t event, void *event_param)
{
    float finalTime;
    // Update event?
    if (event == TWR_LIS2DH12_EVENT_UPDATE)
    {
        // Successfully read accelerometer vectors?
        if (twr_lis2dh12_get_result_g(self, &result))
        {
            magnitude = pow(result.x_axis, 2) + pow(result.y_axis, 2) + pow(result.z_axis, 2);
            magnitude = sqrt(magnitude);
            //twr_log_info("%.4f", magnitude);
        }

        if(magnitude < 0.12 && !falling)
        {
            twr_led_set_mode(&led, TWR_LED_MODE_ON);
            startSeconds = twr_tick_get();
            falling = true;
        }
        else if(magnitude > 0.3 && falling)
        {
            twr_led_set_mode(&led, TWR_LED_MODE_OFF);
            endSeconds = twr_tick_get();
            falling = false;
            finalTime = ((endSeconds - startSeconds) / (float)1000);
            twr_log_info("padal %.4f", finalTime);
            twr_log_info("-----------------");
            twr_radio_pub_float("free-fall-dur", &finalTime);
        }

    }
}


void application_init(void)
{

    // Initialize log
    twr_log_init(TWR_LOG_LEVEL_INFO, TWR_LOG_TIMESTAMP_ABS);
    twr_log_info("APP: Reset");

    // Initialize battery
    twr_module_battery_init();
    twr_module_battery_set_event_handler(battery_event_handler, NULL);
    twr_module_battery_set_update_interval(BATTERY_UPDATE_INTERVAL);

    twr_led_init(&led, TWR_GPIO_LED, false, false);
    twr_led_set_mode(&led, TWR_LED_MODE_OFF);

    // Initialize accelerometer
    twr_lis2dh12_init(&lis2dh12, TWR_I2C_I2C0, 0x19);
    twr_lis2dh12_set_event_handler(&lis2dh12, lis2dh12_event_handler, NULL);
    twr_lis2dh12_set_resolution(&lis2dh12, TWR_LIS2DH12_RESOLUTION_8BIT);
    twr_lis2dh12_set_scale(&lis2dh12, TWR_LIS2DH12_SCALE_16G);
    twr_lis2dh12_set_update_interval(&lis2dh12, ACCELEROMETER_UPDATE_NORMAL_INTERVAL);

    // Initialize radio
    twr_radio_init(TWR_RADIO_MODE_NODE_SLEEPING);

    // Send radio pairing request
    twr_radio_pairing_request("free-fall-meter", VERSION);
}
