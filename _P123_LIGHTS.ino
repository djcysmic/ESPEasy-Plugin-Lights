//#######################################################################################################
//#################################### Plugin 123: Lights ###############################################
//#######################################################################################################

/********************************************************************************************************
 * Copyright:              2016, ddtlabs (aka dev0)
 * repository home:        https://github.com/ddtlabs/ESPEasy-Plugin-Lights
 * contact via FHEM Forum: https://forum.fhem.de/index.php?action=profile;u=7465
 /******************************************************************************************************/

// set L pct 0; set L rgb 000000; sleep 0.5; set L rgb ff0000; sleep 0.5; set L rgb 00ff00; sleep 0.5; set L rgb 0000ff; sleep 0.5; set L rgb 111111; sleep 0.5; set L rgb f7f7f7; sleep 0.5; set L rgb ffffff; set L ct 4000; set L pct 100; sleep 0.5; set L ct 2000; sleep 0.5; set L ct 6000; sleep 0.5; set L ct 4000; sleep 0.5; set L pct 50; sleep 0.5; set L pct 0

#define PLUGIN_123
#define PLUGIN_ID_123         123
#define PLUGIN_NAME_123       "Lights"
#define PLUGIN_VALUENAME1_123 "rgb"
#define PLUGIN_VALUENAME2_123 "ct"
#define PLUGIN_VALUENAME3_123 "pct"
#define PLUGIN_VALUENAME4_123 "colormode"

String  Plugin_123_version = "1.00";

#include <Ticker.h>

boolean Plugin_123_init = false;

// Flags to trigger actions
boolean Plugin_123_debug                    = false;
boolean Plugin_123_triggerSendData          = false;
boolean Plugin_123_triggerSendDataAfterBoot = false;
boolean Plugin_123_tempOff                  = false;

// Fading timer settings
Ticker  Plugin_123_Ticker;
int     Plugin_123_FadingRate        = 50; //Hz
float   Plugin_123_defaultFadingTime = 0;

struct _structLightParam {
  String  rgbStr    = "7f7f7f";
  int     ct        = 3000;
  int     pct       = 50;
  byte    colorMode = 2;  // 1=rgb 2=ct
  float   fading    = 1;
  boolean state     = 0;  // 0=off 1=on
} _lightParam;

struct _structPins {
  int PinNo                 = 0;
  int CurrentLevel          = 0;
  int FadingTargetLevel     = 0;
  int FadingTargetTmp       = 0;
  int FadingMMillisPerStep  = 0;
  int FadingDirection       = 0;
  unsigned long FadingTimer = 0;
} _pins[5];

struct _structOptions {
  boolean rgb_enabled       = 0;
  boolean ww_enabled        = 0;
  boolean cw_enabled        = 0;
  boolean maxBri_enabled    = 0;
  boolean sendData_enabled  = 0;
  int     wwTemp            = 2000;
  int     cwTemp            = 6000;
} _options;


