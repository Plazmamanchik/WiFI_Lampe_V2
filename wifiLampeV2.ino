





/*
╔════════════╦═══════════╗
║   D[нога]  ║  GPIO[N]  ║
╠════════════╬═══════════╣
║    D0      ║   GPIO16  ║
║    D1      ║   GPIO5   ║ (I2C SCL)
║    D2      ║   GPIO4   ║ (I2C SDA)
║    D3      ║   GPIO0   ║
║    D4      ║   GPIO2   ║
║    D5      ║   GPIO14  ║ (SPI SCK)
║    D6      ║   GPIO12  ║ (SPI MISO)
║    D7      ║   GPIO13  ║ (SPI MOSI)
║    D8      ║   GPIO15  ║ (SPI CS)
║    D9      ║   GPIO3   ║ (RX)
║    D10     ║   GPIO1   ║ (TX)
║    D11     ║   GPIO9   ║ (зазвичай не використовується)
║    D12     ║   GPIO10  ║ (зазвичай не використовується)
╚════════════╩═══════════╝
*/




#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <ESP8266mDNS.h>
#include <DNSServer.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>


class Button {
  public:
    #define BTN_NO_PIN -1
    #define HIGH_PULL 0
    #define LOW_PULL 1
    #define NORM_OPEN 0
    #define NORM_CLOSE 1
    #define MANUAL 0
    #define AUTO 1
    Button(int8_t pin, bool type = false, bool dir = false) {
        if (pin != -1) {
            _PIN = static_cast<uint8_t>(pin);
            noPin = false;
            pinMode(_PIN, type ? INPUT : INPUT_PULLUP);
        } else {
            noPin = true;
        }
        inv_state = dir;
    }

    void setDebounce(uint16_t debounce) { _debounce = debounce; }
    void setTimeout(uint16_t timeout) { _timeout = timeout; }
    void setClickTimeout(uint16_t clickTimeout) { _click_timeout = clickTimeout; }
    void setStepTimeout(uint16_t stepTimeout) { _step_timeout = stepTimeout; }

    bool isPress() { return checkFlag(isPress_f); }
    bool isRelease() { return checkFlag(isRelease_f); }
    bool isClick() { return checkFlag(isOne_f); }
    bool isHolded() { return checkFlag(isHolded_f); }
    bool isHold() { return step_flag; }
    bool state() { return btn_state; }
    bool isSingle() { return checkCounter(1); }
    bool isDouble() { return checkCounter(2); }
    bool isTriple() { return checkCounter(3); }
    bool hasClicks() { return checkFlag(counter_flag); }
    uint8_t getClicks() { counter_reset = true; return last_counter; }
    uint8_t getHoldClicks() { return last_hold_counter; }
    bool isStep(uint8_t clicks) {
        if (btn_counter == clicks && step_flag && (millis() - btn_timer >= _step_timeout)) {
            btn_timer = millis();
            return true;
        }
        return false;
    }

    void resetStates() {
        isPress_f = isRelease_f = isOne_f = isHolded_f = step_flag = counter_flag = false;
        last_hold_counter = last_counter = 0;
    }

    void tick() {
        if (!noPin) btn_state = digitalRead(_PIN) ^ inv_state;
        uint32_t now = millis();

        if (btn_state && !btn_flag) {
            if (!btn_deb) {
                btn_deb = true;
                btn_timer = now;
            } else if (now - btn_timer >= _debounce) {
                btn_flag = isPress_f = oneClick_f = true;
            }
        } else {
            btn_deb = false;
        }

        if (!btn_state && btn_flag) {
            btn_flag = false;
            if (!hold_flag) btn_counter++;
            hold_flag = false;
            isRelease_f = true;
            btn_timer = now;
            if (step_flag) resetStep();
            if (oneClick_f) { oneClick_f = false; isOne_f = true; }
        }

        if (btn_flag && btn_state && (now - btn_timer >= _timeout) && !hold_flag) {
            hold_flag = true;
            last_hold_counter = btn_counter;
            isHolded_f = step_flag = true;
            oneClick_f = false;
            btn_timer = now;
        }

        if ((now - btn_timer >= _click_timeout) && btn_counter && !btn_state) {
            last_counter = btn_counter;
            btn_counter = 0;
            counter_flag = true;
        }

        if (counter_reset) resetCounter();
    }

  private:
    uint8_t _PIN;
    uint16_t _debounce = 50, _timeout = 1000, _click_timeout = 400, _step_timeout = 500;
    uint32_t btn_timer = 0;
    uint8_t btn_counter = 0, last_counter = 0, last_hold_counter = 0;
    bool btn_state = false, btn_flag = false, hold_flag = false, step_flag = false;
    bool isPress_f = false, isRelease_f = false, isOne_f = false, isHolded_f = false;
    bool btn_deb = false, oneClick_f = false, counter_flag = false, counter_reset = false;
    bool noPin = true, inv_state = false;

    bool checkFlag(bool &flag) {
        if (flag) { flag = false; return true; }
        return false;
    }
    bool checkCounter(uint8_t count) {
        if (counter_flag && last_counter == count) { counter_reset = true; return true; }
        return false;
    }
    void resetCounter() { last_counter = 0; counter_flag = counter_reset = false; }
    void resetStep() { last_counter = 0; btn_counter = 0; step_flag = false; }
};




class LED {
  public:

    LED(int8_t pin_r_, int8_t pin_g_, int8_t pin_b_, bool dir = false) {
        if (pin_r_ != -1 && pin_g_ != -1  && pin_b_ != -1 ) {
          PIN_R = pin_r_;
          PIN_G = pin_g_;
          PIN_B = pin_b_;

          pinMode(PIN_R, OUTPUT);
          pinMode(PIN_G, OUTPUT);
          pinMode(PIN_B, OUTPUT);
      }

    }

    void setRGB_1023(const int16_t v_red, const int16_t v_green, const int16_t v_blue){
      VALUE_RED = v_red >> 2;
      VALUE_GREEN = v_green >> 2;
      VALUE_BLUE = v_blue >> 2;

      analogWrite(PIN_R,((uint16_t)VALUE_RED * brightness) >> 8);
      analogWrite(PIN_G,((uint16_t)VALUE_GREEN * brightness) >> 8);
      analogWrite(PIN_B,((uint16_t)VALUE_BLUE * brightness) >> 8);
    }

    void setRGB_255(const uint8_t v_red, const uint8_t v_green, const uint8_t v_blue){
      VALUE_RED = v_red;
      VALUE_GREEN = v_green;
      VALUE_BLUE = v_blue;

      analogWrite(PIN_R,((uint16_t)VALUE_RED * brightness) >> 8);
      analogWrite(PIN_G,((uint16_t)VALUE_GREEN * brightness) >> 8);
      analogWrite(PIN_B,((uint16_t)VALUE_BLUE * brightness) >> 8);
    }

