# M5stack_DMXtoMidi
Convert DMX512 to USB-MIDI. Easiest way to turn your DMX console into MIDI fader.
Even people who are not good at soldering can use M5Stack's ready-made modules, assemble them like Lego blocks, write a program, and use it!
舞台照明で使われるDMX512信号を受信し、USB-MIDIに変換します。お手持ちのDMX卓をMIDIフェーダーとして使用できるようになります。
はんだ付けが苦手な人でも、M5Stackの既製品モジュールを使用してレゴブロックのように組み立てて、プログラムを書き込めばすぐに使用できます。

<img src="https://lightingkizai.com/wp-content/uploads/img_0065.jpg" width="256">
<img src="https://lightingkizai.com/wp-content/uploads/img_0064.jpg" width="256">

## What you need 必要なもの
*M5Stack CoreS3 (Be sure to use CoreS3. Only CoreS3 has ESP32-S3 as the main chip, which is mandatory for acting as USB-MIDI device.)
*M5Stack CoreS3 (必ずCoreS3を使用してください。CoreS3のみ、ESP32-S3というモジュールが使われており、これがUSB-MIDIとして振る舞うために必要です。)
*M5Stack DMX Base (or equivalent circuit configuration)
*M5Stack DMX Base (または同等の回路構成を自作したもの)

## How it works 動作内容
*USBで接続すると、「M5STACK_CORES3」という名前のMIDIデバイスとして認識されます。
*DMX信号を受信すると、MIDIのCC (ControlChange) またはノート/ベロシティに変換します。
*MIDIの慣例にしたがい、変化がない限り再送信はしません。
*DMXチャンネル番号の百の位をMIDIチャンネル、十の位以下をコントロールナンバーまたはノートナンバーに変換します。DMX値 (0-255) はMIDI値 (0-127) に自動でスケーリングされます。
*When connected via USB, it is recognized as a MIDI device named "M5STACK_CORES3".
*When it receives a DMX signal, it converts DMX to MIDI CC/Value or Note/Velocity.
*According to the convention of MIDI protocol, it will not retransmit unless there is a change.
*Converts hundred places of DMX channels to MIDI channels, tens and ones to MIDI CC Number or Note Number.
*Example 1: DMXCh 13, Value 255 -> MIDICh 1, CC 1, Value 127)
*Example 2: DMXCh 123, Value 0 -> MIDICh 2, CC 23, Value 0)
*設定画面にて、受信したDMXを本デバイスから再送信するかどうか、NoteにするかCCにするか、Noteの場合はノートオフを送信するかどうか、選択することができます。
*現在のところ変換方法は固定ですが、今後コントロールナンバーやノートナンバーを個別に変更できるようにしたいと思っています。
*On the config screen, you can select whether to resend the received DMX from this device, whether Note or CC, and if Note, whether to send a NoteOff.
*Currently the conversion method is fixed, but I would like to make it able to patch the control number and note number individually in the future.

## Precautions on writing program to M5Stack プログラム書き込み時の注意事項
*Make sure Arduino IDE Setting is correct!
*Arduino IDEの書き込み設定を確認してください。
    *USB CDC On Boot: Enabled
    *Upload Mode: "USB-OTG CDC (TinyUSB)"
    *USB Mode: "USB-OTG (TinyUSB)"
*When using above setting, turn M5CoreS3 into "download mode" to write the program. (Press reset button for 3sec and release after green LED flashed.)
*上記の設定を使用する場合、書き込み前に毎回、M5CoreS3を「ダウンロードモード」にする必要があります。(リセットボタンを3秒押して、緑色のLEDが光ったら離してください。)

## ToDo
*To be able to store config data to SPIFFS
*設定をSPIFFSに保存できるようにする
*Add MIDI patch function
*MIDIパッチ機能の追加
*Opposite direction (MIDI to DMX)
*反対方向 (MIDI to DMX)