boolean Plugin_123(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {

   case PLUGIN_DEVICE_ADD:            // ------------------------------------------->
      {
        Device[++deviceCount].Number = PLUGIN_ID_123;

        // DEVICE TYPE
        //Device[deviceCount].Type = DEVICE_TYPE_ANALOG;
        //Device[deviceCount].Type = DEVICE_TYPE_SINGLE;
        Device[deviceCount].Type = DEVICE_TYPE_DUMMY;

        // SENSOR VTYPE
        //Device[deviceCount].VType = SENSOR_TYPE_SINGLE;
        //Device[deviceCount].VType = SENSOR_TYPE_DUAL;
        //Device[deviceCount].VType = SENSOR_TYPE_TRIPLE;
        Device[deviceCount].VType = SENSOR_TYPE_QUAD;
//        Device[deviceCount].VType = SENSOR_TYPE_HEXA;
        //Device[deviceCount].VType = SENSOR_TYPE_TEMP_BARO;
        //Device[deviceCount].VType = SENSOR_TYPE_DIMMER;

        Device[deviceCount].Custom = false;  //removes some items from html web page. more?
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = false;
        Device[deviceCount].ValueCount = 4;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].TimerOptional = true;
        Device[deviceCount].GlobalSyncOption = true;
        Device[deviceCount].DecimalsOnly = false;
        break;
      }

    case PLUGIN_GET_DEVICENAME:       // ------------------------------------------->
      {
        string = F(PLUGIN_NAME_123);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES: // ------------------------------------------->
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_123));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_123));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_123));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[3], PSTR(PLUGIN_VALUENAME4_123));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:         // ------------------------------------------->
      {
        char tmpString[128];

        string += F("<TR><TD><hr>Lights Plugin V.");
        string += Plugin_123_version;
        string += F("<hr></TD><TD><hr>Check that Gpios are not used elsewhere (plugins or hardware tab)<hr><TD>");
        
        string += F("<TR><TD>Enable RGB Channels:<TD>");
        if (Settings.TaskDevicePluginConfigFloat[event->TaskIndex][1]) {
          string += F("<input type=checkbox name=plugin_123_enableRGB checked>");
          sprintf_P(tmpString, PSTR("<TR><TD>Red Gpio:<TD><input type='text' name='plugin_123_RedPin' value='%u'>"), Settings.TaskDevicePluginConfig[event->TaskIndex][0]);
          string += tmpString;
          sprintf_P(tmpString, PSTR("<TR><TD>Green Gpio:<TD><input type='text' name='plugin_123_GreenPin' value='%u'>"), Settings.TaskDevicePluginConfig[event->TaskIndex][1]);
          string += tmpString;
          sprintf_P(tmpString, PSTR("<TR><TD>Blue Gpio:<TD><input type='text' name='plugin_123_BluePin' value='%u'>"), Settings.TaskDevicePluginConfig[event->TaskIndex][2]);
          string += tmpString;
        }
        else
          string += F("<input type=checkbox name=plugin_123_enableRGB>");

        string += F("<TR><TD>Enable WW Channel:<TD>");
        if (Settings.TaskDevicePluginConfigFloat[event->TaskIndex][2]) {
          string += F("<input type=checkbox name=plugin_123_enableWW checked>");
          sprintf_P(tmpString, PSTR("<TR><TD>WW Gpio:<TD><input type='text' name='plugin_123_WWPin' value='%u'>"), Settings.TaskDevicePluginConfig[event->TaskIndex][3]);
          string += tmpString;
          sprintf_P(tmpString, PSTR("<TR><TD>WW Color Temp (Kelvin):<TD><input type='text' name='plugin_123_WWTemp' value='%u'> (default: 2000)"), Settings.TaskDevicePluginConfig[event->TaskIndex][5]);
          string += tmpString;
        }
        else {
          string += F("<input type=checkbox name=plugin_123_enableWW>");
        }

        string += F("<TR><TD>Enable CW Channel:<TD>");
        if (Settings.TaskDevicePluginConfigFloat[event->TaskIndex][3]) {
          string += F("<input type=checkbox name=plugin_123_enableCW checked>");
          sprintf_P(tmpString, PSTR("<TR><TD>CW Gpio:<TD><input type='text' name='plugin_123_CWPin' value='%u'>"), Settings.TaskDevicePluginConfig[event->TaskIndex][4]);
          string += tmpString;
          sprintf_P(tmpString, PSTR("<TR><TD>CW Color Temp (Kelvin):<TD><input type='text' name='plugin_123_CWTemp' value='%u'> (default: 6000)"), Settings.TaskDevicePluginConfig[event->TaskIndex][6]);
          string += tmpString;
        }
        else
          string += F("<input type=checkbox name=plugin_123_enableCW>");

        if (Settings.TaskDevicePluginConfigFloat[event->TaskIndex][2]        //WW+CW active
            && Settings.TaskDevicePluginConfigFloat[event->TaskIndex][3])   
        { 
          string += F("<TR><TD>Maximize White Brightness:<TD>");
          if (Settings.TaskDevicePluginConfigLong[event->TaskIndex][0])
            string += F("<input type=checkbox name=plugin_123_maxBri checked>");
          else
            string += F("<input type=checkbox name=plugin_123_maxBri>");
        }

          sprintf_P(tmpString, PSTR("<TR><TD>Pon Brightness (%):<TD><input type='text' name='plugin_123_ponVal' value='%u'>"), Settings.TaskDevicePluginConfig[event->TaskIndex][7]);
          string += tmpString;
          

//          sprintf_P(tmpString, PSTR("<TR><TD>Default Fading Time (s):<TD><input type='text' name='plugin_123_fadingTime' value='%u'>"), Settings.TaskDevicePluginConfigFloat[event->TaskIndex][0]);
//          //sprintf_P(tmpString, PSTR("<TR><TD>Default Fading Time (s):<TD><input type='text' name='plugin_123_fadingTime' value='%u'>"), Settings.TaskDevicePluginConfigFloat[event->TaskIndex][0]);
//          string += tmpString;
        string += F("<TR><TD>Default Fading Time (s):<TD><input type='text' name='plugin_123_fadingTime' value='");
        string += Settings.TaskDevicePluginConfigFloat[event->TaskIndex][0];
        string += F("'> (-1 =&gt; disable fading timer)");

        string += F("<TR><TD>Send Boot State:<TD>");
         if (Settings.TaskDevicePluginConfigLong[event->TaskIndex][1])
           string += F("<input type=checkbox name=plugin_123_sendBootState checked>");
        else
          string += F("<input type=checkbox name=plugin_123_sendBootState>");

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:         // ------------------------------------------->
      {
        String plugin1 = WebServer.arg("plugin_123_RedPin");
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = plugin1.toInt();
        String plugin2 = WebServer.arg("plugin_123_GreenPin");
        Settings.TaskDevicePluginConfig[event->TaskIndex][1] = plugin2.toInt();
        String plugin3 = WebServer.arg("plugin_123_BluePin");
        Settings.TaskDevicePluginConfig[event->TaskIndex][2] = plugin3.toInt();
        String plugin4 = WebServer.arg("plugin_123_WWPin");
        Settings.TaskDevicePluginConfig[event->TaskIndex][3] = plugin4.toInt();
        String plugin5 = WebServer.arg("plugin_123_CWPin");
        Settings.TaskDevicePluginConfig[event->TaskIndex][4] = plugin5.toInt();
        String plugin6 = WebServer.arg("plugin_123_WWTemp");
        Settings.TaskDevicePluginConfig[event->TaskIndex][5] = plugin6.toInt();
        String plugin7 = WebServer.arg("plugin_123_CWTemp");
        Settings.TaskDevicePluginConfig[event->TaskIndex][6] = plugin7.toInt();
        String plugin13 = WebServer.arg("plugin_123_ponVal");
        Settings.TaskDevicePluginConfig[event->TaskIndex][7] = plugin13.toInt();
        String plugin15 = WebServer.arg("plugin_123_fadingTime");
        Settings.TaskDevicePluginConfigFloat[event->TaskIndex][0] = plugin15.toFloat();

        String plugin8 = WebServer.arg("plugin_123_enableRGB");
        Settings.TaskDevicePluginConfigFloat[event->TaskIndex][1] = (plugin8 == "on");
        String plugin9 = WebServer.arg("plugin_123_enableWW");
        Settings.TaskDevicePluginConfigFloat[event->TaskIndex][2] = (plugin9 == "on");
        String plugin10 = WebServer.arg("plugin_123_enableCW");
        Settings.TaskDevicePluginConfigFloat[event->TaskIndex][3] = (plugin10 == "on");
        String plugin11 = WebServer.arg("plugin_123_maxBri");
        Settings.TaskDevicePluginConfigLong[event->TaskIndex][0] = (plugin11 == "on");
        String plugin14 = WebServer.arg("plugin_123_sendBootState");
        Settings.TaskDevicePluginConfigLong[event->TaskIndex][1] = (plugin14 == "on");

        // SaveTaskSettings(event->TaskIndex);

        success = true;
        break;
      }

    case PLUGIN_INIT:                 // ------------------------------------------->
      {
        LoadTaskSettings(event->TaskIndex);

        _options.rgb_enabled      = Settings.TaskDevicePluginConfigFloat[event->TaskIndex][1];
        _options.ww_enabled       = Settings.TaskDevicePluginConfigFloat[event->TaskIndex][2];
        _options.cw_enabled       = Settings.TaskDevicePluginConfigFloat[event->TaskIndex][3];
        _options.maxBri_enabled   = Settings.TaskDevicePluginConfigLong[event->TaskIndex][0];
        _options.sendData_enabled = Settings.TaskDeviceSendData[event->TaskIndex];
        if (Settings.TaskDevicePluginConfig[event->TaskIndex][5] > 0)
          _options.wwTemp = Settings.TaskDevicePluginConfig[event->TaskIndex][5];
        if (Settings.TaskDevicePluginConfig[event->TaskIndex][6] > 0)
          _options.cwTemp = Settings.TaskDevicePluginConfig[event->TaskIndex][6];
        Plugin_123_defaultFadingTime = Settings.TaskDevicePluginConfigFloat[event->TaskIndex][0];

        String log = F("INIT : Lights [");

        if (_options.rgb_enabled) {
          for (int PinIndex = 0; PinIndex < 3; PinIndex++) {
            _pins[PinIndex].PinNo = ExtraTaskSettings.TaskDevicePluginConfigLong[PinIndex];
            pinMode(_pins[PinIndex].PinNo, OUTPUT);
          }
          log += F("RGB ");
        } 
        else { log += F("noRGB "); }
        
        if (_options.ww_enabled) {
          _pins[3].PinNo = Settings.TaskDevicePluginConfig[event->TaskIndex][3];
          log += F("WW ");
        } 
        else { log += F("noWW "); }

        if (_options.cw_enabled) {
          _pins[4].PinNo = Settings.TaskDevicePluginConfig[event->TaskIndex][4];
          log += F("CW ");
        }
        else { log += F("noCW "); }

        int ponVal = Settings.TaskDevicePluginConfig[event->TaskIndex][7];

        // disable Timer if defaultFadingTime < 0
        if (Plugin_123_defaultFadingTime >= 0) {
          log += F("FADING ");
          Plugin_123_Ticker.attach_ms(20, Plugin_123_FadingTimer);
        }
        else { log += F("noFADING "); }

        log += (_options.maxBri_enabled) ? F("MAXBRI ") : F("CONSTBRI ");

        if (ponVal >= 0 && ponVal <= 100) {
          _lightParam.pct = ponVal;
          _lightParam.state = 1;
          _lightParam.fading = Plugin_123_defaultFadingTime;
          Plugin_123_setLightParams("ct");
          Plugin_123_setPins_Initiate();
          log += F("PON]");
        } 
        else { log += F("noPON]"); }
        
        addLog(LOG_LEVEL_INFO, log);
        
        Plugin_123_init = true;
        success = true;
        if (Settings.TaskDevicePluginConfigLong[event->TaskIndex][1]) Plugin_123_triggerSendDataAfterBoot = true;
        break;
      }

    case PLUGIN_TEN_PER_SECOND:       // ------------------------------------------->
      {
        // send Values via controller plugin
        if (Plugin_123_triggerSendData || Plugin_123_triggerSendDataAfterBoot)
        {
          Plugin_123_triggerSendData = false;
          Plugin_123_triggerSendDataAfterBoot = false;
          UserVar[event->BaseVarIndex] = Plugin_123_rgbStr2Num(_lightParam.rgbStr);
          UserVar[event->BaseVarIndex + 1] = _lightParam.ct;
          UserVar[event->BaseVarIndex + 2] = _lightParam.pct;
          UserVar[event->BaseVarIndex + 3] = _lightParam.colorMode;
          sendData(event);
        }
      success = true;
      break;
      }

    case PLUGIN_READ:                 // ------------------------------------------->
      {
        // there is no need to read them, just use current values
        UserVar[event->BaseVarIndex] = Plugin_123_rgbStr2Num(_lightParam.rgbStr);
        UserVar[event->BaseVarIndex + 1] = _lightParam.ct;
        UserVar[event->BaseVarIndex + 2] = _lightParam.pct;
        UserVar[event->BaseVarIndex + 3] = _lightParam.colorMode;
        String log;
        log = F("Lights: rgb: ");
        log += Plugin_123_rgbStr2Num(_lightParam.rgbStr);
        log += F(" ct: ");
        log += (int)UserVar[event->BaseVarIndex + 1];
        log += F(" pct: ");
        log += (int)UserVar[event->BaseVarIndex + 2];
        log += F(" cm: ");
        log += (int)UserVar[event->BaseVarIndex + 3];
        addLog(LOG_LEVEL_INFO, log);

        success = true;
        break;
      }

    case PLUGIN_WRITE:                // ------------------------------------------->
      {
        String log = "";
        String command = parseString(string, 1);

        if (command == F("lights"))
        {
          success = true;
          String subCommand = parseString(string, 2);


          if (subCommand == F("rgb"))
          {
            _lightParam.rgbStr = parseString(string, 3);         // rgb string (eg. 11FFAA)
            _lightParam.fading = (parseString(string, 4) == "")  // transiotion time
              ? Plugin_123_defaultFadingTime 
              : parseString(string, 4).toFloat();
            Plugin_123_setCurrentLevelToZeroIfOff();
            Plugin_123_setLightParams("rgb");
            _lightParam.state = 1;
            Plugin_123_setPins_Initiate();
          }

 
          else if (subCommand == F("ct"))
          {
            _lightParam.ct     = event->Par2;                    // color temp val (eg. 3200)
            _lightParam.fading = (parseString(string, 4) == "")  // transition time
              ? Plugin_123_defaultFadingTime 
              : parseString(string, 4).toFloat();
            if (parseString(string, 5) != "")  // pct
              _lightParam.pct = parseString(string, 5).toInt();
            Plugin_123_setCurrentLevelToZeroIfOff();
            Plugin_123_setLightParams("ct");
            _lightParam.state = 1;
            Plugin_123_setPins_Initiate();
          }


          else if (subCommand == F("pct")) {
            _lightParam.pct    = event->Par2;                    // white brightness in %
            _lightParam.fading = (parseString(string, 4) == "")  // transition time
              ? Plugin_123_defaultFadingTime 
              : parseString(string, 4).toFloat();
            if (parseString(string, 5) != "")  // ct
              _lightParam.ct = parseString(string, 5).toInt();
            Plugin_123_setCurrentLevelToZeroIfOff();
            Plugin_123_setLightParams("ct");
            _lightParam.state = 1;
            Plugin_123_setPins_Initiate();
          }


          else if (subCommand == F("toggle")) {
            subCommand = (_lightParam.state == 1) ? "off" : "on";
          }


          if (subCommand == F("on")) {
            _lightParam.fading = (parseString(string, 3) == "") 
              ? Plugin_123_defaultFadingTime 
              : parseString(string, 3).toFloat();
            Plugin_123_setCurrentLevelToZeroIfOff();
            if      (_lightParam.colorMode == 1) { Plugin_123_setLightParams("rgb"); }
            else if (_lightParam.colorMode == 2) { Plugin_123_setLightParams("ct");  }
            _lightParam.state = 1;
            Plugin_123_setPins_Initiate();
          }


          if (subCommand == F("off")) {
            _lightParam.fading = (parseString(string, 3) == "") 
              ? Plugin_123_defaultFadingTime 
              : parseString(string, 3).toFloat();

            // Save current FadingTargetLevel, will be restored in Plugin_123_setPins_Finish()
            for (int PinIndex = 0; PinIndex < 5; PinIndex++) {
              _pins[PinIndex].FadingTargetTmp = _pins[PinIndex].FadingTargetLevel;
              _pins[PinIndex].FadingTargetLevel = 0;
            }
            boolean Plugin_123_tempOff = true;
            _lightParam.state = 0;
            Plugin_123_setPins_Initiate();
          }


          else if ( subCommand != F("rgb") && subCommand != F("ct") 
                 && subCommand != F("pct") && subCommand != F("toggle") 
                 && subCommand != F("on")  && subCommand != F("off") )
          {
            log = F("Lights: unknown subcommand: ");
            log += subCommand;
            addLog(LOG_LEVEL_INFO, log);

            String json;
            printToWebJSON = true;
            json += F("{\n");
            json += F("\"plugin\": \"123\",\n");
            json += F("\"log\": \"");
            json += F("Lights: unknown command: ");
            json += subCommand;
            json += F("\"\n");
            json += F("}\n");
            SendStatus(event->Source, json); // send http response to controller (JSON fmormat)
            break;
          }
          

        } // command lights

        Plugin_123_SendStatus(event->Source);
        if (Plugin_123_debug) Plugin_123_dumpValues();

      } //case PLUGIN_WRITE

  }
  return success;
}