     void setHex(const char *hex) {
      uint8_t rv, gv, bv;

      // "#RGB" формат
      if (hex[4] == '\0') {
        rv = (hex[1] <= '9' ? hex[1]-'0' : (hex[1] & ~0x20)-'A'+10);
        gv = (hex[2] <= '9' ? hex[2]-'0' : (hex[2] & ~0x20)-'A'+10);
        bv = (hex[3] <= '9' ? hex[3]-'0' : (hex[3] & ~0x20)-'A'+10);

        rv = (rv << 4) | rv;
        gv = (gv << 4) | gv;
        bv = (bv << 4) | bv;
      }
      // "#RRGGBB" формат
      else {
        rv = ((hex[1] <= '9' ? hex[1]-'0' : (hex[1] & ~0x20)-'A'+10) << 4) |
              (hex[2] <= '9' ? hex[2]-'0' : (hex[2] & ~0x20)-'A'+10);

        gv = ((hex[3] <= '9' ? hex[3]-'0' : (hex[3] & ~0x20)-'A'+10) << 4) |
              (hex[4] <= '9' ? hex[4]-'0' : (hex[4] & ~0x20)-'A'+10);

        bv = ((hex[5] <= '9' ? hex[5]-'0' : (hex[5] & ~0x20)-'A'+10) << 4) |
              (hex[6] <= '9' ? hex[6]-'0' : (hex[6] & ~0x20)-'A'+10);
      }

      // встановлюємо колір
      VALUE_RED = rv;
      VALUE_GREEN = gv;
      VALUE_BLUE = bv;
      setRGB_255(VALUE_RED,VALUE_GREEN,VALUE_BLUE);
    }

  void setHue(const uint8_t color) {                   // 8-ми битное цветовое колесо
    uint8_t shift = 0;                              // Цветовой сдвиг
    if (color > 170) {                              // Синий - фиолетовый - красный
      shift = (color - 170) * 3;                    // Получаем цветовой сдвиг 0 - 255
      VALUE_RED = shift, VALUE_GREEN = 0, VALUE_BLUE = ~shift;     // Получаем компоненты RGB
    } else if (color > 85) {                        // Зеленый - голубой - синий
      shift = (color - 85) * 3;                     // Получаем цветовой сдвиг 0 - 255
      VALUE_RED = 0, VALUE_GREEN = ~shift, VALUE_BLUE = shift;     // Получаем компоненты RGB
    } else {                                        // Красный - оранжевый - зеленый
      shift = (color - 0) * 3;                      // Получаем цветовой сдвиг 0 - 255
      VALUE_RED = ~shift, VALUE_GREEN = shift, VALUE_BLUE = 0;     // Получаем компоненты RGB
      
    }
    setRGB_255(VALUE_RED,VALUE_GREEN,VALUE_BLUE);
  }
  uint8_t getBrightCRT(byte val) {
    return pgm_read_byte(&(CRTgammaPGM[val]));
  }

  void setBrightnessCRT(const uint8_t brig) {                   // Применяем яркость на компоненты RGB
    brightness = getBrightCRT(brig);
    //brightness = brig;
    setRGB_255(VALUE_RED, VALUE_GREEN, VALUE_BLUE);
  }

  void setBrightness(const uint8_t brig) {                   // Применяем яркость на компоненты RGB
    brightness = brig;
    //brightness = brig;
    setRGB_255(VALUE_RED, VALUE_GREEN, VALUE_BLUE);
  }
  
  uint8_t getValue_RED(){
    return VALUE_RED;
  }
  uint8_t getValue_GREEN(){
    return VALUE_GREEN;
  }
  uint8_t getValue_BLUE(){
    return VALUE_BLUE;
  }
  uint8_t getBrightness(){
    return brightness;
  }


  private:
    int8_t PIN_R, PIN_G, PIN_B;
    uint8_t VALUE_RED, VALUE_GREEN, VALUE_BLUE;
    uint8_t brightness = 255;

    const uint8_t CRTgammaPGM[256] = {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,
      1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3,
      4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 8,
      8, 8, 9, 9, 9, 10, 10, 10, 11, 11, 12, 12, 12, 13, 13, 14,
      14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22,
      23, 23, 24, 24, 25, 26, 26, 27, 28, 28, 29, 30, 30, 31, 32, 32,
      33, 34, 35, 35, 36, 37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 45,
      46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
      62, 63, 64, 65, 66, 68, 69, 70, 71, 72, 73, 74, 76, 77, 78, 79,
      81, 82, 83, 84, 86, 87, 88, 90, 91, 92, 94, 95, 96, 98, 99, 101,
      102, 103, 105, 106, 108, 109, 111, 112, 114, 115, 117, 118, 120, 122, 123, 125,
      126, 128, 130, 131, 133, 135, 136, 138, 140, 142, 143, 145, 147, 149, 150, 152,
      154, 156, 158, 160, 161, 163, 165, 167, 169, 171, 173, 175, 177, 179, 181, 183,
      185, 187, 189, 191, 193, 195, 197, 200, 202, 204, 206, 208, 210, 213, 215, 217,
      219, 221, 224, 226, 228, 231, 233, 235, 238, 240, 242, 245, 247, 250, 252, 255,
    };
};



#define SYS_BTN_PIN 10 // (14 - d5)

#define LED_PIN 2 //d4
Button sysBtn(SYS_BTN_PIN,LOW_PULL,NORM_OPEN);


#define LED_GREEN 12
#define LED_RED 4
#define LED_BLUE 13
LED led(LED_RED,LED_GREEN,LED_BLUE);


bool isWiFiOn = false;


const char* PORTAL_NAME = "ledsgarland";
const byte DNS_PORT = 80;//53
DNSServer dnsServer;
IPAddress apIP(192, 168, 4, 1); 

String apSSID = "ESP_LedsGarland";
String apPassword = "";

ESP8266WebServer webServer(80);

struct WiFiSettingsStruct {
  char ssid[41];    
  char password[41];
} wifiSettings;



#define IR_PIN 14 // D5
IRrecv irrecv(IR_PIN);
decode_results results;



//---------------------------LEDS-------------------
bool isGarlandOn = true;
byte mode = 0;//3
ushort flashTick = 1000;
int strobeTick = 100;
ushort fadeTick = 5;
ushort smoothTick = 1000;

uint8_t saveColor_R = 255;
uint8_t saveColor_G = 255;  
uint8_t saveColor_B = 255; 
//--------------------------------------------------

