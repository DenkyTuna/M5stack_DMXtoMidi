/*
    M5Stack Core S3 DMX to MIDI Converter

    The circuit:
    - M5Stack CoreS3
    - M5Stack Official DMX Base or equivalent circuit

    Created 2024/2/11
    By DenkyTuna
    Modified ---
    By ---

    Precautions:
    *Use CoreS3 (ESP32-S3) !     
    *Make sure Arduino IDE Setting is correct!
     - USB CDC On Boot: Enabled
     - Upload Mode: "USB-OTG CDC (TinyUSB)"
     - USB Mode: "USB-OTG (TinyUSB)"
    *When using above setting, turn M5CoreS3 into "download mode" to write the program.
     Press reset button for 3sec and release after green LED flashed.

    Variables should be named as lowerCamelCase.
    Don't use SOMEVALUE, some_value, somevalue
    Use       someValue

*/

// Dependencies
#include <M5Unified.hpp>  // https://github.com/M5Stack/M5Unified/ v0.1.12
#include <esp_dmx.h>      // https://github.com/someweisguy/esp_dmx/ v3.1.0
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>
#include <gob_unifiedButton.hpp> //https://github.com/GOB52/gob_unifiedButton/ v0.1.1

// Sprite
M5GFX display;
M5Canvas cvs1(&display);
M5Canvas cvs2(&display);
M5Canvas cvs3(&display);
M5Canvas cvsBtnGrid(&display);
M5Canvas cvsBtnLabel(&display);

// Button
goblib::UnifiedButton unifiedButton;

// Create a new instance of the Arduino MIDI Library,
// and attach usbMIDI as the transport.
Adafruit_USBD_MIDI usbMIDI;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usbMIDI, MIDI);

// Config menu
typedef struct {
  uint8_t ID;
  const char itemName[17];
  uint8_t curSel;
  char curSelStr[5];
  uint8_t numOfChoices;
  const char* choicesStr[2];
} dataDictionary;

dataDictionary configList[] {
//ID,  itemName         ,default,string,numOfChoices,choices as string
  {0, "DMX-Out Enable  ", 1,    "Yes",  2,           {"No", "Yes"}},
  {1, "MIDI Note or CC ", 0,    "CC",   2,           {"CC", "Note"}},
  {2, "Send Note Off   ", 0,    "No",   2,           {"No", "Yes"}}
};

// Pitch name
const char* pitchName[] = {"C ", "C#", "D ", "D#", "E ", "F ", "F#", "G ", "G#", "A ", "A#", "B "};

// DMX Setting
// M5Stack CoreS3 DMX Module pin setting: TX-7  RX-10  EN-6
gpio_num_t transmitPin = GPIO_NUM_7;
gpio_num_t receivePin  = GPIO_NUM_10;
gpio_num_t enablePin   = GPIO_NUM_6;

// Separate mode: separate Tx/Rx, using different serial port
static constexpr const dmx_port_t dmxTxPort = 1;
static constexpr const dmx_port_t dmxRxPort = 2;

// DMX data array 送受信データ
uint8_t inData[DMX_PACKET_SIZE];
uint8_t outData[DMX_PACKET_SIZE];

// To compare previous MIDI array 直前のMIDIと比較して同じなら送らない用
uint8_t midiOutPrev[DMX_PACKET_SIZE] = {};

// Backlight ON/OFF
bool backLightOn = true;

// mode setting - home or config screen
enum mode {home, config};
mode curMode = home;

// global variables for Config
int listSize = sizeof(configList) / sizeof(configList[0]);
int focus = 0;
bool selected = false;

// Timer for debug purpose デバッグ用
//unsigned long lastUpdate = millis();

