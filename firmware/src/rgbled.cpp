/* ************************************************************************** */
/** RGB LED handling functions
 */
/* ************************************************************************** */

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */
#include "rgbled.h"
#include "definitions.h"

#define RGBLED_MIN      0
#define RGBLED_MAX      30000

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: File Scope or Global Data                                         */
/* ************************************************************************** */
/* ************************************************************************** */

uint32_t RGBLed::map(uint8_t v)
{
    return RGBLED_MAX - (((uint32_t)v * RGBLED_MAX) / 255);
}

RGBLed::RGBColor RGBLed::temperatureToRGB(float temperature, float setpoint) {
    // Define the temperature range (adjust as needed)
    float min_temp = setpoint - 2.0;
    float max_temp = setpoint + 10.0;

    if (temperature < min_temp)
        temperature = min_temp;
    if (temperature > max_temp)
        temperature = max_temp;
    
    // Map temperature to a value between 0 and 1
    float normalized_temp = (float)(temperature - min_temp) / (max_temp - min_temp);
    
    // Interpolate between blue (cold) and red (hot) using the normalized temperature
    int blue = (int)((1.0 - normalized_temp) * 255.0);
    int red = (int)(normalized_temp * 255.0);
    int green = 0;  // You can modify this based on your preference
    
    // Ensure values are within the valid RGB range (0 to 255)
    red = (red < 0) ? 0 : ((red > 255) ? 255 : red);
    blue = (blue < 0) ? 0 : ((blue > 255) ? 255 : blue);
    green = (green < 0) ? 0 : ((green > 255) ? 255 : green);
    
    RGBColor rgb_color = {red, green, blue};
    return rgb_color;
}

void RGBLed::init()
{    
    TCC0_PWMStart();
    TCC1_PWMStart();
    TCC1_PWM24bitDutySet(TCC1_CHANNEL0, RGBLED_MAX); 
    TCC0_PWM24bitDutySet(TCC0_CHANNEL3, RGBLED_MAX); 
    TCC0_PWM24bitDutySet(TCC0_CHANNEL2, RGBLED_MAX); 
}

void RGBLed::update(uint8_t r, uint8_t g, uint8_t b)
{
    int rd = map(r);
    int gd = map(g);
    int bd = map(b);

    TCC1_PWM24bitDutySet(TCC1_CHANNEL0, rd); 
    TCC0_PWM24bitDutySet(TCC0_CHANNEL3, gd); 
    TCC0_PWM24bitDutySet(TCC0_CHANNEL2, bd); 
}

void RGBLed::updateFromTemp(float t, float sp)
{
    RGBColor c = temperatureToRGB(t, sp);
    update(c.red, c.green, c.blue);
}
