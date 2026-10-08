#pragma once

#define LGFX_USE_V1

#include <LovyanGFX.hpp>

// LGFX for Waveshare RP2040-LCD-0.96
// https://www.waveshare.com/wiki/RP2040-LCD-0.96

class LGFX : public lgfx::LGFX_Device
{
  lgfx::Panel_ILI9341 _panel_instance;
  lgfx::Bus_SPI       _bus_instance;
  lgfx::Light_PWM     _light_instance;

  public:
  LGFX(void)
  {
    {
      auto cfg = _bus_instance.config();
      cfg.spi_host   = 1;
      cfg.spi_mode   = 0;
      cfg.freq_write = 40000000; //もし通信速度が速すぎて動かない場合は10000000等に変更
      cfg.pin_sclk   = 10; //ボードの設計に合わせて変更。SPI通信のクロック信号のポート
      cfg.pin_miso   = -1; //今回の液晶にはMISO端子は不要なため、-1のままにする
      cfg.pin_mosi   = 11; //ボードの設計に合わせて変更。SPI通信のMOSI(TX)信号のポート
      cfg.pin_dc     = 15;  //ボードの設計に合わせて変更。DC通信のポート。
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }

    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs       = 13; //ボードの設計に合わせて変更。CS信号のポート。
      cfg.pin_rst      = 14;//ボードの設計に合わせて変更。RST信号のポート。
      cfg.panel_width  = 240;//液晶の仕様に合わせて変更。ILI9341の場合240
      cfg.panel_height = 320;//液晶の仕様に合わせて変更。ILI9341の場合240<-多分320じゃね？
      cfg.offset_x     = 0;//液晶の仕様に合わせて変更。ここでは0
      cfg.offset_y     = 0;//液晶の仕様に合わせて変更。ここでは0
      cfg.invert       = false;//補色モードかどうか。trueだと、0xFFFFFFが黒になる
      cfg.rgb_order    = false;
      //cfg.offset_rotation = 0;
      _panel_instance.config(cfg);
    }

    {
      auto cfg = _light_instance.config();
      cfg.pin_bl      = 22;//ボードの設計に合わせて変更。バックライトLED制御をマイコンから行っていれば書く
      cfg.pwm_channel = 0;//1;//ボードの設計に合わせて変更。PWM channelは0か1になる。
      _light_instance.config(cfg);
      _panel_instance.setLight(&_light_instance);
    }

    setPanel(&_panel_instance);
  }
};