void setup() {
  Serial.begin(115200);
  M5.begin();
  M5.Lcd.setBrightness(100);
  backLightOn = true;
  display.begin();
  display.clear(TFT_BLACK);

  // Create sprites. Full screen size is 320x240.
  cvs1.setColorDepth(8);
  cvs1.setTextFont(1);
  cvs1.setTextSize(2);
  cvs1.createSprite(display.width(),cvs1.fontHeight()+2);

  cvs2.setColorDepth(1);
  cvs2.setTextFont(1);
  cvs2.setTextSize(1);
  cvs2.createSprite(display.width(), cvs2.fontHeight()*17+1);
  cvs2.print("DMXCh|Value|MIDICh|Val1|Val2|Message             \n"); //53 characters per line
  cvs2.setTextScroll(true);
  cvs2.setScrollRect(0, cvs2.fontHeight(), cvs2.width(), cvs2.height());

  cvs3.setColorDepth(1);
  cvs3.setTextFont(1);
  cvs3.setTextSize(2);
  cvs3.createSprite(display.width(), cvs2.height());

  cvsBtnLabel.setColorDepth(1);
  cvsBtnLabel.setTextFont(1);
  cvsBtnLabel.setTextSize(1);
  cvsBtnLabel.createSprite(display.width(), cvsBtnLabel.fontHeight()*2+2);

  cvsBtnGrid.setColorDepth(1);
  cvsBtnGrid.setTextFont(1);
  cvsBtnGrid.setTextSize(1);
  cvsBtnGrid.createSprite(display.width(), cvsBtnLabel.fontHeight()*2+2);
  cvsBtnGrid.drawLine(0, 0, cvsBtnGrid.width(), 0, TFT_WHITE);
  cvsBtnGrid.drawLine(cvsBtnGrid.width()/3*1, 0, cvsBtnGrid.width()/3*1, cvsBtnGrid.height(), TFT_WHITE);
  cvsBtnGrid.drawLine(cvsBtnGrid.width()/3*2, 0, cvsBtnGrid.width()/3*2, cvsBtnGrid.height(), TFT_WHITE);

  // Create and customize buttons, see also https://github.com/m5stack/M5GFX/blob/master/src/lgfx/v1/LGFX_Button.hpp
  unifiedButton.begin(&display, goblib::UnifiedButton::appearance_t::custom);
  auto btnA = unifiedButton.getButtonA();
  assert(btnA);
  btnA->initButtonUL(
    unifiedButton.gfx(), //LovyanGFX *gfx
    display.width()/3*0, //int16_t x
    cvs1.height()+cvs2.height()+cvsBtnLabel.height()+1, //int16_t y
    display.width()/3, //uint16_t w
    display.height()-cvs1.height()-cvs2.height()-cvsBtnLabel.height()-1, //uint16_t h
    TFT_LIGHTGRAY, //const T& outline
    TFT_DARKGRAY, //const T& fill
    TFT_BLACK, //const T& textcolor
    "A", //const char *label
    2, //float textsize_x (defalt: 1.0f)
    2); //float textsize_y (default: 0.0f)
 
  auto btnB = unifiedButton.getButtonB();
  assert(btnB);
  btnB->initButtonUL(
    unifiedButton.gfx(),
    display.width()/3*1,
    cvs1.height()+cvs2.height()+cvsBtnLabel.height()+1,
    display.width()/3,
    display.height()-cvs1.height()-cvs2.height()-cvsBtnLabel.height()-1,
    TFT_LIGHTGRAY,
    TFT_DARKGRAY,
    TFT_BLACK,
    "B",
    2,
    2);

  auto btnC = unifiedButton.getButtonC();
  assert(btnC);
  btnC->initButtonUL(
    unifiedButton.gfx(),
    display.width()/3*2,
    cvs1.height()+cvs2.height()+cvsBtnLabel.height()+1,
    display.width()/3,
    display.height()-cvs1.height()-cvs2.height()-cvsBtnLabel.height()-1,
    TFT_LIGHTGRAY,
    TFT_DARKGRAY,
    TFT_BLACK,
    "C",
    2,
    2);

  unifiedButton.draw();

  // for debug
  //Serial.begin(115200);

  // MIDI Setting
  MIDI.begin(MIDI_CHANNEL_OMNI);
  dmx_config_t dmxConfig = DMX_CONFIG_DEFAULT;

  // DMX driver install for each Rx, Tx port. esp_dmxドライバを送受信それぞれ準備する。
  dmx_driver_install(dmxTxPort, &dmxConfig, DMX_INTR_FLAGS_DEFAULT);
  dmx_driver_install(dmxRxPort, &dmxConfig, DMX_INTR_FLAGS_DEFAULT);

  // Set pin numbers. ピンの設定
  dmx_set_pin(dmxTxPort, transmitPin, -1, enablePin);
  dmx_set_pin(dmxRxPort, -1, receivePin, -1);

  while (!TinyUSBDevice.mounted()) {
    delay(100);
    }

  cvs2.pushSprite(0, cvs1.height()+1, TFT_BLACK);
  prepHomeScreen();
}