// ---------------------------------------------------------------------------------
// ------------------------------ setLightParams -----------------------------------
// ---------------------------------------------------------------------------------
void Plugin_123_setLightParams(String cmd)
{
  if (cmd == F("rgb"))
  {
    _lightParam.colorMode = 1;
    int _white_;

    // http://stackoverflow.com/questions/23576827/arduino-convert-a-sting-hex-ffffff-into-3-int
    int32_t rgbDec = Plugin_123_rgbStr2Num(_lightParam.rgbStr);
    if (rgbDec > 16777215) rgbDec = 16777215;
    // Split them up into r, g, b values
    _pins[0].FadingTargetLevel = (rgbDec >> 16);
    _pins[1].FadingTargetLevel = (rgbDec >> 8 & 0xFF);
    _pins[2].FadingTargetLevel = (rgbDec & 0xFF);

    if (_options.ww_enabled || _options.cw_enabled) {
      _white_ = Plugin_123_rgb2WhitePortion(_pins[0].FadingTargetLevel, _pins[1].FadingTargetLevel, _pins[2].FadingTargetLevel, 255);
      _pins[0].FadingTargetLevel -= _white_;
      _pins[1].FadingTargetLevel -= _white_;
      _pins[2].FadingTargetLevel -= _white_;
    }

    // todo: split w ->ww/cw with ct and use ww/cw in setPins()

    if (_options.ww_enabled && _options.cw_enabled) {
      if (!_options.maxBri_enabled) _white_ /= 2;
    }
    _pins[0].FadingTargetLevel *=4;
    _pins[1].FadingTargetLevel *=4;
    _pins[2].FadingTargetLevel *=4;
    _pins[3].FadingTargetLevel = _white_ *4;
    _pins[4].FadingTargetLevel = _white_ *4;
  } // rgb

  // --- ct ww cw --------------------------------------------------------------------
  else if (cmd == F("ct") && _options.ww_enabled && _options.cw_enabled)
  {
     if (Plugin_123_debug) Serial.println(F("Lights: setLightsParam: ct ww cw "));

    _lightParam.colorMode = 2;

    // fail-safe
    if (_lightParam.pct < 0)        _lightParam.pct = 0;
    else if (_lightParam.pct > 100) _lightParam.pct = 100;

    // switch rgb lights off
    for (int PinIndex = 0; PinIndex < 3; PinIndex++) {
      _pins[PinIndex].FadingTargetLevel = 0;
    }
   
    float wwTemp = _options.wwTemp;
    float cwTemp = _options.cwTemp;
    
    // correct ct values if out of range
    if (_lightParam.ct < wwTemp) {_lightParam.ct = wwTemp;}
    if (_lightParam.ct > cwTemp) {_lightParam.ct = cwTemp;}
   
    //maxBri !constant
    float temp = _lightParam.ct;
    
    cwTemp -= wwTemp;
    temp   -= wwTemp;
    float cwFactor = temp / cwTemp;
    float wwFactor = 1 - cwFactor;

    float maxBri;
    if (_options.maxBri_enabled)
    {
      if (cwFactor > wwFactor)
        maxBri = 1 / cwFactor;
      else
        maxBri = 1 / wwFactor;
    }
    else
      maxBri = 1;

    _pins[3].FadingTargetLevel = _lightParam.pct * 10.23 * wwFactor * maxBri;
    _pins[4].FadingTargetLevel = _lightParam.pct * 10.23 * cwFactor * maxBri;

    if (Plugin_123_debug) {
      Serial.print(F("Lights: wwFactor: "));
      Serial.println(wwFactor);
      Serial.print(F("Lights: cwFactor: "));
      Serial.println(cwFactor);
      Serial.print(F("Lights: maxBri: "));
      Serial.println(maxBri);
    }

  } // ct (ww/cw)

  
  // --- ct !ww !cw --------------------------------------------------------------------
  else if (cmd == F("ct") && !_options.ww_enabled && !_options.cw_enabled)
  {
    if (Plugin_123_debug) Serial.println(F("Lights: setLightsParam: ct !ww !cw "));
    _lightParam.colorMode = 2;
    Plugin_123_ct2RGB();

  } // ct !ww !cw


  // todo: adopt rgb setting ww or cw
  // --- ct ww ^ cw --------------------------------------------------------------------
  else if (cmd == F("ct") && (_options.ww_enabled ^ _options.cw_enabled))
  {
    if (Plugin_123_debug) Serial.println(F("Lights: setLightsParam: ct ww ^ cw "));
    _lightParam.colorMode = 2;
    Plugin_123_ct2RGB();
    int w = Plugin_123_rgb2WhitePortion(_pins[0].FadingTargetLevel, _pins[1].FadingTargetLevel, _pins[2].FadingTargetLevel, 1023);
      _pins[0].FadingTargetLevel -= w;
      _pins[1].FadingTargetLevel -= w;
      _pins[2].FadingTargetLevel -= w;

    if (_options.ww_enabled)
      _pins[3].FadingTargetLevel = w;
    else
      _pins[4].FadingTargetLevel = w;

  } // ct: ww ^ cw


}


