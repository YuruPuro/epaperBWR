/**
 * 3 Colors ePaper GDEW0213Z16
 * Display SAMPLE Program.
 */
#include <SPI.h>
#include"bitmapdata.h"
#include"image1.h"
#include"image2.h"

int BUSY_Pin = 6; 
int RES_Pin = 7; 
int DC_Pin = 9; 
int CS_Pin = 10; 

// ▲▼▲▼▲▼▲▼▲▼▲▼▲▼▲▼▲▼▲▼▲▼
const uint8_t INIT_COMMANDS[] PROGMEM={
  0x06 , 3 , 0x17 , 0x17 , 0x17 , // boost soft start
  0x04 , 0 ,                      // Power on
  0xFE , 0 ,                      // WaitBusy
  0x00 , 1 , 0x0F , // 0x0B - 左右反転 : 0x07 - 上下反転 : 0x03 - 上下左右反転 : 0x0F - Default :Panel setting
  0x61 , 3 , 0x68 , 0x00 , 0xD4 , // resolution setting : 104 x 212
  0x50 , 1 , 0x77 ,               // Vcom and data interval setting
  0xFF , 0 ,
};

// ePaper 初期化：スペックシートの初期化フローの通り
// 0xF0　以降のコマンドはスケッチ制御用の拡張
void EPD_init(void) {
  // -- Hardware Reset --
  digitalWrite(RES_Pin,LOW) ; // Module reset
  delay(100); //At least 10ms
  digitalWrite(RES_Pin,HIGH);
  delay(100);  

  // -- ePaper Init --
  digitalWrite(CS_Pin, LOW);
  int p = 0 ;
  while(1) {
    uint8_t cmd = pgm_read_byte(&INIT_COMMANDS[p]) ; p++ ;
    uint8_t cnum= pgm_read_byte(&INIT_COMMANDS[p]) ; p++ ;
    if (cmd == 0xFF) {
      break ;
    } else
    if (cmd == 0xFE) {
      EPD_chkstatus();
    } else {
      digitalWrite(DC_Pin, LOW);
      SPI.transfer(cmd); // Send Comand
      if ( cnum > 0) {
        digitalWrite(DC_Pin, HIGH);
        for (int i=0;i<cnum;i++) {
          uint8_t data = pgm_read_byte(&INIT_COMMANDS[p]) ; p++ ;
          SPI.transfer(data); // Send Data
        }
      }
    }
  }
  digitalWrite(CS_Pin, HIGH);
}

// Display Refresh コマンド送信
void EPD_refresh(void) {
  digitalWrite(CS_Pin, LOW);
  digitalWrite(DC_Pin, LOW);
  SPI.transfer(0x12);          //DISPLAY REFRESH  
  digitalWrite(CS_Pin, HIGH);

  delay(100);      //!!!The delay here is necessary, 200uS at least!!!     
  EPD_chkstatus();
} 

// Deep Sleep　モードに移行
void EPD_sleep(void) {
  digitalWrite(CS_Pin, LOW);
  digitalWrite(DC_Pin, LOW);
  SPI.transfer(0X50);
  digitalWrite(DC_Pin, HIGH);
  SPI.transfer(0xf7);
       
  digitalWrite(DC_Pin, LOW);
  SPI.transfer(0X02);   //power off

  EPD_chkstatus();

  digitalWrite(DC_Pin, LOW);
  SPI.transfer(0X07);   //deep sleep
  digitalWrite(DC_Pin, HIGH);
  SPI.transfer(0xA5);

  digitalWrite(CS_Pin, HIGH);
}

// Busy信号をチェック、Readyになるまで待つ
void EPD_chkstatus(void) {
  digitalWrite(DC_Pin, LOW);
  do {
    SPI.transfer(0x71);
  } while(digitalRead(BUSY_Pin)==0);   
  delay(200) ;
}

// ▲▼▲▼▲▼▲▼▲▼▲▼▲▼▲▼▲▼▲▼▲▼
// 画面初期化：真っ白にする
void PIC_display_Clean(void) {
  digitalWrite(CS_Pin, LOW);

  digitalWrite(DC_Pin, LOW);
  SPI.transfer(0x10);         // Transfer B/W
  digitalWrite(DC_Pin, HIGH);
  for(int i=0;i<2756;i++) {
    SPI.transfer(0xFF) ;
  }
  
  digitalWrite(DC_Pin, LOW);
  SPI.transfer(0x13);         // Transfer RED
  digitalWrite(DC_Pin, HIGH);
  for(int i=0;i<2756;i++) {
    SPI.transfer(0xFF) ;
  }

  digitalWrite(CS_Pin, HIGH);
}

