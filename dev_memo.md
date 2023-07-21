# メモ

## プロジェクトの生成

pico-project-generatorを使う
pico_project.py --project vscode cube

includePathに修正が必要
環境変数をうまく読み込めていない？
../pico-sdk/** に

## ビルド

mkdir build
cd build
cmake ..
make cube

## SDカードの読み込み

FatFsを使う
プログラムはこれ
<https://qiita.com/Yukiya_Ishioka/items/6b5b6cb246f1d1e94461>
カードスロットはこれ
<https://akizukidenshi.com/download/ds/akizuki/K-5488_AE-MICRO-SD-DIP.pdf>
回路はこれ
<https://elehobica.blogspot.com/2021/03/raspberry-pi-picosdxcexfat-spi-if.html>
あと，ffconf.hでFF_FS_READONLYを1に

## USBシリアル通信

microUSBのみでシリアル通信をする

```CMakeLists.txt
pico_enable_stdio_uart(cube 0)
pico_enable_stdio_usb(cube 1)
```

stdio_init_all();

あとはstdinとstdoutがつながる

Macなら
screen /dev/tty.usbmodem14201 115200
で見れる
control+a, kで終了

non-blockingで受信したい
<https://ysin1128.hatenablog.com/entry/2021/08/25/143519#%E9%96%A2%E6%95%B0%E7%99%BA%E8%A6%8B>

uint32_t tud_cdc_available (void);
バッファにあるバイト数を取得

uint32_t tud_cdc_read (void* buffer, uint32_t bufsize);
bufsizeバイトだけバッファから読み取り，bufferに書き込む

uint32_t tud_cdc_write (void* buffer, uint32_t bufsize);
↑の書き込み版．送信はしない

uint32_t tud_cdc_write_flush (void);
バッファに書き込まれたものを送信する

## Windows上の環境構築

pico-setup-windowsをインストール
<https://github.com/raspberrypi/pico-setup-windows>

環境変数にPICO_SDK_PATHとPICO_TOOLCHAIN_PATHをセット
VSCodeにCMakeをインストール
「すべてのプロジェクトのビルド」でビルド

Pico - Visual Studio Code のショートカットから起動
