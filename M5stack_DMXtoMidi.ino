///////////////////////////////////////////////////////
/// M5Stack Core S3 DMX-MIDI Converter by DenkyTuna ///
/// *Use CoreS3 (ESP32-S3) !                        ///
/// *Make sure Arduino IDE Setting!                 ///
///  - USB CDC On Boot: Enabled                     ///
///  - Upload Mode: "USB-OTG CDC (TinyUSB)"         ///
///  - USB Mode: "USB-OTG (TinyUSB)"                ///
/// *When using above setting, turn M5CoreS3 into   ///
///  "download mode" to write the program.          ///
///  Press reset button for 3sec                    ///
///  and release after green LED flashed.           ///
///////////////////////////////////////////////////////

// 0 = Pass-through, 1 = Separate. Define according to your hardware setting.
// If you use M5 DMX Module and S2 switch = Pass-through, define 0. Otherwise 1.
// M5 DMX Module の S2 スイッチを in/out 直結で使っている場合0, 分離で使っている場合1
#define SEPARATEMODE 1

#include <M5Unified.hpp>  // https://github.com/M5Stack/M5Unified/
#include <esp_dmx.h>      // https://github.com/someweisguy/esp_dmx/ v3.1.0
#include <Adafruit_TinyUSB.h>
#include <MIDI.h>

// USB MIDI object
Adafruit_USBD_MIDI usb_midi;

// Create a new instance of the Arduino MIDI Library,
// and attach usb_midi as the transport.
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);

// To compare previous MIDI array 直前のMIDIと比較して同じなら送らない用
uint8_t midiout_prev[DMX_PACKET_SIZE] = {};

// Note or CC
bool midinotemode = 1;

// DMX Setting
// M5Stack CoreS3 DMX Module pin setting: TX-7  RX-10  EN-6
gpio_num_t transmitPin = GPIO_NUM_7;
gpio_num_t receivePin  = GPIO_NUM_10;
gpio_num_t enablePin   = GPIO_NUM_6;

// Separate mode: separate Tx/Rx, using different serial port
static constexpr const dmx_port_t dmxTxPort = 1;
static constexpr const dmx_port_t dmxRxPort = 2;

// DMX data array 送受信データ
uint8_t in_data[DMX_PACKET_SIZE];
uint8_t out_data[DMX_PACKET_SIZE];

// Timer for debug purpose デバッグ用
//unsigned long lastUpdate = millis();

void setup(void) {
  M5.begin();

  // for debug
  //Serial.begin(115200);

  // MIDI Setting
  MIDI.begin(MIDI_CHANNEL_OMNI);

  dmx_config_t dmxConfig = DMX_CONFIG_DEFAULT;
    
  // driver install for each Rx, Tx port. esp_dmxドライバを送受信それぞれ準備する。
  dmx_driver_install(dmxTxPort, &dmxConfig, DMX_INTR_FLAGS_DEFAULT);
  dmx_driver_install(dmxRxPort, &dmxConfig, DMX_INTR_FLAGS_DEFAULT);

  // Set pin numbers. ピンの設定
  dmx_set_pin(dmxTxPort, transmitPin, -1, enablePin);
  dmx_set_pin(dmxRxPort, -1, receivePin, -1);

  while (!TinyUSBDevice.mounted()) {
    delay(500);
    }
}

void loop(void) {
  
  // For debug
  //unsigned long now = millis();

  dmx_packet_t packet;

// Receive DMX data from Rx. RX側からDMXデータを受信する。
  if (dmx_receive(dmxRxPort, &packet, DMX_TIMEOUT_TICK)) {
      if (!packet.err) {
          dmx_read(dmxRxPort, in_data, packet.size);
          
          for (int j = 1; j <= packet.size; j++) {
            uint8_t v1 = j % 100;
            uint8_t v2 = in_data[j] * 127 / 255; //scale DMX (0-255) to midi (0-127)
            uint8_t ch = j / 100 + 1;
            if (v2 != midiout_prev[j]) {
              switch (midinotemode) {
                case 0: //CC mode
                  MIDI.sendControlChange(v1, v2, ch); //cc, value, channel
                  break;
                case 1: //Note mode
                  MIDI.sendNoteOn(v1, v2, ch); //note, velo, channel
                  break;
              }
              midiout_prev[j] = v2;
            }
          }

          // Copy input data to output (only for separate mode).
          memcpy(out_data, in_data, DMX_PACKET_SIZE);

      }
  } 

// Send DMX data from Tx.(only for separate mode). 
// TX側からDMXデータを送信する。(セパレートモードのみ)
#if SEPARATEMODE == 1
  dmx_write(dmxTxPort, out_data, DMX_PACKET_SIZE);

  if (ESP_ERR_TIMEOUT != dmx_wait_sent(dmxTxPort, DMX_TIMEOUT_TICK)) {
      dmx_send(dmxTxPort, DMX_PACKET_SIZE);
  }

#else
#endif

}