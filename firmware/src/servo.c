/*
 */

/*!
 * \file
 *
 */

#include "servo.h"
#include "definitions.h"

/**
 * @brief Click ctx object definition.
 */
typedef struct
{

    // ctx variable 

    uint8_t slave_address_of_pca9685;
    uint8_t slave_address_of_ltc2497;

    uint8_t min_pos;
    uint8_t max_pos;
    uint16_t vref;
    uint16_t low_res;
    uint16_t high_res;

    servo_pos_and_res_t pos_and_res;

} servo_t;

static servo_t servo_ctx;

// ---------------------------------------------- PRIVATE FUNCTION DECLARATIONS 

static uint16_t map_priv ( servo_map_t map );

// ------------------------------------------------ PUBLIC FUNCTION DEFINITIONS

SERVO_RETVAL servo_init ( )
{
    servo_ctx.slave_address_of_pca9685 = 0x40;
    servo_ctx.slave_address_of_ltc2497 = 0x14;

    return SERVO_OK;
}

void servo_default_cfg ( )
{
    servo_pos_and_res_t pos_and_res;

    pos_and_res.min_position = 0;
    pos_and_res.max_position = 180;
    pos_and_res.low_resolution = SERVO_DEFAULT_LOW_RESOLUTION;
    pos_and_res.high_resolution = SERVO_DEFAULT_HIGH_RESOLUTION;
    
    servo_setting( pos_and_res ); 
    servo_set_vref( SERVO_VREF_3300 );
    servo_set_mode( SERVO_REG_MODE_1, SERVO_MODE1_RESTART_ENABLE | SERVO_MODE1_USE_ALL_CALL_ADR );
    servo_sleep( );
    servo_set_freq( 30 );
    servo_set_mode( SERVO_REG_MODE_1, SERVO_MODE1_RESTART_ENABLE | SERVO_MODE1_AUTO_INCREMENT_ENABLE | SERVO_MODE1_USE_ALL_CALL_ADR );
}

void servo_generic_write_of_pca9685 ( uint8_t reg, uint8_t *data_buf, uint8_t len )
{
    uint8_t tx_buf[ 256 ];
    uint8_t cnt;
    
    tx_buf[ 0 ] = reg;
    
    for ( cnt = 1; cnt <= len; cnt++ )
    {
        tx_buf[ cnt ] = data_buf[ cnt - 1 ]; 
    }
    
    SERCOM2_I2C_Write(servo_ctx.slave_address_of_pca9685, tx_buf, len + 1);
    while (SERCOM2_I2C_IsBusy())    
        ;
}

void servo_generic_write_of_ltc2497 ( uint8_t reg, uint8_t *data_buf, uint8_t len )
{
    uint8_t tx_buf[ 256 ];
    uint8_t cnt;
    
    tx_buf[ 0 ] = reg;
    
    for ( cnt = 1; cnt <= len; cnt++ )
    {
        tx_buf[ cnt ] = data_buf[ cnt - 1 ]; 
    }
    
    SERCOM2_I2C_Write(servo_ctx.slave_address_of_ltc2497, tx_buf, len + 1);
    while (SERCOM2_I2C_IsBusy())    
        ;
}

void servo_generic_read_of_pca9685 ( uint8_t reg, uint8_t *data_buf, uint8_t len )
{  
    SERCOM2_I2C_WriteRead(servo_ctx.slave_address_of_pca9685, &reg, 1, data_buf, len);
    while (SERCOM2_I2C_IsBusy())    
        ;
}

void servo_generic_read_of_ltc2497 ( uint8_t reg, uint8_t *data_buf, uint8_t len )
{  
    SERCOM2_I2C_WriteRead(servo_ctx.slave_address_of_ltc2497, &reg, 1, data_buf, len);
    while (SERCOM2_I2C_IsBusy())    
        ;
}