const char PAGE_ROOT[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Remotecontrol</title>

<!-- ----------------------------------------------------------------------------------------------------STYLE REMOTECONTROL ---------------------------------------------------------------------------------------------------- -->
    <style>
        /* Змінено для контролю переповнення */
        .remotecontrol {
          -ms-user-select: none; /* Для Internet Explorer/Edge */
            -moz-user-select: none; /* Для Firefox */
            -webkit-user-select: none; /* Для Chrome, Safari, Opera */

            background: white; 
            border: 2px solid black;
            width: 290px; 
            height: 500px; 
            margin: 20px auto; /* Додано відступ знизу */
            border-radius: 10px;
            display: flex;
            flex-direction: column; /* Змінено, щоб елементи були розташовані вертикально */
            justify-content: space-between; /* Змінено, щоб елементи були розташовані вертикально */
            padding: 10px; /* Змінено, щоб було більше простору між контролерами */
            margin-top: 60px;
        }
        .headcon {
            display: flex;
            justify-content: space-between; /* Змінено, щоб контролери були розташовані горизонтально */
        }
        .bodycon {
            display: flex;
            justify-content: center; /* Змінено, щоб контролери були розташовані по центру */
            height: 400px;
            width: 260px;
            flex-direction: row; /* Змінено, щоб елементи були розташовані горизонтально */
        }
        .brightness {
            background: gray;
            border: none; 
            width: 120px; 
            height: 40px; 
            border-radius: 7px;
            margin-top: 40px;
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 5px;
        }
        .switch {
            background: gray;
            border: none; 
            width: 120px; 
            height: 40px; 
            border-radius: 7px;
            margin-top: 40px;
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 5px;
        }
        .button {
          background: white;
          border: none;
          width: 40px;
          height: 40px;
          border-radius: 50%;
          display: flex;
          justify-content: center;
          align-items: center;
          cursor: pointer;
        }
        .color {
            background: gray;
            border: none; 
            width: 200px; 
            height: 390px; 
            border-radius: 7px 0 7px 7px; /* Змінено, щоб кути були закруглені */
            margin-bottom: 40px; /* Додано відступ знизу */
            display: grid; /* Змінено, щоб можна було використовувати grid-систему для розміщення кнопок */
            grid-template-columns: repeat(3, 1fr); /* Змінено, щоб було 4 стовпчика з однаковою шириною */
            grid-template-rows: repeat(2, 1fr) repeat(3, 1fr);
            grid-gap: 5px; /* Змінено, щоб було простори між кнопками */
            padding: 5px; /* Змінено, щоб кнопки не прилягали до країв блоку */
        }
        .white {
            background: gray;
            border: none;
            width: 40px;
            height: 40px;
            border-radius: 0 7px 15px 0;
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 5px;
            margin-bottom: 10px;
        }
        .mode {
            background-color: white;
            border-radius: 5px 0 0 0;
            height: 348px;
            width: 60px;
            position: absolute;
            top: 268px;
            margin-left: 216px;
            padding: 5px;
        }
        .mode .button {
            
            user-select: none;
            background-color: gray;
            width: 40px;
            height: 40px;
            border-radius: 50%;
            margin-bottom: 5px;
            word-wrap: break-word;
            overflow: hidden;
            text-overflow: ellipsis;
            font-size: 9px;
        }
        .led {
            width: 40px;
            height: 40px;
            background-color: white;
            margin: 0 auto;
            border-radius: 7px;
            border: 0px solid black
        }
    </style>

<!-- ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- -->

<!-- ----------------------------------------------------------------------------------------------------SCRIPT REMOTECONTROL ---------------------------------------------------------------------------------------------------- -->

    <script>
          function color(colorVaule) {
            currentColor = colorVaule;
            var xhttp = new XMLHttpRequest();
            xhttp.open('POST', '/', true);
            xhttp.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');
            xhttp.send('color=' + currentColor);
          }
          function switchLED(state) {
            var xhttp = new XMLHttpRequest();
            xhttp.open('POST', '/', true);
            xhttp.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');
            xhttp.send('command=' + state);
          }
          function tickCommand(command) {
            var xhttp = new XMLHttpRequest();
            xhttp.open('POST', '/', true);
            xhttp.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');
            xhttp.send('tickCommand=' + command);
          }
          function modeCommand(mode) {
            var xhttp = new XMLHttpRequest();
            xhttp.open('POST', '/', true);
            xhttp.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');
            xhttp.send('modeCommand=' + mode);
          }
           function updateSensorValue() {
              var xhttp = new XMLHttpRequest();
              xhttp.onreadystatechange = function() {
                  if (this.readyState == 4 && this.status == 200) {
                      var analogValues = this.responseText.split(',');
                      var dr = parseInt(analogValues[0]);
                      var dg = parseInt(analogValues[1]);
                      var db = parseInt(analogValues[2]);
                      var ledColor = 'rgb(' + dr + ',' + dg + ',' + db + ')';
                      setRGB(dr,dg,db);
                      document.getElementById('led').style.backgroundColor = ledColor;
                  }
              };
              xhttp.open('GET', '/sendData', true);
              xhttp.send();
          }
          setInterval(updateSensorValue, 70);



        function connectWiFi() {
          var ssid = document.getElementById('ssid').value;
          var password = document.getElementById('password').value;
          var xhttp = new XMLHttpRequest();
          xhttp.open('POST', '/connect', true);
          xhttp.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');
          xhttp.send('ssid=' + ssid + '&password=' + password);
        }

        function back() {
          window.location.href = '/';
        }

        function updateIp() {
          var xhttp = new XMLHttpRequest();
          xhttp.onreadystatechange = function() {
            if (this.readyState == 4 && this.status == 200) {
              var conteinerWiFi = document.getElementById('setting');
              var showIpWiFi = document.getElementById('showIp');
              if (this.responseText == '0') {
                 conteinerWiFi.style.display='block';
                 showIpWiFi.style.display='none';
              } else {
                 conteinerWiFi.style.display='none';
                 showIpWiFi.style.display='block';
                 showIpWiFi.innerText = this.responseText;
              }
            }
          };
          xhttp.open('GET', '/getIpStan', true);
          xhttp.send();
        }
        setInterval(updateIp, 1000);
     </script>

<!-- --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- -->


</head>
<body style="background-color:grey">

<!-- ----------------------------------------------------------------------------------------------------HTML REMOTECONTROL ---------------------------------------------------------------------------------------------------- -->

    <div class="led" id="led"></div>
    <div class="remotecontrol">
        <div class="headcon">
            <div class="brightness">
                <div class="button" style="font-size: 24px; font-weight: bold;" onclick="tickCommand('add')">⇧</div>
                <div class="button" style="font-size: 24px; font-weight: bold;" onclick="tickCommand('remove')">⇩</div>
            </div>
            <div class="switch">
                <div class="button" style="background-color: black; color: white;" onclick="switchLED('off')">Off</div>
                <div class="button" style="background-color: red; color: white;" onclick="switchLED('on')">On</div>
            </div>
        </div>
        <div class="bodycon">
            <div class="color">
                <div class="button" style="background-color: #FF0000;" onclick="color('#FF0000')">R</div>
                <div class="button" style="background-color: #008000;" onclick="color('#008000')">G</div>
                <div class="button" style="background-color: #00008B;" onclick="color('#00008B')">B</div>
                <div class="button" style="background-color: #FF8C00;" onclick="color('#FF8C00')"></div>
                <div class="button" style="background-color: #00FF00;" onclick="color('#00FF00')"></div>
                <div class="button" style="background-color: #0000FF;" onclick="color('#0000FF')"></div>
                <div class="button" style="background-color: #FFA500;" onclick="color('#FFA500')"></div>
                <div class="button" style="background-color: #20B2AA;" onclick="color('#20B2AA')"></div>
                <div class="button" style="background-color: #800080;" onclick="color('#800080')"></div>
                <div class="button" style="background-color: #FFD700;" onclick="color('#FFD700')"></div>
                <div class="button" style="background-color: #008B8B;" onclick="color('#008B8B')"></div>
                <div class="button" style="background-color: #FF00FF;" onclick="color('#FF00FF')"></div>
                <div class="button" style="background-color: #FFFF00;" onclick="color('#FFFF00')"></div>
                <div class="button" style="background-color: #008080;" onclick="color('#008080')"></div>
                <div class="button" style="background-color: #EE82EE;" onclick="color('#EE82EE')"></div>
            </div>
            <div class="white">
                <div class="button" style="background-color: #FFFFFF;" onclick="color('#FFFFFF')">W</div>
            </div>
            <div class="mode">
                <div class="button" style="background-color: gray; margin-top: 29px; " onclick="modeCommand('flash')">FLASH</div>
                <div class="button" style="background-color: gray; margin-top: 39px" onclick="modeCommand('strobe')">STROBE</div>
                <div class="button" style="background-color: gray; margin-top: 38px" onclick="modeCommand('fade')">FADE</div>
                <div class="button" style="background-color: gray; margin-top: 41px;" onclick="modeCommand('smooth')">SMOOTH</div>
            </div>
        </div>
    </div>

<!-- --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- -->


<!-- ----------------------------------------------------------------------------------------------------STYLE WIFI ---------------------------------------------------------------------------------------------------- -->

<style>

    .container {
      max-width: 400px;
      margin: 0 auto;
      padding: 20px;
      background-color: #9c9a9a;
      border-radius: 5px;
      box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
    }
    h1 {
      color: #333;
      text-align: center;
    }
    label {
      display: block;
      margin-bottom: 10px;
      font-weight: bold;
    }
    .input-container {
      display: flex;
      flex-direction: column;
      align-items: center;
      margin-bottom: 20px; /* Відступ між контейнером інпутів і іншими елементами */
    }
    input[type='text'], input[type='password'] {
      width: 100%;
      padding: 10px;
      margin-bottom: 10px; /* Відступ між інпутами */
      border: 1px solid #ccc;
      border-radius: 3px;
    }
    .connect-button {
      background-color: #007BFF;
      color: #fff;
      border: none;
      padding: 10px 20px;
      border-radius: 3px;
      cursor: pointer;
      text-align:center
    }
    .connect-button:hover {
      background-color: #0056b3;
    }
    .back-button {
      background-color: #ccc;
      color: #333;
      border: none;
      padding: 10px 20px;
      border-radius: 3px;
      cursor: pointer;
      margin-top: 10px; /* Відступ між кнопкою і іншими елементами */
    }
    .back-button:hover {
      background-color: #999;
    }
  </style>

<!-- --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- -->

<!-- ----------------------------------------------------------------------------------------------------STYLE COLOR PICKER ---------------------------------------------------------------------------------------------------- -->



<style>
.container {
  text-align: center;
  font-family: sans-serif;
}

.color-picker {
  position: relative;
  display: inline-block;
}

#colorWheel {
  border-radius: 50%;
  cursor: crosshair;
  touch-action: none;
}

