/* ************************************************************************** */
/** Descriptive File Name

  @Company
    Company Name

  @File Name
    filename.h

  @Summary
    Brief description of the file.

  @Description
    Describe the purpose of this file.
 */
/* ************************************************************************** */

#ifndef _BLUESMIRF_H    /* Guard against multiple inclusion */
#define _BLUESMIRF_H


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */

/* This section lists the other files that are included in this file.
 */

typedef enum 
{
  Command_None = 0,
  Command_Open = 1,
  Command_Close = 2,
} CommandEnum;

class BlueSmirf
{
public:
    BlueSmirf();

    void init();
    bool update();

    void setAppStatus(int status);
    void setSwitches(bool fan, bool cell);
    void setTemperature(float temp);
    void setHumidity(int hum);

    bool connected();
    CommandEnum command();
    float temperatureSetpoint() { return (float)_tempSetpoint / 10.0; }

    bool manual(){ return _manual; }
    void setFan(bool on) { _fan = on; }
    bool fan() { return _fan; }
    bool fanOn() { return _fanCommand; }
    void setCell(bool on) { _cell = on; }
    bool cell() { return _cell; }
    bool cellOn() { return _cellCommand; }
    void setMains(bool on) { _mains = on; }
    bool mains() { return _mains; }
    bool mainsOn() { return _mainsCommand; }
  
private:
  typedef enum {
    Proto_WaitSTX,
    Proto_WaitMode,
    Proto_WaitSetpointMSB,
    Proto_WaitSetpointLSB,
    Proto_WaitCommand,
    Proto_WaitOutput,
    Proto_WaitETX
  } ProtoStatusEnum;

  bool protoUpdate();
    
  ProtoStatusEnum _protoStatus;  
  bool _connected;
  int _temperature;
  int _tempSetpoint;
  int _tempTmp;
  int _humidity;
  bool _manual;
  bool _fan;
  bool _cell;
  bool _mains;
  CommandEnum _command;
  bool _fanCommand;
  bool _cellCommand;
  bool _mainsCommand;
  int _appStatus;
  unsigned long _lastMsgMs;
};

#endif /* _BLUESMIRF_H */

/* *****************************************************************************
 End of File
 */
