# Portable RPG Console

Raspberry Pi Pico W（RP2040）で動作する、**自作の携帯型RPGゲーム機**です。

ゲームロジックだけでなく、LCD描画、物理ボタン入力、音声再生、SDカードへのセーブ／ロード、振動、3Dプリント用の筐体・ボタンまで含めて制作しました。

## 🎮 Demo

[![Gameplay Demo](https://img.youtube.com/vi/t7yi3DA9XdE/hqdefault.jpg)](https://youtu.be/t7yi3DA9XdE)

**[YouTubeでプレイ動画を見る](https://youtu.be/t7yi3DA9XdE)**

## Features

- 迷路・ダンジョンの自動生成
- ターン制バトル / ボス戦
- 仲間・スキル・装備・進化システム
- アイテム / ショップ / 宿屋 / 仲間の預かり
- SDカードへのセーブ / ロード
- BGM / SE / 振動
- 3Dプリント用の筐体・ボタン

## Hardware

- Raspberry Pi Pico W / RP2040
- ILI9341 320 × 240 LCD
- SD Card
- DACによる音声出力
- Physical Buttons
- Vibration Motor

## Technical Highlights

### メモリ制約を考慮したLCD描画

320 × 240画面全体のフレームバッファを持つのではなく、**320 × 32 pxのline sprite**を使って画面を分割描画しています。

限られたRAM上で、フィールド・戦闘・各種メニューを描画できる構成にしています。

### RP2040のマルチコアを利用した音声処理

ゲーム処理とは別にRP2040のもう一方のcoreを音声処理に利用しています。

SDカードから読み込んだBGMの再生と、リアルタイムに生成するSEを組み合わせて出力しています。

### Procedural Map Generation

探索マップは固定マップだけでなく、アルゴリズムによる自動生成に対応しています。

迷路生成をベースに、部屋・階段・宝箱・敵とのエンカウントなどを組み合わせてダンジョンを構成しています。

### Software Architecture

開発初期は多くの処理が単一の `.ino` に集中していましたが、規模の拡大に合わせて責務を分離しました。

```text
Game
├─ GameSession
├─ Exploration / Battle / MenuFlow
├─ Game Systems
├─ Renderers
└─ Audio / Storage / Input / Graphics
```

ゲームロジックからLCD・SD・Audioなどのハードウェア依存処理を分離し、Rendererもゲーム状態を変更しないread-onlyな構造にしています。

最終的にはArduino側のentry pointをシンプルにし、ゲーム全体の制御を `Game` 以下へ集約しました。

```cpp
void setup() {
    Game::boot();
}

void loop() {
    ctx->update();
}
```

## Tech Stack

`C++` `Arduino` `RP2040` `LovyanGFX` `SdFat` `Fusion 360`