#selector {
  position: absolute;
  width: 14px;
  height: 14px;
  border: 2px solid #000;
  border-radius: 50%;
  background: white;
  pointer-events: none;
  transform: translate(-50%, -50%);
  transition: background 0.15s linear;
}

.rgb-inputs {
  display: flex;
  justify-content: center;
  gap: 10px;
  margin-top: 15px;
}

.rgb-inputs input {
  width: 60px;
  text-align: center;
  border: 1px solid #888;
  border-radius: 5px;
  padding: 5px;
  font-size: 14px;
}

.slider-container {
  width: 80%;
  margin: 20px auto;
  text-align: center;
}

.slider-container label {
  display: block;
  margin-bottom: 5px;
  font-weight: bold;
}

.slider-container input[type=range] {
  width: 100%;
  appearance: none;
  height: 8px;
  border-radius: 5px;
  outline: none;
  background: linear-gradient(to right, black, var(--slider-color, white));
}


</style>


<!-- --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- -->

<!-- ---------------------------------------------------------------------------------------------------- SCRIPT COLOR PICKER ---------------------------------------------------------------------------------------------------- -->





<script>
function drawColorWheel(canvas, radius) {
  const ctx = canvas.getContext("2d");
  const image = ctx.createImageData(radius * 2, radius * 2);

  for (let y = -radius; y < radius; y++) {
    for (let x = -radius; x < radius; x++) {
      const i = (y + radius) * radius * 2 + (x + radius);
      const d = Math.sqrt(x * x + y * y);
      if (d <= radius) {
        const angle = Math.atan2(y, x) * 180 / Math.PI;
        const hue = (angle + 360) % 360;
        const sat = d / radius;
        const color = hslToRgb(hue / 360, sat, 0.5 + (1 - sat) * 0.5);
        const index = i * 4;
        image.data[index] = color[0];
        image.data[index + 1] = color[1];
        image.data[index + 2] = color[2];
        image.data[index + 3] = 255;
      }
    }
  }
  ctx.putImageData(image, 0, 0);
}

function hslToRgb(h, s, l) {
  let r, g, b;
  if (s == 0) {
    r = g = b = l;
  } else {
    const hue2rgb = function (p, q, t) {
      if (t < 0) t += 1;
      if (t > 1) t -= 1;
      if (t < 1 / 6) return p + (q - p) * 6 * t;
      if (t < 1 / 2) return q;
      if (t < 2 / 3) return p + (q - p) * (2 / 3 - t) * 6;
      return p;
    };
    const q = l < 0.5 ? l * (1 + s) : l + s - l * s;
    const p = 2 * l - q;
    r = hue2rgb(p, q, h + 1 / 3);
    g = hue2rgb(p, q, h);
    b = hue2rgb(p, q, h - 1 / 3);
  }
  return [r * 255, g * 255, b * 255];
}

function rgbToHsl(r, g, b) {
  r /= 255; g /= 255; b /= 255;
  const max = Math.max(r, g, b);
  const min = Math.min(r, g, b);
  let h, s, l = (max + min) / 2;

  if (max === min) {
    h = s = 0;
  } else {
    const d = max - min;
    s = l > 0.5 ? d / (2 - max - min) : d / (max + min);
    switch (max) {
      case r: h = (g - b) / d + (g < b ? 6 : 0); break;
      case g: h = (b - r) / d + 2; break;
      case b: h = (r - g) / d + 4; break;
    }
    h /= 6;
  }
  return [h, s, l];
}