// 全画面モードで描画
void PIC_display(const uint8_t* picData_BW,const uint8_t* picData_RED) {
  digitalWrite(CS_Pin, LOW);

  if (picData_BW != NULL) {
    digitalWrite(DC_Pin, LOW);
    SPI.transfer(0x10);           // Transfer B/W
    digitalWrite(DC_Pin, HIGH);
    for(int i=0;i<2756;i++) {     // 212x104 (212x13)
      uint8_t data = pgm_read_byte(&picData_BW[i]);
      SPI.transfer(data) ;
    }
  }

  if (picData_RED != NULL) {
    digitalWrite(DC_Pin, LOW);
    SPI.transfer(0x13);           // Transfer RED
    digitalWrite(DC_Pin, HIGH);
    for(int i=0;i<2756;i++) {     // 212x104 (212x13)
      uint8_t data = pgm_read_byte(&picData_RED[i]);
      SPI.transfer(data) ;
    }
  }

  digitalWrite(CS_Pin, HIGH);
}

// 部分表示モードで描画
// flagRED - false:黒で描画  true:赤で描画
void PIC_display_Partial(uint8_t x , uint16_t y , uint8_t w , uint8_t h , uint8_t *ImageAddr,bool flagRED) {
  uint8_t wData , data ;
  digitalWrite(CS_Pin, LOW);

  // --- 描画範囲設定
  int dataSize = w * h ;

  digitalWrite(DC_Pin, LOW);
  SPI.transfer(0x90);           // Partial In
  digitalWrite(DC_Pin, HIGH);
  data = ( x << 3 ) ;
  SPI.transfer(data);           // XS
  data = ( (x + w -1) << 3 ) | 0x03 ;
  SPI.transfer(data);           // XE
  SPI.transfer(0);              // YS
  SPI.transfer(y);              // YS
  SPI.transfer(0);              // YE
  SPI.transfer((y+h-1));        // YE

  digitalWrite(DC_Pin, LOW);
  SPI.transfer(0x91);           // Partial In

  digitalWrite(DC_Pin, LOW);
  data = (flagRED) ? 0x13 : 0x10 ;
  SPI.transfer(data);           // Transfer B/W
  digitalWrite(DC_Pin, HIGH);
  for(int i=0;i<dataSize;i++){
    wData = pgm_read_byte(&ImageAddr[i]) ;
    SPI.transfer(wData);
  }

  digitalWrite(DC_Pin, LOW);
  SPI.transfer(0x92);           // Partial Out

  digitalWrite(CS_Pin, HIGH);
}

// ▲▼▲▼▲▼▲▼▲▼▲▼▲▼▲▼▲▼▲▼▲▼
void setup() {
  SPI.begin();  //SPIを初期化、SCK、MOSI、SSの各ピンの動作は出力、SCK、MOSIはLOW、SSはHIGH
  SPI.setClockDivider(SPI_CLOCK_DIV2);  // 8MHz
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);

  pinMode(BUSY_Pin, INPUT); 
  pinMode(RES_Pin, OUTPUT);  
  pinMode(DC_Pin, OUTPUT);    
  pinMode(CS_Pin, OUTPUT);    
}

void loop() {
  while(1) {
    //PICTURE1：全画面モード：トランプ
    EPD_init(); //EPD init
    PIC_display_Clean();
    PIC_display(epd_bitmap_image3B , epd_bitmap_image3R );
    EPD_refresh();//EPD_refresh   
    EPD_sleep();//EPD_sleep,Sleep instruction is necessary, please do not delete!!!
    delay(10000);

    //PICTURE2：部分描画モード：ささら＆ゆるぷろ
    EPD_init(); //EPD init
    PIC_display_Clean();
    PIC_display_Partial(0,0,13,129,epd_bitmap_image2,false) ;
    PIC_display_Partial(1,130,10,80,epd_bitmap_image,true) ;
    EPD_refresh();//EPD_refresh   
    EPD_sleep();//EPD_sleep,Sleep instruction is necessary, please do not delete!!!
    delay(10000);

    //PICTURE Clean
    EPD_init(); //EPD init
    PIC_display_Clean();
    EPD_refresh();//EPD_refresh   
    EPD_sleep();//EPD_sleep,Sleep instruction is necessary, please do not delete!!!
    while(1);
  }
}
