#include "bluesmirf.h"
#include "definitions.h"

BlueSmirf::BlueSmirf()
{
  _protoStatus = Proto_WaitSTX;
  _command = Command_None;
  _manual = false;
  _fan = false;
  _cell = false;
  _mains = false;
  _appStatus = 0;
  _lastMsgMs = 0;
  _connected = false;
  _tempSetpoint = 50; // 5°C
}
        
void BlueSmirf::init()
{
    SERCOM3_USART_Write((uint8_t*)"$$$", 3);
    SYSTICK_DelayMs(100);
    
    SERCOM3_USART_Write((uint8_t*)"SM,0\r", 5);
    SYSTICK_DelayMs(100);
    
    SERCOM3_USART_Write((uint8_t*)"---\r", 4);
    SYSTICK_DelayMs(100);
}

bool BlueSmirf::update()
{
  bool newMessage = false;
  
  while (SERCOM3_USART_ReadCountGet())
  {
    newMessage |= protoUpdate();
  }

  if (!newMessage)
  {
    if (_lastMsgMs == 0)
      return false;

    unsigned long delta = SYSTICK_GetTickCounter() - _lastMsgMs;
    if (delta > 5000)
    {
      init();
        
      _command = Command_None;
      _connected = false;

      _lastMsgMs = 0;
    }

    return false;
  }

  _lastMsgMs = SYSTICK_GetTickCounter();
  _connected = true;

  uint8_t tmp;
  
  SERCOM3_USART_Write((uint8_t*)"$", 1);
  
  tmp = (uint8_t)_appStatus;
  SERCOM3_USART_Write(&tmp, 1);

  tmp = (uint8_t)_temperature >> 8;
  SERCOM3_USART_Write(&tmp, 1);

  tmp = (uint8_t)_temperature & 0xff;
  SERCOM3_USART_Write(&tmp, 1);

  tmp = (uint8_t)_humidity;
  SERCOM3_USART_Write(&tmp, 1);

  tmp = 0x00;
  if (_fan) tmp |= 0x01;
  if (_cell) tmp |= 0x02;
  if (_mains) tmp |= 0x04;
              
  SERCOM3_USART_Write(&tmp, 1);

  SERCOM3_USART_Write((uint8_t*)'#', 1);

  return true;
}

void BlueSmirf::setSwitches(bool fan, bool cell)
{
  _fan = fan;
  _cell = cell;
}

void BlueSmirf::setTemperature(float temp)
{
    _temperature = (int)(temp * 10);
}
void BlueSmirf::setHumidity(int hum)
{
    _humidity = hum;
}

void BlueSmirf::setAppStatus(int status)
{
  _appStatus = status;
}

bool BlueSmirf::connected()
{
  return _connected;
}

CommandEnum BlueSmirf::command()
{
  CommandEnum tmp = _command;
  _command = Command_None;
  
  return tmp;
}

bool BlueSmirf::protoUpdate()
{
  bool newMessage = false;
  uint8_t c;
  
  SERCOM3_USART_Read(&c, 1);

  switch (_protoStatus)
  {
    case Proto_WaitSTX:
      if (c == '$')
        _protoStatus = Proto_WaitMode;
      break;
    case Proto_WaitMode:
      _manual = (c != 0);
      _protoStatus = Proto_WaitCommand;
      break;
    case Proto_WaitCommand:
      _command = (CommandEnum)c;
      _protoStatus = Proto_WaitSetpointMSB;
      break;
    case Proto_WaitSetpointMSB:
      _tempTmp = c;
      _protoStatus = Proto_WaitSetpointLSB;
      break;
    case Proto_WaitSetpointLSB:
      _tempTmp = (_tempTmp << 8) | c;
      _tempSetpoint = _tempTmp;
      _protoStatus = Proto_WaitOutput;
      break;
    case Proto_WaitOutput:
      _fanCommand = ((c & 0x01) != 0);
      _cellCommand = ((c & 0x02) != 0);
      _mainsCommand = ((c & 0x04) != 0);
      _protoStatus = Proto_WaitETX;
      break;
    case Proto_WaitETX:
      newMessage = true;
      _protoStatus = Proto_WaitSTX;
      break;
  }

  return newMessage;
}
