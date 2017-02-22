/*
*プログラム
*   最終更新日: 10.24.2016
*   説明:
*     MediaPlayerControllerの動作をテストします.
*     音楽再生モジュールが必要
*/

#include "MediaPlayerController.h"

MediaPlayerController mediaPlayerCtrl;

void setup() {
  mediaPlayerCtrl.Begin(12, 13);
}

void loop() {
  
  while(digitalRead(3));
  mediaPlayerCtrl.Stop();
  mediaPlayerCtrl.Load("Wav.wav", 1);
  mediaPlayerCtrl.Play();

  delay(2000);

  while(digitalRead(3));
  mediaPlayerCtrl.Load("sengok_1.wav", 0);
  mediaPlayerCtrl.Play();

  delay(2000);

  while(digitalRead(3));
  mediaPlayerCtrl.Load("PlaDis.wav", 0);
  mediaPlayerCtrl.Play();
  delay(2000);

  while(digitalRead(3));
  mediaPlayerCtrl.Load("Gakko.wav", 0);
  mediaPlayerCtrl.Play();
  delay(2000);

  while(digitalRead(3));
  mediaPlayerCtrl.Load("inubok_3.wav", 0);
  mediaPlayerCtrl.Play();
  delay(2000);

  while(digitalRead(3));
  mediaPlayerCtrl.Load("PSO.wav", 0);
  mediaPlayerCtrl.Play();
  delay(2000);
}