// ---------------------------------------------------------------------------------
// ------------------------------ setPins_Initiate ---------------------------------
// ---------------------------------------------------------------------------------
void Plugin_123_setPins_Initiate()
{
  // todo: be more exactly calculation
  for (int PinIndex = 0; PinIndex < 5; PinIndex++) {
    if (_pins[PinIndex].FadingTargetLevel > 1019 )
      _pins[PinIndex].FadingTargetLevel = 1023;
  }
  
  // fading will be done in Plugin_123_FadingTimer (called by timer, triggered by FadingDirection != 0)
  if (_lightParam.fading > 0 && Plugin_123_defaultFadingTime >= 0) {
    for (int PinIndex = 0; PinIndex < 5; PinIndex++) {
      _pins[PinIndex].FadingMMillisPerStep = 1000 / Plugin_123_FadingRate;
      _pins[PinIndex].FadingDirection = (abs(_pins[PinIndex].FadingTargetLevel - _pins[PinIndex].CurrentLevel)) / (Plugin_123_FadingRate * _lightParam.fading / 2 );
      if (_pins[PinIndex].FadingDirection == 0)  { _pins[PinIndex].FadingDirection = 1; }
      if (_pins[PinIndex].FadingTargetLevel < _pins[PinIndex].CurrentLevel) { _pins[PinIndex].FadingDirection = _pins[PinIndex].FadingDirection * -1; }
    }
  }

  // no fading at all
  else {
    if (_options.rgb_enabled) {
      analogWrite(_pins[0].PinNo, _pins[0].FadingTargetLevel);
      analogWrite(_pins[1].PinNo, _pins[1].FadingTargetLevel);
      analogWrite(_pins[2].PinNo, _pins[2].FadingTargetLevel);
      _pins[0].CurrentLevel = _pins[0].FadingTargetLevel;
      _pins[1].CurrentLevel = _pins[1].FadingTargetLevel;
      _pins[2].CurrentLevel = _pins[2].FadingTargetLevel;
    }
    if (_options.ww_enabled) {
      analogWrite(_pins[3].PinNo, _pins[3].FadingTargetLevel);
      _pins[3].CurrentLevel = _pins[3].FadingTargetLevel;
    }
    if (_options.cw_enabled) {
      analogWrite(_pins[4].PinNo, _pins[4].FadingTargetLevel);
      _pins[4].CurrentLevel = _pins[4].FadingTargetLevel;
    }

    Plugin_123_setPins_Finish();
  }

  if (Plugin_123_debug) {
    for (int PinIndex = 0; PinIndex < 5; PinIndex++) {
      Serial.print(F("setPins: .CurrentLevel: "));          Serial.println( _pins[PinIndex].CurrentLevel);
      Serial.print(F("setPins: .FadingTargetLevel: "));     Serial.println( _pins[PinIndex].FadingTargetLevel);
      Serial.print(F("setPins: .FadingMMillisPerStep: "));  Serial.println( _pins[PinIndex].FadingMMillisPerStep);
      Serial.print(F("setPins: .FadingDirection:      "));  Serial.println( _pins[PinIndex].FadingDirection);
    }
  }
}