void servo_setting ( servo_pos_and_res_t pos_and_res )
{
    servo_ctx.min_pos = pos_and_res.min_position;
    servo_ctx.max_pos = pos_and_res.max_position;
    servo_ctx.low_res  = pos_and_res.low_resolution & 0x0FFF;
    servo_ctx.high_res = pos_and_res.high_resolution & 0x0FFF;
}

void servo_set_vref ( uint16_t v_ref )
{
    servo_ctx.vref = v_ref;
}

void servo_stop ( )
{
    SERVO_OE_Set();
}

void servo_start ( )
{
    SERVO_OE_Clear();
}

void servo_soft_reset ( )
{
    uint8_t write_reg[ 1 ];

    servo_generic_write_of_pca9685( SERVO_SOFT_RESET, write_reg, 0 );
    
    SYSTICK_DelayMs(100);
}

void servo_sleep (  )
{
    servo_set_mode( SERVO_REG_MODE_1,
                   SERVO_MODE1_RESTART_ENABLE |
                   SERVO_MODE1_EXTCLK_PIN_CLOCK |
                   SERVO_MODE1_LOW_POWER_MODE |
                   SERVO_MODE1_AUTO_INCREMENT_ENABLE |
                   SERVO_MODE1_USE_ALL_CALL_ADR );
}

void servo_set_mode ( uint8_t mode, uint8_t w_data )
{    
    servo_start( );
    servo_generic_write_of_pca9685( mode, &w_data, 1 );
    SYSTICK_DelayMs(100);
}

void servo_set_position ( uint8_t motor, uint8_t position )
{
    uint8_t write_reg[ 4 ];
    uint16_t set_map;
    uint16_t on = 0x0000;
    servo_map_t map; 
    
    map.x = position;
    map.in_min = servo_ctx.min_pos ;
    map.in_max = servo_ctx.max_pos;
    map.out_min = servo_ctx.low_res;
    map.out_max = servo_ctx.high_res;

    set_map = map_priv( map ) ;
    if ( set_map < 70 )
    {
        set_map = 70;
    }
        
    write_reg[ 0 ] = on;
    write_reg[ 1 ] = on >> 8;
    write_reg[ 2 ] = set_map;
    write_reg[ 3 ] = set_map >> 8;

    servo_start( );
    servo_generic_write_of_pca9685( motor, write_reg, 4 );
}

void servo_set_freq ( uint16_t freq )
{
    uint32_t prescale_val;
    uint8_t write_buf[ 1 ];
    
    prescale_val = 25000000;
    prescale_val /= 4096;
    prescale_val /= freq;
    prescale_val -= 1;
    
    write_buf[ 0 ] = prescale_val;
    
    servo_start( );
    servo_generic_write_of_pca9685( SERVO_REG_PRE_SCALE, write_buf, 1 );

    SYSTICK_DelayMs(100);
}

uint32_t servo_get_channel ( uint8_t channel )
{
    uint8_t read_reg[ 3 ];
    uint32_t adc_value;

    servo_start( );
    servo_generic_read_of_ltc2497( channel, read_reg, 3 );
    
    adc_value = read_reg[ 0 ];
    adc_value = adc_value << 8;
    adc_value = adc_value | read_reg[ 1 ];
    adc_value = adc_value << 8;
    adc_value = adc_value | read_reg[ 2 ];
    
    adc_value = adc_value & 0x00FFFFC0;
    adc_value = adc_value >> 5;

    return adc_value;
}

uint16_t setvo_get_current ( uint8_t channel )
{
    uint32_t adc_value;
    uint16_t current;

    adc_value = servo_get_channel( channel );
    adc_value = adc_value & 0x00003FFF;
    current = ( adc_value * servo_ctx.vref ) / 65535;
    
    return  current;
}

// ----------------------------------------------- PRIVATE FUNCTION DEFINITIONS

static uint16_t map_priv ( servo_map_t map )
{
    uint16_t val;

    val = ( map.x - map.in_min ) * ( map.out_max - map.out_min ) / ( map.in_max - map.in_min );
    val = val + map.out_min + 10 ;

    return val;
}

// ------------------------------------------------------------------------- END