let inputs, selector, ctx, radius, canvas;
let lastSendTime_RGB = 0;
let lastSendTime_Brightness = 0;
let lastSendTime_Warmth = 0;


const SEND_INTERVAL = 100; // 5 запитів/сек

window.onload = () => {
  canvas = document.getElementById("colorWheel");
  selector = document.getElementById("selector");
  ctx = canvas.getContext("2d");
  radius = canvas.width / 2;
  inputs = {
    r: document.getElementById("rInput"),
    g: document.getElementById("gInput"),
    b: document.getElementById("bInput"),
  };

  const brightnessSlider = document.getElementById("brightness");
  const warmthSlider = document.getElementById("warmth");

  drawColorWheel(canvas, radius);
  selector.style.left = radius + "px";
  selector.style.top = radius + "px";

  let dragging = false;

  function updateSelector(clientX, clientY) {
    const rect = canvas.getBoundingClientRect();
    const cx = rect.left + radius;
    const cy = rect.top + radius;
    const dx = clientX - cx;
    const dy = clientY - cy;
    const d = Math.sqrt(dx * dx + dy * dy);
    if (d > radius) return;

    selector.style.left = clientX - rect.left + "px";
    selector.style.top = clientY - rect.top + "px";

    const pixel = ctx.getImageData(clientX - rect.left, clientY - rect.top, 1, 1).data;
    const r = pixel[0], g = pixel[1], b = pixel[2];
    selector.style.background = `rgb(${r},${g},${b})`;

    inputs.r.value = Math.round(r);
    inputs.g.value = Math.round(g);
    inputs.b.value = Math.round(b);

    // оновлюємо колір повзунка яскравості
    document.documentElement.style.setProperty("--slider-color", `rgb(${r},${g},${b})`);

    sendRGB(r, g, b);
  }

  // події миші
  canvas.addEventListener("mousedown", (e) => {
    dragging = true;
    updateSelector(e.clientX, e.clientY);
  });
  window.addEventListener("mousemove", (e) => {
    if (dragging) updateSelector(e.clientX, e.clientY);
  });
  window.addEventListener("mouseup", () => (dragging = false));

  // сенсор
  canvas.addEventListener("touchstart", (e) => {
    dragging = true;
    const t = e.touches[0];
    updateSelector(t.clientX, t.clientY);
  });
  window.addEventListener("touchmove", (e) => {
    if (dragging) {
      const t = e.touches[0];
      updateSelector(t.clientX, t.clientY);
    }
  });
  window.addEventListener("touchend", () => (dragging = false));

  // яскравість
  brightnessSlider.addEventListener("input", (e) => {
    const value = +e.target.value;
    setBrightness(value);
  });

  // теплота (від білого до жовтого)
  warmthSlider.addEventListener("input", (e) => {

    const now = Date.now();
    if (now - lastSendTime_Warmth < 150) return;
    lastSendTime_Warmth = now;

    const value = +e.target.value; // 0–255
    const r = 255;
    const g = 255 - value * 0.22; // легке зменшення зеленого
    const b = 255 - value; // прибираємо синій
    setRGB(r, g, b);
    sendRGB(r, g, b);
  });
};

// ======== відправка з throttle ========
function sendRGB(r, g, b) {
  const now = Date.now();
  if (now - lastSendTime_RGB < SEND_INTERVAL) return;
  lastSendTime_RGB = now;

  const xhttp = new XMLHttpRequest();
  xhttp.open('POST', '/', true);
  xhttp.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');
  xhttp.send(`r=${r}&g=${g}&b=${b}`);
}

// ======== встановлення кольору вручну ========
function setRGB(r, g, b) {
  if (!canvas || !selector) return;

  inputs.r.value = r;
  inputs.g.value = g;
  inputs.b.value = b;
  selector.style.background = `rgb(${r},${g},${b})`;

  const [h, s, l] = rgbToHsl(r, g, b);
  const angle = h * 2 * Math.PI;
  const dist = s * radius;
  const x = radius + Math.cos(angle) * dist;
  const y = radius + Math.sin(angle) * dist;
  selector.style.left = x + "px";
  selector.style.top = y + "px";

  document.documentElement.style.setProperty("--slider-color", `rgb(${r},${g},${b})`);
}

// ======== встановлення яскравості ========
function setBrightness(value) {

  const now = Date.now();
  if (now - lastSendTime_Brightness < SEND_INTERVAL) return;
  lastSendTime_Brightness = now;

  const xhttp = new XMLHttpRequest();
  xhttp.open('POST', '/', true);
  xhttp.setRequestHeader('Content-type', 'application/x-www-form-urlencoded');
  xhttp.send('brightness=' + value);
}

function getBrightness() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      const value = parseInt(this.responseText);
      const brightnessSlider = document.getElementById('brightness');
      brightnessSlider.value = value; // правильно без дужок після value
    }
  };
  xhttp.open('GET', '/sendBrightness', true);
  xhttp.send();
}

getBrightness();


</script>


<!-- --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- -->

<!-- ---------------------------------------------------------------------------------------------------- HTML COLOR PICKER ---------------------------------------------------------------------------------------------------- -->

<div class='container'>
  <h1>Color Picker</h1>
  <div class="color-picker">
    <canvas id="colorWheel" width="300" height="300"></canvas>
    <div id="selector"></div>
  </div>

  <div class="rgb-inputs">
    <input type="number" id="rInput" min="0" max="255" value="255">
    <input type="number" id="gInput" min="0" max="255" value="255">
    <input type="number" id="bInput" min="0" max="255" value="255">
  </div>

  <div class="slider-container">
    <label>Brightness</label>
    <input type="range" class="range-brightness" id="brightness" min="0" max="255" value="255">
  </div>

  <div class="slider-container">
    <label>Warmth</label>
    <input type="range" style="background: linear-gradient(to right, white, yellow);" id="warmth" min="0" max="255" value="0">
  </div>
</div>

<!-- --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- -->


<!-- ---------------------------------------------------------------------------------------------------- HTML IP ---------------------------------------------------------------------------------------------------- -->

<h1 class='white-text' id='showIp' style='text-align:center;'></h1>

<!-- --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- -->

<!-- ---------------------------------------------------------------------------------------------------- HTML WIFI ---------------------------------------------------------------------------------------------------- -->

<div class='container' id='setting'>
  <h1>Wi-Fi налаштування</h1>
  <div class='input-container'>
    <label for='ssid'>Назва Wi-Fi:</label>
    <input type='text' id='ssid' name='ssid'>
    <label for='password'>Пароль Wi-Fi:</label>
    <input type='text' id='password' name='password'>
  </div>
  <div>
    <button class='connect-button' onclick='connectWiFi();'>
      <b>З'єднатись</b>
    </button>
  </div>
</div>

<!-- --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- -->