void loop(void) {
  M5.update();
  unifiedButton.update();

  if (curMode == home) {
    dmxreceive();
  } else if (curMode == config) {
    configScreen();
  }

  unifiedButton.draw();
}

void dmxreceive(void) {
  // For debug
  //unsigned long now = millis();

  dmx_packet_t packet;

  // Receive DMX data from Rx. RX側からDMXデータを受信する。
  if (dmx_receive(dmxRxPort, &packet, DMX_TIMEOUT_TICK)) {
      if (!packet.err) {
          dmx_read(dmxRxPort, inData, packet.size);
          
          for (int j = 1; j <= packet.size; j++) {
            uint8_t v1 = j % 100;
            uint8_t v2 = inData[j] * 127 / 255; //scale DMX (0-255) to midi (0-127)
            uint8_t ch = j / 100 + 1;

            if (v2 != midiOutPrev[j]) {
               if (configList[1].curSel == 0) {
                  MIDI.sendControlChange(v1, v2, ch); //cc, value, channel
                  cvs2.printf("  %3d|  %3d|    %2d| %3d| %3d|    CC:%3d   Val:%3d \n",j,inData[j],ch,v1,v2,v1,v2);
               } else if (configList[1].curSel == 1) {
                  int8_t o = v1 / 12 - 1;
                  uint8_t p = v1 % 12;
                  if (v2 != 0) {
                     MIDI.sendNoteOn(v1, v2, ch); //note, velo, channel
                     cvs2.printf("  %3d|  %3d|    %2d| %3d| %3d|NoteOn:%s%2d Velo:%3d \n",j,inData[j],ch,v1,v2,pitchName[p],o,v2);
                  } else {
                     MIDI.sendNoteOff(v1, 0, ch); //note, velo, channel
                     cvs2.printf("  %3d|  %3d|    %2d| %3d| %3d|NoteOF:%s%2d Velo:%3d \n",j,inData[j],ch,v1,v2,pitchName[p],o,0);                   
                  }
                  if (configList[2].curSel == 1) {
                      if (v2 != 0) {
                         MIDI.sendNoteOff(v1, 0, ch);
                         cvs2.printf("  %3d|  %3d|    %2d| %3d| %3d|NoteOF:%s%2d Velo:%3d \n",j,inData[j],ch,v1,v2,pitchName[p],o,0);
                      }
                  }
               }
            }
              midiOutPrev[j] = v2;
          }
      }
  } 

  // Send DMX data from Tx.(only for separate mode). 
  // TX側からDMXデータを送信する。(セパレートモードのみ)
  if (configList[0].curSel == 1) {
    // Copy input data to output (only for separate mode).
    memcpy(outData, inData, DMX_PACKET_SIZE);
    dmx_write(dmxTxPort, outData, DMX_PACKET_SIZE);

    if (ESP_ERR_TIMEOUT != dmx_wait_sent(dmxTxPort, DMX_TIMEOUT_TICK)) {
      dmx_send(dmxTxPort, DMX_PACKET_SIZE);
    }
  }

  cvs2.pushSprite(0, cvs1.height()+1);

  if (M5.BtnA.wasReleased()) {
    if (backLightOn == true) {
      M5.Lcd.setBrightness(1);
      backLightOn = false;
    } else {
      M5.Lcd.setBrightness(100);
      backLightOn = true;
    }
  }

  if (M5.BtnB.wasReleased()) {
    prepConfScreen();
  }
}

