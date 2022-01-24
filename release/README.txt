コマンドプロンプトまたターミナルを開いてbasic(M5Stack Basic用)またはcore2(M5Stack Core2用)に移動し下記コマンドを実行してください。

MacOSの場合、
./upload.sh

Windowsの場合、
./upload.bat

うまく書き込めない場合、下記のようにポートを指定してください。

MacOSの場合、
./upload.sh "/dev/tty.usbserial****"

Windowsの場合、
./upload.bat COM8