</body>
</html>
)rawliteral";


void handleRoot() {
  String color = webServer.arg("color");
  String command = webServer.arg("command");
  String tickCommand = webServer.arg("tickCommand");
  String modeCommand = webServer.arg("modeCommand"); 

  if(webServer.hasArg("brightness")) led.setBrightness(webServer.arg("brightness").toInt());

  if (webServer.hasArg("r") && webServer.hasArg("g") && webServer.hasArg("b") && isGarlandOn) {
    mode = 0;
    uint8_t COLOR_R = webServer.arg("r").toInt();
    uint8_t COLOR_G = webServer.arg("g").toInt();
    uint8_t COLOR_B = webServer.arg("b").toInt();

    led.setRGB_255(COLOR_R, COLOR_G, COLOR_B);
  }


  if (color != ""  && isGarlandOn) {
    mode = 0;
    led.setHex(color.c_str());
    saveColor_R = led.getValue_RED();
    saveColor_G = led.getValue_GREEN();
    saveColor_B = led.getValue_BLUE();
  }

  if (command != "") {
    if (command == "on") {
      isGarlandOn = true;
      led.setRGB_255(saveColor_R,saveColor_G,saveColor_B);
    } else {
      isGarlandOn = false;
      led.setRGB_255(0, 0, 0);
    }
  }

  
  if (tickCommand == "remove" && isGarlandOn == 1 && flashTick < 2000 && mode == 1) {
    flashTick = flashTick + 20;
  } else if (tickCommand == "add" && isGarlandOn == 1 && flashTick > 20 && flashTick != 20 && mode == 1) {
    flashTick = flashTick - 20;
  }


  if (tickCommand == "remove" && isGarlandOn == 1 && strobeTick < 100 && mode == 2) {
    strobeTick = strobeTick + 2;
  } else if (tickCommand == "add" && isGarlandOn == 1 && strobeTick > 6 && strobeTick != 7 && mode == 2) {
    strobeTick = strobeTick - 2;
  }


  if (tickCommand == "remove" && isGarlandOn == 1 && fadeTick < 50 && mode == 3) {
    fadeTick = fadeTick + 5;
  } else if (tickCommand == "add" && isGarlandOn == 1 && fadeTick > 0 && fadeTick != 7 && mode == 3) {
    fadeTick = fadeTick - 5;
  }

  if (tickCommand == "remove" && isGarlandOn == 1 && smoothTick < 2000 && mode == 4) {
    smoothTick = smoothTick + 20;
  } else if (tickCommand == "add" && isGarlandOn == 1 && smoothTick > 20 && smoothTick != 20 && mode == 4) {
    smoothTick = smoothTick - 20;
  }


  if (modeCommand != ""){
    if (modeCommand == "flash") {
      mode = 1;
    } else if (modeCommand == "strobe") {
      mode = 2;
    } else if (modeCommand == "fade") {
      mode = 3;
    } else if (modeCommand == "smooth") {
      mode = 4;
    }
  }
  webServer.send_P(200, "text/html", PAGE_ROOT);
}


void modes_led() {
  unsigned long currentMillis = millis();
  static uint8_t baseBrightness = 255;
  static uint8_t effectBrightness = 255; // починаємо з максимуму
  static int fadeAmount = -1;
  static bool flagSaveBaseBrightness = false;

  static unsigned long previousMillis = 0;
  static ushort smoothStep = 0;
  static uint8_t redValue = 255, greenValue = 0, blueValue = 0;

  if (mode == 1 && isGarlandOn == 1) {
    if (currentMillis - previousMillis >= flashTick) {
      previousMillis = currentMillis;

      if (smoothStep == 0) {
        led.setRGB_255(255,0,0);
      } else if (smoothStep == 1) {
        led.setRGB_255(255,255,0);
      } else if (smoothStep == 2) {
        led.setRGB_255(0,255,0);
      } else if (smoothStep == 3) {
        led.setRGB_255(0,255,255);
      } else if (smoothStep == 4) {
        led.setRGB_255(0,0,255);
      } else if (smoothStep == 5) {
        led.setRGB_255(255,0,255);
      }
      smoothStep = (smoothStep + 1) % 6;
    }
  }


  if (mode == 2 && isGarlandOn == 1) {

    // один раз зберігаємо базову яскравість
    if (!flagSaveBaseBrightness) {
      baseBrightness = led.getBrightness();
      effectBrightness = baseBrightness; // починаємо з поточної (наприклад 255)
      fadeAmount = -5;                   // одразу йдемо вниз
      flagSaveBaseBrightness = true;
    }

    if (currentMillis - previousMillis >= strobeTick) {
      previousMillis = currentMillis;

      // плавна зміна
      effectBrightness += fadeAmount;

      // переворот на межах
      if (effectBrightness <= 0) {
        effectBrightness = 0;
        fadeAmount = +1;
      } else if (effectBrightness >= baseBrightness) {
        effectBrightness = baseBrightness;
        fadeAmount = -1;
      }

      // застосування
      led.setBrightnessCRT(effectBrightness);
    }

  } else {
    // повертаємо звичайну яскравість при виході з режиму
    if (flagSaveBaseBrightness) {
      led.setBrightness(baseBrightness);
      flagSaveBaseBrightness = false;
    }
  }

  if (mode == 3 && isGarlandOn == 1) {
    int brightness = 0;
    if (currentMillis - previousMillis >= fadeTick) {
      previousMillis = currentMillis;

     


      if (greenValue < 255 && redValue == 255 && blueValue == 0) {
        greenValue++;
      } else if (greenValue == 255 && redValue > 0 && blueValue == 0) {
        redValue--;
      } else if (greenValue == 255 && redValue == 0 && blueValue < 255) {
        blueValue++;
      } else if (greenValue > 0 && redValue == 0 && blueValue == 255) {
        greenValue--;
      } else if (greenValue == 0 && redValue < 255 && blueValue == 255) {
        redValue++;
      } else if (greenValue == 0 && redValue == 255 && blueValue > 0) {
        blueValue--;
      }
      saveColor_R = redValue;
      saveColor_G = greenValue;
      saveColor_B = blueValue;
      led.setRGB_255(redValue,greenValue,blueValue);
    }
  }




  // Роздільна затримка для режиму "smooth"
   if (mode == 4 && isGarlandOn == 1) {
     if (currentMillis - previousMillis >= smoothTick) {
       previousMillis = currentMillis;

       if (smoothStep == 0) {
         led.setRGB_255(255,0,0);
       } else if (smoothStep == 1) {
         led.setRGB_255(0,255,0);
       } else if (smoothStep == 2) {
         led.setRGB_255(0,0,255);
       }
       smoothStep = (smoothStep + 1) % 3;  // Збільшуємо крок (0, 1, 2) і циклічно переходимо через всі кольори
     }
   }
}