// ---------------------------------------------------------------------------------
// ------------------------------ setPins_Finish -----------------------------------
// ---------------------------------------------------------------------------------
void Plugin_123_setPins_Finish()
{
  if (_options.rgb_enabled) {
    setPinState(PLUGIN_ID_123, _pins[0].PinNo, PIN_MODE_PWM, _pins[0].FadingTargetLevel);
    setPinState(PLUGIN_ID_123, _pins[1].PinNo, PIN_MODE_PWM, _pins[1].FadingTargetLevel);
    setPinState(PLUGIN_ID_123, _pins[2].PinNo, PIN_MODE_PWM, _pins[2].FadingTargetLevel);
  }
  if (_options.ww_enabled) {
    setPinState(PLUGIN_ID_123, _pins[3].PinNo, PIN_MODE_PWM, _pins[3].FadingTargetLevel);
  }
  if (_options.cw_enabled) {
    setPinState(PLUGIN_ID_123, _pins[4].PinNo, PIN_MODE_PWM, _pins[4].FadingTargetLevel);
  }

  // device was switched 'off', restore old values => transmit to controller
  if (Plugin_123_tempOff) {
    for (int PinIndex = 0; PinIndex < 5; PinIndex++) {
      _pins[PinIndex].FadingTargetLevel = _pins[PinIndex].FadingTargetTmp;
    }
  }

  // send current values via controller plugin, done in PLUGIN_TEN_PER_SECOND
  if (_options.sendData_enabled)
    Plugin_123_triggerSendData = true; 
}


