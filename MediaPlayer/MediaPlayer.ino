/*
   WAVEファイル再生用Arudinoプログラム
    最終更新日: 12.13.2016

    説明:
      このプログラムを使うことで以下のことができます.
        microSDカード内にあるwave形式音楽ファイルを再生する.
   導入方法:
     下の対応ハードに対応した音楽再生用Arudinoを用意する.
     音楽再生用Arduinoにスケッチ'MediaPlayer'を書き込む
      このとき音声出力をPWMModeか8BitModeかを決める.
     音楽再生用Arudinoを操作するための別の命令送信用Arudinoを用意する.
     microSDカードアダプターを用意する.
     スピーカーを用意する.(圧電スピーカー, ダイナミックスピーカー)
     microSDカードとarudinoAを接続する.
       接続表:
         pin10: CS
         pin11: DI<MOSI>
         pin12: DO<MISO>
         pin13: CLK<SCK>
     スピーカーとarudinoAを接続する.
      PWMModeのとき:
       接続pin: DP3
      8BitModeのとき:
        DP0, DP1, DP2, DP3, DP4, DP5, DP6, DP7

     音楽再生用Arduinoと命令送信用Arduinoを接続する.
       接続表:
         (音楽再生用Arduino, 命令送信用Arduino)=
         (GND, GND), (DP8, StopPin), (DP9 *3_1, StatePin), (DP14, TX)

         注:
*          *3_1: 変更可能, 音楽再生用ヘッダファイル内を参照
    対応ハード:
      マイコン: ATMEGA
      クロック数: 8MHz, 16MHz; *4_1
      タイマー機能: 8bit
      PWM出力: あり
      メモリ容量: 2KB以上

      注:
*      *4_1: プログラム内で変更する箇所あり

    対応ファイル(WAVE)
      サンプリング周波数: 64KHz *5_2, 32KHz, 16KHz, 8KHz
      量子化精度: 8bit
      ファイル名:
        8文字 + 3文字
          例: (wrong): 「inuboku.wav」; (correct): 「inubok.wav」
      ファイル位置:
        ルートディレクトリ内におくこと *5_1

      注:
*      *5_1: 任意のフォルダーに入れることができる. 下のプログラムコード内を参照
*      *5_2: 64KHzは16MHzArudinoのみ対応
              64KHzを再生できない問題がある(1/4.2016)

    再生方法:
      MediaPlayerController内を参照

    更新履歴:
      12/17.2015:
        プログラムの完成

      12/31.2015
        再生周波数が本来の周波数より遅い問題を修正
        16MHzのArudinoに対応した

      1/4.2016:
        説明欄更新
        問題の発見:
          64KHzの音楽ファイルを再生できない

      2.3.2016:
        シリアルデータ受信部分を改良

      4.22.2016:
        スクリプト修正

      9.12.2016:
        Idle時に動作周波数のパルスが出力される問題を修正

      10.24.2016:
        8Bit出力に対応
        ArduinoIDE1.6系に対応

      12.13.2016:
        Controller変更に伴う修正

*/
#include <SD.h>
#include <SPI.h>
#include <SoftwareSerial.h>

#include "WaveArduino.h"

//命令送信用Arudinoと接続するpinを設定
//命令送信用Arudinoへ曲の再生情報を送信するときに使う
#define STATE_PIN_MEDIA 9

//補足
//PIN8は命令送信用ArduinoからStop命令を受け取る

//シリアル通信ピン
#define SERIAL_PIN_RX 14
#define SERIAL_PIN_TX 15

//使用するマイコンのクロック周波数に従って変更
//#define _DEFINE_8MHz_
#define _DEFINE_16MHz_

//音声出力モードの選択
//PWMMode:
//  DP3から音声信号をPWMとして出力します
//
//8BitMode:
//  DP0からDP7から音声信号を8bitで出力します
//  DP0の信号は音声情報の1bit目に対応します
#define _USE_PWM_MODE_
//#define _USE_8BIT_MODE_

File fp;
MONO_PCM pcm0;

SoftwareSerial mediaSerial(SERIAL_PIN_RX, SERIAL_PIN_TX);

//重複サウンド再生用変数; サンプリング周波数が異なるwaveファイルを扱うため
volatile byte overlapCnt;
byte overlap;

//サウンド再生状態格納用変数
//  0000000X:
//    再生状態
//      0: stop; 1: play
//
//  XXXXXXX0:
//    再生モード
//      0: 通常再生
//      1: ループ再生
//
volatile byte state;

//waveファイル名保存用変数; 8文字 + ".wav" + NULL = 13文字
char waveFileName[13];

//関数プロタイプ宣言
extern byte GetCh(void);
extern void SerialClear(void);
extern void Play(void);
extern void Stop(void);

