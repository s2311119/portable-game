#include "Graphic.h"

#include <LGFX_TFT_eSPI.hpp>

Graphic::Graphic() : lineSprite(&lcd) {}

void Graphic::init(){
    lcd.init();
    lcd.setRotation(1);
    lcd.setColorDepth(16);
    lcd.setSwapBytes(true);
    
    // ★ メモリ消費はたったの 20KB！ (320x32)
    lineSprite.setColorDepth(16);
    lineSprite.setSwapBytes(true);
    lineSprite.createSprite(SCREEN_WIDTH, BLOCK_HEIGHT);
    lineSprite.setFont(&fonts::lgfxJapanGothic_12);

    lcd.setFont(&fonts::lgfxJapanGothic_12);
    //lcd.fillScreen(TFT_BLACK); // 初期化時は黒埋め
  }