void readStoredCredentials() {
  if (EEPROM.read(0) != 'W') {
    memset(&wifiSettings, 0, sizeof(wifiSettings));
    EEPROM.put(1, wifiSettings);
    EEPROM.write(0, 'W');
    EEPROM.commit();
  }
  EEPROM.get(1, wifiSettings);
  Serial.println("SSID з EEPROM: " + String(wifiSettings.ssid));
  Serial.println("Password з EEPROM: " + String(wifiSettings.password));

  WiFi.begin(wifiSettings.ssid, wifiSettings.password);
    if (WiFi.waitForConnectResult() == WL_CONNECTED) {
      Serial.println("Wi-Fi підключено");
      Serial.println("IP: " + WiFi.localIP().toString());
    } else {
      Serial.println("Не вдалось підключитись до збереженої мережі");
      isWiFiOn = true;
      WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
      WiFi.softAP(apSSID, apPassword);
    }
}

void handleConnect() {
  String ssidInput = webServer.arg("ssid");
  String passwordInput = webServer.arg("password");

  Serial.println("Нове з'єднання:");
  Serial.println("SSID: " + ssidInput);
  Serial.println("Password: " + passwordInput);

  WiFi.begin(ssidInput.c_str(), passwordInput.c_str());

  if (WiFi.waitForConnectResult() == WL_CONNECTED) {
    ssidInput.toCharArray(wifiSettings.ssid, 41);
    passwordInput.toCharArray(wifiSettings.password, 41);
    EEPROM.put(1, wifiSettings);
    EEPROM.commit();

    Serial.println("Wi-Fi збережено");
    Serial.println("IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("Помилка з'єднання");
  }
}


void checkBTN(){
  sysBtn.tick();
  
  static unsigned long lastClientCheck = 0;  // остання перевірка клієнтів
  static unsigned long apActivatedTime = 0;  // час активації AP
  
  
  static unsigned long lastUpdate = 0;
  static int brightness = 0;
  static int fadeAmount = 5;


  if(sysBtn.isTriple()){
    Serial.println("=== ESP8266 Information ===");

    Serial.print("Chip ID: ");
    Serial.println(ESP.getChipId());

    Serial.print("CPU Frequency: ");
    Serial.print(ESP.getCpuFreqMHz());
    Serial.println(" MHz");

    Serial.print("Flash Size: ");
    Serial.print(ESP.getFlashChipSize() / 1024);
    Serial.println(" KB");

    Serial.print("Sketch Size: ");
    Serial.print(ESP.getSketchSize() / 1024);
    Serial.println(" KB");

    Serial.print("Free Sketch Space: ");
    Serial.print(ESP.getFreeSketchSpace() / 1024);
    Serial.println(" KB");

    Serial.print("Flash Chip Real Size: ");
    Serial.print(ESP.getFlashChipRealSize() / 1024);
    Serial.println(" KB");

    Serial.print("Flash Chip Speed: ");
    Serial.print(ESP.getFlashChipSpeed() / 1000000.0);
    Serial.println(" MHz");

    Serial.print("Boot Version: ");
    Serial.println(ESP.getBootVersion());

    Serial.print("Boot Mode: ");
    Serial.println(ESP.getBootMode());

    Serial.print("Boot Reason: ");
    Serial.println(ESP.getResetInfo());

    Serial.print("SDK Version: ");
    Serial.println(ESP.getSdkVersion());

    Serial.print("Core Version: ");
    Serial.println(ESP.getCoreVersion());

    Serial.print("Free Heap: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");

    Serial.print("Flash Mode: ");
    Serial.println(ESP.getFlashChipMode());

    Serial.println("===========================");
  }

  static unsigned long pressStart = 0;
  static bool apStarted = false;  // флаг запуску AP

  if (sysBtn.isHold()) {
    if (pressStart == 0) {
      // фіксуємо момент першого утримання
      pressStart = millis();
      apStarted = false; // кожне нове натискання починаємо з нуля
    }

    if (!apStarted && (millis() - pressStart >= 10000)) {
      // 10 секунд минуло — запускаємо AP
      WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

      String ssidName = apSSID;
      if (WiFi.status() == WL_CONNECTED) {
        ssidName += " " + WiFi.localIP().toString();
        if (ssidName.length() > 31) ssidName = ssidName.substring(0, 31);
      }

      WiFi.softAP(ssidName.c_str(), apPassword);
      isWiFiOn = true;
      apActivatedTime = millis();
      dnsServer.start(DNS_PORT, "*", apIP);
      Serial.println("Точка доступу активована: " + ssidName);

      apStarted = true;  // тепер більше не спамить
    }

  } else {
    // кнопка відпущена — скидаємо таймер
    pressStart = 0;
  }



  // --- Плавне блимання світлодіода ---

  if (isWiFiOn) {
    dnsServer.processNextRequest();
    if (millis() - lastUpdate > 15) {
      lastUpdate = millis();
      brightness += fadeAmount;
      if (brightness <= 0 || brightness >= 255) fadeAmount = -fadeAmount;
      analogWrite(LED_PIN,brightness);
    }
  } else {
    analogWrite(LED_PIN, 255); // постійно світло
  }

  // --- Перевірка клієнтів AP кожні 1 сек ---
  if (isWiFiOn && (millis() - lastClientCheck > 1000)) {
    lastClientCheck = millis();
    int clients = WiFi.softAPgetStationNum();
    if (clients > 0) {
      apActivatedTime = millis();
    } else {
      if (millis() - apActivatedTime > 30000) {
        WiFi.softAPdisconnect(true);
        isWiFiOn = false;
        analogWrite(LED_PIN, 0);
        Serial.println("Точка доступу вимкнена через відсутність клієнтів");
      }
    }
  }
}


void handleGetIpStan(){
  if (WiFi.status() == WL_CONNECTED){
    webServer.send(200, "text/plain", WiFi.localIP().toString());
  }else{
    webServer.send(200, "text/plain", "0");
  }
}

void sendData() {
  String data = String(led.getValue_RED()) + "," + 
                String(led.getValue_GREEN()) + "," + 
                String(led.getValue_BLUE());
  webServer.send(200, "text/plain", data);
}

void sendBringhtness(){
  webServer.send(200, "text/plain", String(led.getBrightness()));
}



void setup() {
  EEPROM.begin(128); 
  Serial.begin(115200);


  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN,HIGH);



  if (MDNS.begin(PORTAL_NAME)) {
    Serial.println("mDNS responder started");
  } else {
    Serial.println("Error setting up MDNS responder!");
  }
  dnsServer.start(DNS_PORT, "*", apIP);
  readStoredCredentials();

  webServer.on("/", handleRoot);
  webServer.on("/connect", handleConnect);
  webServer.on("/getIpStan", HTTP_GET,handleGetIpStan);
  webServer.on("/sendBringhtness", HTTP_GET, sendBringhtness);
  webServer.on("/sendData", HTTP_GET, sendData);
  
  webServer.onNotFound([]() {
    webServer.sendHeader("Location", "/");
    webServer.send(302, "text/plain", "");
  });

  webServer.begin();


  pinMode(IR_PIN, INPUT_PULLUP);
  irrecv.enableIRIn();

  led.setRGB_255(saveColor_R, saveColor_G, saveColor_B);

}