void setup() {
  //ピン設定
#ifdef _USE_PWM_MODE_
  // PD3; OC2B; Arduino D3
  DDRD  |=  B00001000;
#endif

#ifdef _USE_8BIT_MODE_
  DDRD |= B11111111;
#endif

  //状態送信用ピン設定
  pinMode(STATE_PIN_MEDIA, OUTPUT);
  digitalWrite(STATE_PIN_MEDIA, LOW);

  //SDカード設定
  pinMode(10, OUTPUT);  //CS - PD10
  SD.begin(10);

  //TIMER2設定
  //高速PWM; コンペアマッチでLOW; TOP = OCR2A
#ifdef _USE_PWM_MODE_
  TCCR2A = _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);  //8bit 高速PWM
  OCR2B = 255; //PWM出力初期値255; 常に5Vを出して動作周波数のパルスを出さないようにする
#endif

#ifdef _USE_8BIT_MODE_
  TCCR2A = _BV(WGM21) | _BV(WGM20);
#endif

  TCCR2B = _BV(CS20) | _BV(WGM22);  //分周なし
  OCR2A = 249;  //16MHz / 64Khz = 250 (less one)

  //シリアル通信開始
  mediaSerial.begin(19200);

  //変数初期化
  state = 0x00;
}

void loop() {
  //(ループモードかつPlay)ではないとき
  if (!((state & 0x02) && (state & 0x01)))
  {
    //メインマイコンに状態を送信
    digitalWrite(STATE_PIN_MEDIA, LOW);

    for (;;)
    {
      byte ch;
      ch = GetCh();

      //ControllerからLoad命令がきたとき
      if (ch == 1)
      {
        //play_mode読み込み
        state = GetCh() << 1;

        byte i;
        //waveファイル名を読み込む
        for (i = 0; i < 13; i++)
        {
          waveFileName[i] = GetCh();
          if (waveFileName[i] == '\0')
          {
            break;
          }
          //終端文字を追加する; オーバーフロー防止
          waveFileName[12] = '\0';
        }
      }

      //ControllerからPlay命令がきたとき
      else if (ch == 2)
      {
        state |= 0x01;
      }

      if (state & 0x01) break;  //受信データ処理を終了する; ループから抜ける

      //mediaSerial.println(waveFileName);
    }
    //Controllerに状態を送信
    digitalWrite(STATE_PIN_MEDIA, HIGH);

    //シリアルデータ削除
    SerialClear();
  }

  //サウンド再生開始
  Play();

}

//サウンド再生用関数
void Play(void)
{
  //waveファイルが存在するか確認する
  if (!SD.exists(waveFileName))
  {
    Stop();
    return;
  }
  
  //PCM構造体の初期化
  pcm0.bufIndex = 0;
  pcm0.bufPage = 0;
  pcm0.readNextPage = 1;
  overlapCnt = 0;

  //WAVファイル読み込み
  MonoWaveRead(&pcm0, waveFileName, &fp);

  //8MHzArudinoのとき
#ifdef _DEFINE_8MHz_
  overlap = (32000 / pcm0.fs);
#endif

  //16MHzArudinoのとき
#ifdef _DEFINE_16MHz_
  overlap = (64000 / pcm0.fs);
#endif

  //タイマー割り込み開始
  TIMSK2 |= _BV(TOIE2);

  //音データ順次読み込み
  while (TIMSK2 & _BV(TOIE2))
  {
    MonoWaveSoundRead(&pcm0, &fp);

    //予備
    MonoWaveSoundRead(&pcm0, &fp);
  }

  //再生終了
#ifdef _USE_PWM_MODE_
  OCR2B = 255;
#endif

#ifdef _USE_8BIT_MODE_
  PORTD = B00000000;
#endif
  fp.close();
}

//サウンドデータ再生割り込み
ISR(TIMER2_OVF_vect)
{
  if (PINB & B00000001)
  {
    Stop();
  }
  //データを出力
  if (!overlapCnt)
  {
#ifdef _USE_PWM_MODE_
    OCR2B = pcm0.buf[pcm0.bufPage][pcm0.bufIndex++];
#endif

#ifdef _USE_8BIT_MODE_
    PORTD = pcm0.buf[pcm0.bufPage][pcm0.bufIndex++];
#endif
  }
  overlapCnt++;
  if (overlapCnt >= overlap) overlapCnt = 0;
  //再生位置がbufの最後に達したとき
  if (pcm0.bufIndex == pcm0.readSize[pcm0.bufPage])
  {
    //曲の最後まで来たとき
    if (pcm0.bufIndex != MEDIA_BUF_SIZE)
    {
      TIMSK2 &= ~_BV(TOIE2);
    }

    pcm0.bufIndex = 0;  //再生位置をリセット
    pcm0.bufPage ^= 1;  //bufのpageを切り替え
    pcm0.readNextPage = 1;    //bufの読み込みを開始
  }
}

//サウンド停止割り込み
void Stop(void)
{
  state &= ~0x01;
  TIMSK2 &= ~_BV(TOIE2);
}

//
//関数: byte getch(void)
//  説明: optibootから送られてくるシリアルデータを受信します.
//        送られてくるまで待機します.
//
byte GetCh(void)
{
  int ch;
  for (;;)
  {
    ch = mediaSerial.read();
    if (ch != -1)
    {
      break;
    }
  }
  return (byte)ch;
}

//
//関数: void serialClear(void)
//  説明:
//    シリアルデータバッファを削除します.
//
void SerialClear(void)
{
  while (mediaSerial.read() != -1);
}
