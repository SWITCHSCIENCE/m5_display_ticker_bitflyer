# m5_display_ticker_bitflyer
M5Stack Basic/Core2 にBitflyerティッカーを表示する

![スクリーンショット](screenshot.png "スクリーンショット")

## 使い方

プロビジョニングツール`ESP BLE Provisioning`([Android](https://play.google.com/store/apps/details?id=com.espressif.provble)/[iOS](https://apps.apple.com/in/app/esp-ble-provisioning/id1473590141))をスマートフォンにインストールし、M5Stackに表示されるQRコードを読み取りWiFi設定を行ってください。

WiFi接続に成功するとティッカー情報が表示されます。

WiFi設定をやり直したい場合、Aボタンを長押ししてください。M5Stackに保存されたWiFi設定がクリアされ再度QRコードが表示されます。

## ビルド環境

[arduino-cli](https://arduino.github.io/arduino-cli/0.19/installation/)をインストールする。

```sh
$ curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR=~/.local/bin sh
```
初期設定ファイルを作成する。
```sh
$ arduino-cli config init
Config file written to: /home/yusuke/.arduino15/arduino-cli.yaml
```
ボードマネージャのURLを追加する。
```
$ arduino-cli config add board_manager.additional_urls https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/arduino/package_m5stack_index.json
```
`M5Stack`用コアをインストールする。
```sh
$ arduino-cli core update-index
$ arduino-cli core install m5stack:esp32
$ arduino-cli core list
ID             Installed Latest Name
m5stack:esp32  2.0.2     2.0.2  M5Stack
```
`M5Core2`ライブラリをインストールする。
```sh
$ arduino-cli lib install --no-deps M5Core2@0.0.6
```
`M5Stack`ライブラリをインストールする。
```sh
$ arduino-cli lib install M5Stack
```
`ArduinoJson`ライブラリをインストールする。
```sh
$ arduino-cli lib install ArduinoJson
```
`ArduinoWebsockets`ライブラリをインストールする。
```sh
$ arduino-cli lib install ArduinoWebsockets
```
`spiffsgen.py`をダウンロードする。
```sh
$ wget https://raw.githubusercontent.com/espressif/esp-idf/d95b15c55740b417d1a935ac006dba4cfaeef3cf/components/spiffs/spiffsgen.py
```

## ビルドと書き込み

### M5Stack Basic (No OTA)
```sh
$ ./spiffsgen.py 2031616 data/ data_Basic.spiffs.bin
$ python "/home/yusuke/.arduino15/packages/esp32/tools/esptool_py/3.1.0/esptool.py" --chip esp32 --port "/dev/ttyS8" --baud 921600  --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect 0x210000 data_Basic.spiffs.bin
$ arduino-cli compile -b m5stack:esp32:m5stack-core-esp32 -v --build-property build.partitions=no_ota
$ arduino-cli upload  -b m5stack:esp32:m5stack-core-esp32 -v -p /dev/ttyS8
```

### M5Stack Core2 (Default)
```sh
$ ./spiffsgen.py 3604480 data/ data_Core2.spiffs.bin
$ python "/home/yusuke/.arduino15/packages/esp32/tools/esptool_py/3.1.0/esptool.py" --chip esp32 --port "/dev/ttyS9" --baud 921600  --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect 0xc90000 data_Core2.spiffs.bin
$ arduino-cli compile -b m5stack:esp32:m5stack-core2 -v
$ arduino-cli upload  -b m5stack:esp32:m5stack-core2 -v -p /dev/ttyS9
```
