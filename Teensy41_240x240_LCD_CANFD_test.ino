/*
 * Demo sketch for use with:
 * http://skpang.co.uk/catalog/teensy-41-triple-can-board-with-240x240-ips-lcd-p-1592.html
 *  
 * can1 baudrate 500kbps
 * 
 * can2 baudrate 500kbps
 * 
 * can3 baudrate  
 * Nominal baudrate 500kbps
 * CAN FD data baudrate 2000kbps
 * 
 * www.skpang.co.uk May 2020
 * 
 * 
 * */
 
#include <FlexCAN_T4.h>
FlexCAN_T4FD<CAN3, RX_SIZE_256, TX_SIZE_16> FD;
FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> can2;
FlexCAN_T4<CAN1, RX_SIZE_256, TX_SIZE_16> can1;

#include <SPI.h>
#include <ST7735_t3.h> // Hardware-specific library
#include <ST7789_t3.h> // Hardware-specific library
#include <ST7735_t3_font_Arial.h>
#include <ST7735_t3_font_ArialBold.h>

#define TFT_RST    32   // chip reset
#define TFT_DC     9   // tells the display if you're sending data (D) or commands (C)   --> WR pin on TFT
#define TFT_MOSI   11  // Data out    (SPI standard)
#define TFT_SCLK   13  // Clock out   (SPI standard)
#define TFT_CS     10  // chip select (SPI standard)

int LCD_BL = 33;       // LCD back light control

ST7789_t3 tft = ST7789_t3(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
IntervalTimer timer;
uint8_t d=0;
bool stopfd = 0;

void canSniff20(const CAN_message_t &msg) { // global callback
  Serial.print("T4: ");
  Serial.print("MB "); Serial.print(msg.mb);
  Serial.print(" OVERRUN: "); Serial.print(msg.flags.overrun);
  Serial.print(" BUS "); Serial.print(msg.bus);
  Serial.print(" LEN: "); Serial.print(msg.len);
  Serial.print(" EXT: "); Serial.print(msg.flags.extended);
  Serial.print(" REMOTE: "); Serial.print(msg.flags.remote);
  Serial.print(" TS: "); Serial.print(msg.timestamp);
  Serial.print(" ID: "); Serial.print(msg.id, HEX);
  Serial.print(" IDHIT: "); Serial.print(msg.idhit);
  Serial.print(" Buffer: ");
  for ( uint8_t i = 0; i < msg.len; i++ ) {
    Serial.print(msg.buf[i], HEX); Serial.print(" ");
  } Serial.println();
}



void setup(void) {
  
  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, HIGH);  // Turn LCD backlight on
 
  Serial.begin(115200); delay(1000);
  Serial.println("Teensy 4.0 CAN FD test. www.skpang.co.uk Jan 2020");
 
  FD.begin();

  CANFD_timings_t config;
  config.clock = CLK_24MHz;
  config.baudrate =   500000;
  config.baudrateFD = 2000000;
  config.propdelay = 190;
  config.bus_length = 1;
  config.sample = 75;
  FD.setRegions(64);
  FD.setBaudRate(config);
  FD.onReceive(canSniff);

  FD.setMBFilter(ACCEPT_ALL);
  FD.setMBFilter(MB13, 0x1);
  FD.setMBFilter(MB12, 0x1, 0x3);
  FD.setMBFilterRange(MB8, 0x1, 0x04);
  FD.enableMBInterrupt(MB8);
  FD.enableMBInterrupt(MB12);
  FD.enableMBInterrupt(MB13);
  FD.enhanceFilter(MB8);
  FD.enhanceFilter(MB10);
  FD.distribute();
  FD.mailboxStatus();

  can2.begin();
  can2.setBaudRate(500000);
  can2.setMBFilter(ACCEPT_ALL);
  can2.distribute();
  can2.mailboxStatus();
  
  can1.begin();
  can1.setBaudRate(500000);
  can1.setMBFilter(ACCEPT_ALL);
  can1.distribute();
  can1.mailboxStatus();

  
  tft.init(240, 240);
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLUE);

  tft.setCursor(5,5);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setFont(Arial_32);
  tft.println("Teensy  4.1");

  tft.setCursor(10,80);
  tft.setFont(Arial_40_Bold); 
  tft.println("240x240");
  tft.setCursor(10 ,140);
  tft.println("IPS LCD");
  
  tft.setFont(Arial_16_Bold);
  tft.setCursor(10,212);
  tft.println("www.skpang.co.uk");   

  timer.begin(sendframe, 500000); // Send frame every 500ms
}

