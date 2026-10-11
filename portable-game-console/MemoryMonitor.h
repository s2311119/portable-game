#pragma once

#include <Arduino.h>
#include <malloc.h>

class MemoryMonitor {
private:
  unsigned long lastCheckTime = 0;
  const unsigned long INTERVAL = 10000; // 10秒 (10000ms)

public:
  void update() {
    unsigned long now = millis();
    if (now - lastCheckTime >= INTERVAL) {
      lastCheckTime = now;
      printInfo();
    }
  }

  void printInfo() {
    struct mallinfo m = mallinfo();
    
    // RP2040のRAMは約264KBですが、プログラム領域などを除いた
    // ヒープとして使える領域の統計を表示します。
    
    Serial.println("=== Memory Status (10s) ===");
    Serial.printf("Used (使用中): %d bytes (%.2f KB)\n", m.uordblks, m.uordblks / 1024.0);
    Serial.printf("Free (空き)  : %d bytes (%.2f KB)\n", m.fordblks, m.fordblks / 1024.0);
    
    // メモリ断片化の目安（空きがあるのに大きい確保ができない原因）
    // これが小さいと、大きな連続したメモリ(画像など)が確保できません
    // Serial.printf("Max Free Block: %d bytes\n", m.mxordblk); // (環境によっては0になる場合あり)
    
    Serial.println("===========================");
  }
};