// ---------------------------------------------------------------------------------
// ------------------------------ FadingTimer --------------------------------------
// ---------------------------------------------------------------------------------
void Plugin_123_FadingTimer()
{
  //Fading
  for (int PinIndex = 0; PinIndex < 5; PinIndex++)
  {
    if (_pins[PinIndex].FadingDirection != 0)
    {
      if (millis() > _pins[PinIndex].FadingTimer)
      {
        _pins[PinIndex].FadingTimer = millis() + _pins[PinIndex].FadingMMillisPerStep;
        _pins[PinIndex].CurrentLevel = _pins[PinIndex].CurrentLevel + _pins[PinIndex].FadingDirection;
        if (_pins[PinIndex].CurrentLevel >= _pins[PinIndex].FadingTargetLevel && _pins[PinIndex].FadingDirection > 0)
        {
          _pins[PinIndex].FadingDirection = 0;
          _pins[PinIndex].CurrentLevel = _pins[PinIndex].FadingTargetLevel;
          if (_pins[0].FadingDirection == 0 && _pins[1].FadingDirection == 0 && _pins[2].FadingDirection == 0 && _pins[3].FadingDirection == 0 && _pins[4].FadingDirection == 0) {           
            addLog(LOG_LEVEL_INFO, "Lights: Fade up complete");
            Plugin_123_setPins_Finish();
          }
        }
        if (_pins[PinIndex].CurrentLevel <= _pins[PinIndex].FadingTargetLevel && _pins[PinIndex].FadingDirection < 0)
        {
          _pins[PinIndex].FadingDirection = 0;
          _pins[PinIndex].CurrentLevel = _pins[PinIndex].FadingTargetLevel;
          if (_pins[0].FadingDirection == 0 && _pins[1].FadingDirection == 0 && _pins[2].FadingDirection == 0 && _pins[3].FadingDirection == 0 && _pins[4].FadingDirection == 0) {
            addLog(LOG_LEVEL_INFO, "Lights: Fade down complete");
            Plugin_123_setPins_Finish();
          }
        }
        analogWrite(_pins[PinIndex].PinNo, _pins[PinIndex].CurrentLevel);
      }
    }
  }
}


