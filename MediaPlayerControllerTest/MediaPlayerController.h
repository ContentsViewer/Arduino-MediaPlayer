/*
*音楽再生用モジュール操作用関数
*  最終更新日: 10.24.2016
*
*  説明:
*    音楽再生用Arduinoを操作するための関数です.
*    音楽再生用Arudinoを用いています.
*
*  対応音楽ファイル:
*          対応ファイル(WAVE)
*            サンプリング周波数: 64KHz *5_2, 32KHz, 16KHz, 8KHz
*            量子化精度: 8bit
*            ファイル名:
*              8文字 + 3文字
*              例: (wrong): 「inuboku.wav」; (correct): 「inubok.wav」
*            ファイル位置:
*              ルートディレクトリ内におくこと *5_1
*
*            注:
*              *5_1: 任意のフォルダーに入れることができる. 下のプログラムコード内を参照
*              *5_2: 64KHzは16MHzArudinoのみ対応
*                    64KHzを再生できない問題がある(1/4.2016)
*                    
*  導入方法:
*    下の対応ハードに対応した音楽再生用Arudinoを用意する.
*    音楽再生用Arduinoにスケッチ'MediaPlayer'を書き込む
*     このとき音声出力をPWMModeか8BitModeかを決める.
*    音楽再生用Arudinoを操作するための別の命令送信用Arudinoを用意する.
*    microSDカードアダプターを用意する.
*    スピーカーを用意する.(圧電スピーカー, ダイナミックスピーカー)
*    microSDカードとarudinoAを接続する.
*      接続表:
*        pin10: CS
*        pin11: DI<MOSI>
*        pin12: DO<MISO>
*        pin13: CLK<SCK>
*    スピーカーとarudinoAを接続する.
*     PWMModeのとき:
*      接続pin: DP3
*     8BitModeのとき:
*       DP0, DP1, DP2, DP3, DP4, DP5, DP6, DP7
*       
*    音楽再生用Arduinoと命令送信用Arduinoを接続する.
*      接続表:
*        (音楽再生用Arduino, 命令送信用Arduino)=
*        (GND, GND), (DP8, StopPin), (DP9 *3_1, StatePin), (DP14, TX)
*
*        注:
*          *3_1: 変更可能, 音楽再生用ヘッダファイル内を参照
*  使用例:
*    wave音楽ファイル再生:
*      使用曲: 「sengok_1.wav」, 「PlaDis_3.wav」 *4_1
*
#include "MediaPlayerController.h"

MediaPlayerController mediaPlayerCtrl;

void setup() {
  mediaPlayerCtrl.Begin(12, 13);
}

void loop() {
  
  while(digitalRead(3));
  mediaPlayerCtrl.Stop();
  
  mediaPlayerCtrl.Load("sengok_1.wav", 2);
  mediaPlayerCtrl.Play();
  
  mediaPlayerCtrl.Load("PlaDis.wav", 1);
  mediaPlayerCtrl.Play();
  
  //delay(2000);
}

*      
*
*  補足:
*
*  更新履歴:
*    12.4.2015:
*      プログラムの完成
*
*    12.31.2015:
*      説明欄更新
*
*    1.4.2016:
*      説明欄更新
*      問題の発見:
*        64KHzの音楽ファイルを再生できない
*
*    2.3.2016:
*      myMEDIA_playerのシリアル通信開始待機を削除
*        myMEDIA_player側でシリアル通信を常にしているように変更した
*
*    4.22.2016:
*      スクリプト修正
*      
*    10.24.2016:
*       MediaPlayer修正にともなう変更
*       
*    12.13.2016:
*       Play関数のパフォーマンス改善
*       
*/

#include "Arduino.h"

class MediaPlayerController
{
private:
    byte stopPin;
    byte statePin;
    byte mode;

public:
    
    //
    //関数
    //  説明:
    //    MediaPlayerControllerを開始します
    //
    //  引数:
    //      stopPin:
    //        MediaPlayerにサウンド停止命令を送るピン
    //
    //      statePin:
    //        MediaPlayerから送られてくるサウンド再生状態を取得するピン
    //
    void Begin(byte stopPin, byte statePin)
    {
        this -> stopPin = stopPin;
        this -> statePin = statePin;

        pinMode(stopPin, OUTPUT);
        pinMode(statePin, INPUT);

        digitalWrite(stopPin, LOW);

        //シリアル通信開始
        Serial.begin(19200);
    }

    //
    //関数
    //  説明:
    //    MediaPlayerにサウンド停止命令を送ります
    //
    void Stop(void)
    {
        //サウンド停止命令を送る
        digitalWrite(stopPin, HIGH);
        while(digitalRead(statePin));
        digitalWrite(stopPin, LOW);
    }

    //関数
    //  説明:
    //    音楽ファイルをロードします
    //
    //  引数:
    //    waveFileName:
    //      音楽ファイル名
    //
    //    mode:
    //      0: 通常再生
    //      1: リピート再生
    //      2: サウンド停止待機
    //
    void Load(char *waveFileName, byte mode)
    {
        this -> mode = mode;

        //サウンド停止
        Stop();

        //Load命令を送る
        Serial.write(1);  //ヘッダは 1
        Serial.write(mode);  //再生モードを送信
        Serial.print(waveFileName);  //ファイル名を送る
        Serial.write(NULL);  //終端文字を追加する
    }


    //
    //関数
    //  説明:
    //    サウンドを再生します
    //
    void Play(void)
    {
        //サウンド停止
        Stop();

        //Play命令を送る
        Serial.write(2);  //コードは 2

        //再生状態になるまで待機
        while(!digitalRead(statePin));
        
        //状態送信待機
        //delay(100);

        //mode = 2のとき
        if (mode == 2)
        {
            //再生中のとき処理待機; STATE_PIN_MEDIAがLOWになったら処理再開
            while (digitalRead(statePin));
        }
    }
};