void prepHomeScreen (void) {
  cvs1.clear(TFT_BLACK);
  cvs1.drawCentreString("DMX TO MIDI TOOL", cvs1.width()/2, 0, 1);
  cvs1.drawLine(0, cvs1.fontHeight()+1, cvs1.width(), cvs1.fontHeight()+1, TFT_WHITE);
  cvs1.pushSprite(0, 0);
  cvsBtnLabel.clear(TFT_BLACK);
  cvsBtnLabel.drawCentreString("Backlight Dim", cvsBtnLabel.width()*1/6, 1, 1);
  cvsBtnLabel.drawCentreString("Config", cvsBtnLabel.width()*3/6, 1, 1);
  cvsBtnLabel.drawCentreString(" ", cvsBtnLabel.width()*5/6, 1, 1);
  cvsBtnLabel.pushSprite(0, cvs1.height()+cvs2.height()+1);
  cvsBtnGrid.pushSprite(0, cvs1.height()+cvs2.height()+1, TFT_BLACK);  
  curMode = home;
}

void prepConfScreen(void) {
  cvs1.clear(TFT_GREEN);
  cvs1.drawCentreString("CONFIG", cvs1.width()/2, 0, 1);
  cvs1.drawLine(0, cvs1.fontHeight()+1, cvs1.width(), cvs1.fontHeight()+1, TFT_WHITE);
  cvs1.pushSprite(0, 0);
  cvsBtnLabel.clear(TFT_BLACK);
  cvsBtnLabel.drawCentreString("Go Down", cvsBtnLabel.width()*1/6, 1, 1);
  cvsBtnLabel.drawCentreString("Enter", cvsBtnLabel.width()*3/6, 1, 1);
  cvsBtnLabel.drawCentreString("Hold to Home", cvsBtnLabel.width()*3/6, cvsBtnLabel.fontHeight()+1, 1);
  cvsBtnLabel.drawCentreString("Change", cvsBtnLabel.width()*5/6, 1, 1);
  cvsBtnLabel.pushSprite(0, cvs1.height()+cvs2.height()+1);
  cvsBtnGrid.pushSprite(0, cvs1.height()+cvs2.height()+1, TFT_BLACK);
  focus = 0;
  selected = false; 
  curMode = config;
}

void configScreen(void) {
  cvs3.clear(TFT_BLACK);
  cvs3.setCursor(0, 0);
  
  for (int k = 0; k <= listSize - 1; k++) {
    if (focus == k && selected == false) {
      cvs3.print(">");
    } else {
      cvs3.print(" ");
    }
    cvs3.printf("%s:", configList[k].itemName);

    if (focus == k && selected == true) {
      cvs3.print(">");
    } else {
      cvs3.print(" ");
    }
    cvs3.printf("%s\n", configList[k].curSelStr);
  }

  if (M5.BtnA.wasReleased()) {
    if (selected == false) {
      focus = focus + 1;
      if (focus > listSize - 1) {
        focus = 0;
      }
    }
  }

  if (M5.BtnB.wasReleased()) {
    selected ^= 1;
  }

  if (M5.BtnB.wasReleaseFor(1000)) {
    prepHomeScreen();
  }

  if (M5.BtnC.wasReleased()) {
    if (selected == true) {
      uint8_t chosenItem = configList[focus].curSel;
      chosenItem = chosenItem + 1;
       if (chosenItem > configList[focus].numOfChoices - 1) {
        chosenItem = 0;
        }
      configList[focus].curSel = chosenItem;
      strcpy(configList[focus].curSelStr, configList[focus].choicesStr[chosenItem]);
    }
  }

  cvs3.pushSprite(0, cvs1.height()+1);
}