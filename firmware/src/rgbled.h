/* ************************************************************************** */
/** RGB LED handling functions
 */
/* ************************************************************************** */

#ifndef _RGBLED_H    /* Guard against multiple inclusion */
#define _RGBLED_H


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */
#include <stdint.h>

class RGBLed
{
public:
    // *****************************************************************************
    /**
      @Function
        void init ( ) 

      @Summary
        Initialize the RGB LED
     */
    void init();

    /**
      @Function
        void update(uint8_t r, uint8_t g, uint8_t b) 

      @Summary
        Update the RGB LED with the given color
     */
    void update(uint8_t r, uint8_t g, uint8_t b);
    void updateFromTemp(float t, float sp);

private:
    typedef struct {
        int red;
        int green;
        int blue;
    } RGBColor;

    uint32_t map(uint8_t v);
    RGBColor temperatureToRGB(float temperature, float setpoint);
};

#endif /* _EXAMPLE_FILE_NAME_H */

/* *****************************************************************************
 End of File
 */