// ---------------------------------------------------------------------------------
// ------------------------------ rgb2WhitePortion ---------------------------------
// ---------------------------------------------------------------------------------
int Plugin_123_rgb2WhitePortion(int r, int g, int b, int base)
{
  //white part, base 255 or 1023
  int w = base;
  if (r < w) w = r;
  if (g < w) w = g;
  if (b < w) w = b;
  return w;
}


// ---------------------------------------------------------------------------------
// ------------------------------ Hue2RGB ------------------------------------------
// ---------------------------------------------------------------------------------
float Plugin_123_Hue2RGB(float v1, float v2, float vH)
{
  if (vH < 0) vH += 1;
  if (vH > 1) vH -= 1;
  if ((6 * vH) < 1) return (v1 + (v2 - v1) * 6 * vH);
  if ((2 * vH) < 1) return v2;
  if ((3 * vH) < 2) return v1 + (v2 - v1) * ((0.66666666666) - vH) * 6;
  return (v1);
}


// ---------------------------------------------------------------------------------
// ------------------------------ ct2RGB -------------------------------------------
// ---------------------------------------------------------------------------------

void Plugin_123_ct2RGB()
{
  // calculation based on: http://www.tannerhelland.com/4435/convert-temperature-rgb-algorithm-code

  // reduce ct to a valid/useful value
  if (_lightParam.ct > 5500) _lightParam.ct = 5500;
  if (_lightParam.ct < 2500) _lightParam.ct = 2500;
  
  // kelvin -> mired
  double ct = 1000000/_lightParam.ct;
  // adjusted by 1000K
  double temp = (1000000/ct)/100 + 10;

  int r = 0;
  int g = 0;
  int b = 0;

  r = 255;
  if (temp > 66) r = pow(329.698727446 * (temp - 60), -0.1332047592);
  if (r < 0)     r = 0 ;
  if (r > 255)   r = 255 ;

  g = (temp <= 66) 
    ? 99.4708025861 * log(temp) - 161.1195681661
    : pow(288.1221695283 * (temp - 60), -0.0755148492);
  if (g < 0)   g = 0 ;
  if (g > 255) g = 255;

  b = 255;
  if (temp <= 19) b = 0;
  if (temp < 66)  b = 138.5177312231 * log(temp-10) - 305.0447927307;
  if (b < 0)      b = 0 ;
  if (b > 255)    b = 255 ;

  _pins[0].FadingTargetLevel = r * 4 * _lightParam.pct / 100;
  _pins[1].FadingTargetLevel = g * 4 * _lightParam.pct / 100;
  _pins[2].FadingTargetLevel = b * 4 * _lightParam.pct / 100;

  if (Plugin_123_debug) {
    Serial.print(F("Lights: ct2RGB: "));
    Serial.print(r);
    Serial.print(F("/"));
    Serial.print(g);
    Serial.print(F("/"));
    Serial.println(b);
  }
}


