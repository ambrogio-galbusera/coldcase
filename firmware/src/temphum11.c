
/*!
 * \file
 *
 */

#include "definitions.h"
#include "temphum11.h"

// ------------------------------------------------ PUBLIC FUNCTION DEFINITIONS

void temphum11_default_cfg ( )
{
    temphum11_write_config( TEMPHUM11_NORMAL_OPERATION | TEMPHUM11_HEATER_DISABLED |
                            TEMPHUM11_TEMP_HUM_ACQUIRED | TEMPHUM11_TEMP_RESOLUTION_14bit |
                            TEMPHUM11_HUM_RESOLUTION_14bit ); 
}

void temphum11_write_config ( uint16_t config )
{
    uint8_t write_reg[ 3 ];
    
    write_reg[ 0 ] = TEMPHUM11_REG_CONFIGURATION;
    write_reg[ 1 ] = ( uint8_t )( config >> 8 );
    write_reg[ 2 ] = ( uint8_t )( config );  

    SERCOM0_I2C_Write(TEMPHUM11_DEVICE_SLAVE_ADDR, write_reg, 3);
    while (SERCOM0_I2C_IsBusy()) 
        ;
}

uint16_t temphum11_read_data ( uint8_t reg )
{
    uint8_t write_reg[ 1 ];
    uint8_t read_reg[ 2 ] = { 0xff, 0xee };
    uint16_t read_data = 0;

    write_reg[ 0 ] = reg;

    SERCOM0_I2C_Write(TEMPHUM11_DEVICE_SLAVE_ADDR, write_reg, 1);
    while (SERCOM0_I2C_IsBusy()) 
        ;
    
    SYSTICK_DelayMs(10);
    
    SERCOM0_I2C_Read(TEMPHUM11_DEVICE_SLAVE_ADDR, read_reg, 2);
    while (SERCOM0_I2C_IsBusy()) 
        ;

    read_data = read_reg[ 0 ];
    read_data = read_data << 8 ;
    read_data = read_data | read_reg[ 1 ];
    
    return read_data;
}

float temphum11_get_temperature ( uint8_t temp_in )
{
    uint16_t temp_out = 0;
    float temperature;
    
    temp_out = temphum11_read_data( TEMPHUM11_REG_TEMPERATURE );
    temperature = ( float )( temp_out / 65536.0 ) * 165.0 - 40;

    if ( temp_in == TEMPHUM11_TEMP_IN_KELVIN )
    {
        temperature = temperature + 273.15;
    }
    else if ( temp_in == TEMPHUM11_TEMP_IN_FAHRENHEIT )
    {
        temperature = ( temperature * 9.0/5.0 ) + 32;
    }
    
    return temperature;
}

float temphum11_get_humidity ( )
{
    uint16_t hum_out = 0;
    float humidity;

    hum_out = temphum11_read_data( TEMPHUM11_REG_HUMIDITY );
    humidity = ( float )( hum_out / 65536.0 ) * 100.0;
    
    return humidity;
}

// ----------------------------------------------- PRIVATE FUNCTION DEFINITIONS



// ------------------------------------------------------------------------- END