void sendframe()
{
  CAN_message_t msg2;
  msg2.id = 0x40;
  
  for ( uint8_t i = 0; i < 8; i++ ) {
    msg2.buf[i] = i + 1;
  }
  
  msg2.buf[0] = d++;
  msg2.seq = 1;
  can2.write(MB15, msg2);
 
  CAN_message_t msg1;
  msg1.id = 0x7df;
  msg1.buf[0] = d;
  can1.write(msg1);

  
  CANFD_message_t msg;
  msg.len = 64;
  msg.id = 0x321;
  msg.seq = 1;
  msg.buf[0] = d; msg.buf[1] = 2; msg.buf[2] = 3; msg.buf[3] = 4;
  msg.buf[4] = 5; msg.buf[5] = 6; msg.buf[6] = 9; msg.buf[7] = 9;
  FD.write( msg);
}

void loop() {
  
  FD.events(); /* needed for sequential frame transmit and callback queues */
  CANFD_message_t msg;
  CAN_message_t msgcan1;
  CAN_message_t msgcan2;
  
  if(FD.readMB(msg)){   /* check if we received a CAN frame */
      Serial.print("MB: "); Serial.print(msg.mb);
      Serial.print("  OVERRUN: "); Serial.print(msg.flags.overrun);
      Serial.print("  ID: 0x"); Serial.print(msg.id, HEX );
      Serial.print("  EXT: "); Serial.print(msg.flags.extended );
      Serial.print("  LEN: "); Serial.print(msg.len);
      Serial.print("  BRS: "); Serial.print(msg.brs);
      Serial.print(" DATA: ");
      for ( uint8_t i = 0; i <msg.len ; i++ ) {
        Serial.print(msg.buf[i]); Serial.print(" ");
      }
      Serial.print("  TS: "); Serial.println(msg.timestamp);
    
  }

  if(can1.read(msgcan1))
  {   /* check if we received a CAN frame */
      Serial.println("Got can1");
      Serial.print("MB: "); Serial.print(msgcan1.mb);
      Serial.print("  OVERRUN: "); Serial.print(msgcan1.flags.overrun);
      Serial.print("  ID: 0x"); Serial.print(msgcan1.id, HEX );
      Serial.print("  EXT: "); Serial.print(msgcan1.flags.extended );
      Serial.print("  LEN: "); Serial.print(msgcan1.len);
      
      Serial.print(" DATA: ");
      for ( uint8_t i = 0; i <msgcan1.len ; i++ ) {
        Serial.print(msgcan1.buf[i]); Serial.print(" ");
      }
      Serial.print("  TS: "); Serial.println(msgcan1.timestamp);
    
  }

  if(can2.read(msgcan2))
  {   /* check if we received a CAN frame */
      Serial.println("Got can2");
      Serial.print("MB: "); Serial.print(msgcan2.mb);
      Serial.print("  OVERRUN: "); Serial.print(msgcan2.flags.overrun);
      Serial.print("  ID: 0x"); Serial.print(msgcan2.id, HEX );
      Serial.print("  EXT: "); Serial.print(msgcan2.flags.extended );
      Serial.print("  LEN: "); Serial.print(msgcan2.len);
      
      Serial.print(" DATA: ");
      for ( uint8_t i = 0; i <msgcan2.len ; i++ ) {
        Serial.print(msgcan2.buf[i]); Serial.print(" ");
      }
      Serial.print("  TS: "); Serial.println(msgcan2.timestamp);
    
  }
 
}

void canSniff(const CANFD_message_t &msg) {
  if ( stopfd ) return;
  Serial.print("ISR - MB "); Serial.print(msg.mb);
  Serial.print("  OVERRUN: "); Serial.print(msg.flags.overrun);
  Serial.print("  LEN: "); Serial.print(msg.len);
  Serial.print(" EXT: "); Serial.print(msg.flags.extended);
  Serial.print(" TS: "); Serial.print(msg.timestamp);
  Serial.print(" ID: "); Serial.print(msg.id, HEX);
  Serial.print(" Buffer: ");
  for ( uint8_t i = 0; i < msg.len; i++ ) {
    Serial.print(msg.buf[i], HEX); Serial.print(" ");
  } Serial.println();
}