// ---------------------------------------------------------------------------------
// ------------------------------ rgbStr2Num ---------------------------------------
// ---------------------------------------------------------------------------------
int32_t Plugin_123_rgbStr2Num(String rgbStr)
{
  int32_t rgbDec = (int) strtol( &rgbStr[0], NULL, 16);
  return rgbDec;
}


// ---------------------------------------------------------------------------------
// ------------------------------ set Current Level To Zero If Off -----------------
// ---------------------------------------------------------------------------------
void Plugin_123_setCurrentLevelToZeroIfOff()
{
  if (!_lightParam.state) {
    for (int PinIndex = 0; PinIndex < 5; PinIndex++) {
      _pins[PinIndex].CurrentLevel = 0;
    }
  }
}


// ---------------------------------------------------------------------------------
// ------------------------------ JsonResponse -------------------------------------
// ---------------------------------------------------------------------------------
void Plugin_123_SendStatus(byte eventSource)
{
  String log = String(F("Lights: Set ")) + _pins[0].FadingTargetLevel
             + String(F("/")) + _pins[1].FadingTargetLevel + String(F("/")) + _pins[2].FadingTargetLevel
             + String(F("/")) + _pins[3].FadingTargetLevel + String(F("/")) + _pins[4].FadingTargetLevel;
  addLog(LOG_LEVEL_INFO, log);

  String onOff = (_lightParam.state) ? "on" : "off";
  String cm = (_lightParam.colorMode == 2) ? "ct" : "rgb";

  String json;
  printToWebJSON = true;
  json += F("{\n");
  json += F("\"plugin\": \"123");
  json += F("\",\n\"onOff\": \"");
  json += onOff;
  json += F("\",\n\"rgb\": \"");
  json += _lightParam.rgbStr;
  json += F("\",\n\"pct\": \"");
  json += _lightParam.pct;
  json += F("\",\n\"ct\": \"");
  json += _lightParam.ct;
  json += F("\",\n\"colormode\": \"");
  json += cm;
  json += F("\"\n}\n");
  SendStatus(eventSource, json); // send http response to controller (JSON fmormat)
}


// ---------------------------------------------------------------------------------
// ------------------------------ Debug --------------------------------------------
// ---------------------------------------------------------------------------------
void Plugin_123_dumpValues()
{
  Serial.println(F(""));
  Serial.print(F("Lights: colorMode: "));     Serial.println(_lightParam.colorMode);
  Serial.print(F("Lights: ct: "));            Serial.println(_lightParam.ct);
  Serial.print(F("Lights: pct: "));           Serial.println(_lightParam.pct);
  Serial.print(F("Lights: r: "));             Serial.print(_pins[0].CurrentLevel); Serial.print(F(" -> ")); Serial.println(_pins[0].FadingTargetLevel);
  Serial.print(F("Lights: g: "));             Serial.print(_pins[1].CurrentLevel); Serial.print(F(" -> ")); Serial.println(_pins[1].FadingTargetLevel);
  Serial.print(F("Lights: b: "));             Serial.print(_pins[2].CurrentLevel); Serial.print(F(" -> ")); Serial.println(_pins[2].FadingTargetLevel);
  Serial.print(F("Lights: ww: "));            Serial.print(_pins[3].CurrentLevel); Serial.print(F(" -> ")); Serial.println(_pins[3].FadingTargetLevel);
  Serial.print(F("Lights: cw: "));            Serial.print(_pins[4].CurrentLevel); Serial.print(F(" -> ")); Serial.println(_pins[4].FadingTargetLevel);
  Serial.print(F("Lights: state: "));         Serial.print(_lightParam.state);
  Serial.println(F(""));
  
}