void setModeIR(const uint64_t value_IR){
/*  if (color != ""  && isGarlandOn) {
    mode = 0;
    led.setHex(color.c_str());
    saveColor_R = led.getValue_RED();
    saveColor_G = led.getValue_GREEN();
    saveColor_B = led.getValue_BLUE();
  }

  
  if (value_IR == 16220287 && isGarlandOn == 1 && flashTick < 2000 && mode == 1) {
    flashTick = flashTick + 20;
    Serial.println(flashTick);
  } else if (value_IR == 16187647 && isGarlandOn == 1 && flashTick > 20 && flashTick != 20 && mode == 1) {
    flashTick = flashTick - 20;
    Serial.println(flashTick);
  }


  if (value_IR == 16220287 && isGarlandOn == 1 && strobeTick < 100 && mode == 2) {
    strobeTick = strobeTick + 2;
    Serial.println(strobeTick);
  } else if (value_IR == 16187647 && isGarlandOn == 1 && strobeTick > 6 && strobeTick != 7 && mode == 2) {
    strobeTick = strobeTick - 2;
    Serial.println(strobeTick);
  }


  if (value_IR == 16220287 && isGarlandOn == 1 && fadeTick < 50 && mode == 3) {
    fadeTick = fadeTick + 5;
    Serial.println(fadeTick);
  } else if (value_IR == 16187647 && isGarlandOn == 1 && fadeTick > 0 && fadeTick != 7 && mode == 3) {
    fadeTick = fadeTick - 5;
    Serial.println(fadeTick);
  }

  if (value_IR == 16220287 && isGarlandOn == 1 && smoothTick < 2000 && mode == 4) {
    smoothTick = smoothTick + 20;
    Serial.println(smoothTick);
  } else if (value_IR == 16187647 && isGarlandOn == 1 && smoothTick > 20 && smoothTick != 20 && mode == 4) {
    smoothTick = smoothTick - 20;
    Serial.println(smoothTick);
  }





  if (value_IR == 16236607){
    isGarlandOn = true;
    led.setRGB_255(saveColor_R,saveColor_G,saveColor_B);
  }else if (value_IR == 16203967){
    isGarlandOn = false;
    led.setRGB_255(0, 0, 0);
  }
  else if (value_IR == 16240687) mode = 1;
  else if (value_IR == 16248847) mode = 2;
  else if (value_IR == 16238647) mode = 3;
  else if (value_IR == 16246807) mode = 4;

  */

  if (value_IR == 16195807 && isGarlandOn == 1){ led.setHex("#FF0000"); mode = 0;}
  else if (value_IR == 16228447 && isGarlandOn == 1){ led.setHex("#008000"); mode = 0;}
  else if (value_IR == 16212127 && isGarlandOn == 1){ led.setHex("#00008B"); mode = 0;}
  else if (value_IR == 16191727 && isGarlandOn == 1){ led.setHex("#FF8C00"); mode = 0;}
  else if (value_IR == 16224367 && isGarlandOn == 1){ led.setHex("#00FF00"); mode = 0;}
  else if (value_IR == 16208047 && isGarlandOn == 1){ led.setHex("#0000FF"); mode = 0;}
  else if (value_IR == 16199887 && isGarlandOn == 1){ led.setHex("#FFA500"); mode = 0;}
  else if (value_IR == 16232527 && isGarlandOn == 1){ led.setHex("#20B2AA"); mode = 0;}
  else if (value_IR == 16216207 && isGarlandOn == 1){ led.setHex("#800080"); mode = 0;}
  else if (value_IR == 16189687 && isGarlandOn == 1){ led.setHex("#FFD700"); mode = 0;}
  else if (value_IR == 16222327 && isGarlandOn == 1){ led.setHex("#008B8B"); mode = 0;}
  else if (value_IR == 16206007 && isGarlandOn == 1){ led.setHex("#FF00FF"); mode = 0;}
  else if (value_IR == 16197847 && isGarlandOn == 1){ led.setHex("#FFFF00"); mode = 0;}
  else if (value_IR == 16230487 && isGarlandOn == 1){ led.setHex("#008080"); mode = 0;}
  else if (value_IR == 16214167 && isGarlandOn == 1){ led.setHex("#EE82EE"); mode = 0;}
  else if (value_IR == 16244767 && isGarlandOn == 1){ led.setHex("#FFFFFF"); mode = 0;}


  else if (value_IR == 16220287 && isGarlandOn == 1 && flashTick < 2000 && mode == 1) flashTick += 20; 
  else if (value_IR == 16187647 && isGarlandOn == 1 && flashTick > 20 && flashTick != 20 && mode == 1) flashTick -= 20;

  else if (value_IR == 16220287 && isGarlandOn == 1 && strobeTick < 100 && mode == 2) strobeTick += 2; 
  else if (value_IR == 16187647 && isGarlandOn == 1 && strobeTick > 6 && strobeTick != 7 && mode == 2) strobeTick -= 2;

  else if (value_IR == 16220287 && isGarlandOn == 1 && fadeTick < 50 && mode == 3) fadeTick += 5; 
  else if (value_IR == 16187647 && isGarlandOn == 1 && fadeTick > 0 && fadeTick != 7 && mode == 3) fadeTick -= 5;

  else if (value_IR == 16220287 && isGarlandOn == 1 && smoothTick < 2000 && mode == 4) smoothTick += 20; 
  else if (value_IR == 16187647 && isGarlandOn == 1 && smoothTick > 20 && smoothTick != 20 && mode == 4) smoothTick -= 20;

  else if (value_IR == 16236607) { isGarlandOn = true; led.setRGB_255(saveColor_R, saveColor_G, saveColor_B); saveColor_R = led.getValue_RED(); saveColor_G = led.getValue_GREEN(); saveColor_B = led.getValue_BLUE(); } 
  else if (value_IR == 16203967) { isGarlandOn = false; led.setRGB_255(0, 0, 0); } 

  else if (value_IR == 16240687) mode = 1; 
  else if (value_IR == 16248847) mode = 2; 
  else if (value_IR == 16238647) mode = 3; 
  else if (value_IR == 16246807) mode = 4;
  webServer.handleClient();
}





void tickIR(){
  static unsigned long saveTimeUpdate = 0;
  if(millis() - saveTimeUpdate >= 100){
     if (irrecv.decode(&results)) {
      setModeIR(results.value);
      webServer.handleClient();
      irrecv.resume(); // готувати приймач до наступного сигналу
    }
    saveTimeUpdate = millis();
  }
}



void loop() {
  checkBTN();
  webServer.handleClient();
  MDNS.update();
  modes_led();
  tickIR();
  yield();
}



