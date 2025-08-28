#include "DFRobot_DS323X.h"
#include <mcp_can.h>
#include <SPI.h>

DFRobot_DS323X rtc;


MCP_CAN CAN0(10);                               // Set CS to pin 10

unsigned int firstPage=1;
unsigned int page=1;


String getTime(){
    // Extract hour and minute
  unsigned char hour = rtc.getHour();
  unsigned char minute = rtc.getMinute();
  // Format to hh:mm
  char buffer[6]; // "hh:mm" + null terminator
  sprintf(buffer, "%02d:%02d", hour, minute);

  return String(buffer);
}

void serialFlush() {
  while (Serial.available() > 0) {
    Serial.read();
  }
}

int getPageID() {
  Serial.print("get dp");
  Serial.write(0xFF);
  Serial.write(0xFF);
  Serial.write(0xFF);

  unsigned long start = millis();
  while (millis() - start < 500) { // timeout 500ms
    if (Serial.available() >= 4) {
      if (Serial.read() == 0x71) {   // respon untuk get
        int pageId = Serial.read();    // page ID
        Serial.read(); // buang byte dummy
        Serial.read(); // buang byte dummy
        Serial.read(); // buang byte dummy
        Serial.read(); // buang byte dummy
        Serial.read(); // buang byte dummy
        Serial.read(); // buang byte dummy
        return pageId;
      }
    }
  }
  return -1; // gagal baca
}
unsigned int hourInc(unsigned int currentTime ){
  currentTime++;
  if(currentTime==24){
    return 0;
  }

  return currentTime;
}
unsigned int hourDec(unsigned int currentTime ){
  currentTime--;
  if(currentTime==-1){
    return 23;
  }

  return currentTime;
}
unsigned int minuteInc(unsigned int currentTime ){
  currentTime++;
  if(currentTime==60){
    return 0;
  }
  return currentTime;
}
unsigned int minuteDec(unsigned int currentTime ){
  currentTime--;
  if(currentTime==-1){
    return 59;
  }
  return currentTime;
}
void welcomePage(int loading){
  delay(loading);
  gotoPage(firstPage);
  page=firstPage;
}
void sendText(char caption[],String text){
  Serial.print(caption);
  Serial.print(".txt=\"");
  Serial.print(text);
  Serial.print("\"");
  Serial.write(0xFF); 
  Serial.write(0xFF); 
  Serial.write(0xFF);
}

void gotoPage(int newPage){
  Serial.print("page "+ String(newPage));
  Serial.write(0xFF); 
  Serial.write(0xFF); 
  Serial.write(0xFF);
  int pageNow = -1;
  while (pageNow != newPage) {
    pageNow = getPageID();
  }
  page=getPageID();
}
void btnEvent(int currPage){
  if (Serial.available() > 0) {
    if (Serial.read() == 0x65) {  // Touch event
      byte page = Serial.read();      // Page ID
      byte component = Serial.read(); // Component ID
      byte event = Serial.read();     // Press(1)/Release(0)
      for(int i=0;i<3;i++) Serial.read();
      if(page==currPage && event==1){
        switch (page){
          case 0:
          break;
          case 1:
            if(component==0x0c){
              gotoPage(3);
            }
          break;
          case 2:
          break;
          case 3:
          if(component==0x0a){
              gotoPage(1);
            }
          if(component==0x08){
              gotoPage(4);
            }
          break;
          case 4:
          if(component==0x05){
              gotoPage(1);
            }
          if(component==0x04){
              rtc.setTime(rtc.getYear(), rtc.getMonth(), rtc.getDate(), hourInc(rtc.getHour()), rtc.getMinute(), rtc.getSecond());
            }
          if(component==0x06){
              rtc.setTime(rtc.getYear(), rtc.getMonth(), rtc.getDate(), hourDec(rtc.getHour()), rtc.getMinute(), rtc.getSecond());
            }
          if(component==0x0a){
              rtc.setTime(rtc.getYear(), rtc.getMonth(), rtc.getDate(), rtc.getHour(), minuteInc(rtc.getMinute()), rtc.getSecond());
            }
          if(component==0x0b){
              rtc.setTime(rtc.getYear(), rtc.getMonth(), rtc.getDate(), rtc.getHour(), minuteDec(rtc.getMinute()), rtc.getSecond());
            }
          break;
          case 5:
          if(component==0x03){
              gotoPage(1);
            }
          break;
        }
      }
     
    }
  }
}
void viewPage(){
  switch (page){
    case 1:
    pageDaylight();
    btnEvent(page);
    break;
    case 2:
    break;
    case 3: //page setting
    pageSetting();
    btnEvent(page);
    break;
    case 4: //page time setting
    pageTimeSet();
    btnEvent(page);
    break;
    case 5: //page brightness setting
    break;
  }
}

void pageTimeSet(){
  sendText("t5",String(rtc.getHour()));
  sendText("t2",String(rtc.getMinute()));
}
void pageDaylight(){
  long unsigned int rxId;
  unsigned char len = 0;
  unsigned char rxBuf[8];
  if (CAN0.checkReceive() == CAN_MSGAVAIL) {
    CAN0.readMsgBuf(&rxId, &len, rxBuf);
    if(rxId==0x100){
      sendText("t8",String(rxBuf[0])+"%");
      sendText("t9",String(rxBuf[1])+"%");
      sendText("t10",String(rxBuf[2])+"C");
    }
  }
  sendText("t1",getTime());
}

void pageSetting(){
  sendText("t5",getTime());
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  rtc.begin();

  welcomePage(4000);

      // Initialize MCP2515 running at 16MHz with a baudrate of 500kb/s and the masks and filters disabled.
  if(CAN0.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) != CAN_OK){
    while(1);
  }
  CAN0.setMode(MCP_NORMAL);                     // Set operation mode to normal so the MCP2515 sends acks to received data.
  serialFlush();
}
void loop() {
  viewPage();
}
