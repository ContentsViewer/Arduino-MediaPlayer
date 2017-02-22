/*
*WAVEファイル操作用関数(Arduino用)
*  最終更新日 10.24.2016
*
*  説明:
*    SDカードにあるWAVEファイルを扱います.
*    PCM構造体への値の代入を主に処理します.
*
*  使用例とその説明:
*    //使用するWAVEファイル「mario_1.wav」: 8bit, 32khz
*    #include <SD.h>
*    #include <SPI.h>
*
*    #include "WaveArduino.h"
*
*    MONO_PCM pcm0;  //*1
*
*    void setup() {
*
*      pinMode(10, OUTPUT);
*      SD.begin(10);
*
*      //TIMER PWM設定
*      DDRD  |=  B00001000;                             // PD3 (OC2B) (Arduino D3)
*      TCCR2A = _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);  //8bit 高速PWM
*      TCCR2B = _BV(CS20);  //分周なし
*    }
*
*    void loop() {
*      //PCM構造体の初期化
*      pcm0.bufIndex = 0;
*      pcm0.bufPage = 0;
*      pcm0.readNextPage = 1;
*      overlapCnt = 0;
*
*      //WAVファイル読み込み
*      monoWaveRead(&pcm0, "mario_1.wav", &fp);  //*2
*
*      //タイマー割り込み開始
*      TIMSK2 |= _BV(TOIE2);
*
*      //音データ順次読み込み
*      while (TIMSK2 & _BV(TOIE2))
*      {
*        monoWaveSoundRead(&pcm0, &fp);  //*3
*      }
*
*      //再生終了
*      OCR2B = 0;
*      fp.close();
*      delay(5000);
*
*    }
*
*  //サウンドデータを再生します
*  ISR(TIMER2_OVF_vect)
*  {
*    OCR2B = pcm0.buf[pcm0.bufPage][pcm0.bufIndex++];  // データをPWMとして出力
*
*    //再生位置がbufの最後に達したとき
*    if (pcm0.bufIndex == pcm0.readSize[pcm0.bufPage])
*    {
*      //曲の最後まで来たとき
*      if (pcm0.bufIndex != MEDIA_BUF_SIZE)
*      {
*        TIMSK2 &= ~_BV(TOIE2);
*      }
*
*      pcm0.bufIndex = 0;  //再生位置をリセット
*      pcm0.bufPage ^= 1;  //bufのpageを切り替え
*      pcm0.readNextPage = 1;    //bufの読み込みを開始 *4
*    }
*  }
*
*  以下説明:
*    *1:
*      MOONO_PCM構造体を使用して変数pcm0を作成
*
*    *2:
*      これを実行することで以下のことがおこります.
*        変数pcm0に「標本化周波数」「量子化精度」「音データの長さ」代入されます.
*        変数pcm0.bufに音データが読み込まれます.
*          pcm.bufには2つのページがあります
*          1つのページにつきマクロ定義された「MEDIA_BUF_SIZE」分の要素をもちます.
*
*    *3:
*      この関数は以下のことをしています.
*        pcm.flg = 1のときに音データを読み込みます.
*          読み込み先はpcm.bufです.
*            bufのページはすでに再生されたものです. 現在再生中に使われているページ以外のもう一つのページです.
*        読み込みを終えると, pcm.flg = 0にします.
*
*    *4:
*      bufの読み込みとbuf再生の関係
*        bufの読み込みとbufの再生は同時に行っています.
*          page0 の bufを再生しているとき, page1 の bufに音データを書き込みます
*          再生位置がpage0 の bufの最後にきたとき, page1 の bufを再生します; このときpage1 の buf には音データがすでに書き込まれています.
*          page1 の buf を読み込み始めるのと同時に, page0 の buf に音データを書き込みます
*
*   更新履歴:
*       11.25.2016:
*           プログラムの完成:
*
*       4.22.2016:
*           スクリプト修正
*           
*       10.24.2016:
*           ArduinoIDE 1.6系に対応
*/
#include <SD.h>
#include <SPI.h>
#include "Arduino.h"

//384が好ましい
#define MEDIA_BUF_SIZE 320

typedef struct
{
    //標本化周波数
    int fs;

    //量子化精度
    byte bits;

    //音データの長さ
    int sLength;

    byte buf[2][MEDIA_BUF_SIZE];
    byte bufPage;
    byte readNextPage;

    //再生位置
    short bufIndex;

    short readSize[2];

} MONO_PCM;


//関数のプロタイプ宣言
extern void MonoWaveRead(MONO_PCM *pcm, char *fileName, File *fp);
extern void MonoWaveSoundRead(MONO_PCM *pcm, File *fp);

//===コード======================================================================================
//
//関数: void mono_wave_sound_read(MONO_PCM *pcm, File *fp)
//  説明:
//    WAVEファイルのサウンド情報を読み込みます.
//
//
//  補足説明:
//    この関数を実行する前に関数「void mono_wave_read(MONO_PCM *pcm, char *file_name, File *fp)」を呼ぶ必要があります.
//
void MonoWaveSoundRead(MONO_PCM *pcm, File *fp)
{
    //読み込みフラグがたったときにサウンドデータを読み込む
    if (pcm -> readNextPage)
    {
        pcm -> readSize[pcm -> bufPage ^ 1] = (*fp).read(pcm -> buf[pcm -> bufPage ^ 1], MEDIA_BUF_SIZE); //音データの読み取り
        pcm -> readNextPage = 0;
    }
}

//
//関数: void mono_wave_read(MONO_PCM *pcm, char *file_name, File *fp)
//  説明:
//    WAVEファイルの情報を読み込みます.
// 
void MonoWaveRead(MONO_PCM *pcm, char *fileName, File *fp)
{
    int n;
    char riffChunkID[4];
    long riffChunkSize;
    char riffFormType[4];
    char fmtChunkID[4];
    long fmtChunkSize;
    short fmtWaveFormatType;
    short fmtChannel;
    long fmtSamplesPerSec;
    long fmtBytesPerSec;
    short fmtBlockSize;
    short fmtBitsPerSample;
    char dataChunkID[4];
    long dataChunkSize;

    *fp = SD.open(fileName);

    (*fp).read(riffChunkID, 4);
    (*fp).read(&riffChunkSize, 4);
    (*fp).read(riffFormType, 4);
    (*fp).read(fmtChunkID, 4);
    (*fp).read(&fmtChunkSize, 4);
    (*fp).read(&fmtWaveFormatType, 2);
    (*fp).read(&fmtChannel, 2);
    (*fp).read(&fmtSamplesPerSec, 4);
    (*fp).read(&fmtBytesPerSec, 4);
    (*fp).read(&fmtBlockSize, 2);
    (*fp).read(&fmtBitsPerSample, 2);
    (*fp).read(dataChunkID, 4);
    (*fp).read(&dataChunkSize, 4);

    pcm -> fs = fmtSamplesPerSec;  //標本化周波数
    pcm -> bits = fmtBitsPerSample; //量子化精度
    pcm -> sLength = dataChunkSize / ((pcm -> bits) / 8); //音データの長さ

    //音データをpage0 bufに読み込む, page1 bufに読み込む
    pcm -> readSize[pcm -> bufPage] = (*fp).read(pcm -> buf[pcm -> bufPage], MEDIA_BUF_SIZE);
    MonoWaveSoundRead(pcm, fp);
}

