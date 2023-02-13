
#include "N2Kstuff.h"


// void _AddByteEscapedToBuf(unsigned char byteToAdd, uint8_t &idx, unsigned char *buf, int &byteSum)
// {
//   buf[idx++]=byteToAdd;
//   byteSum+=byteToAdd;

//   if (byteToAdd == Escape) {
//     buf[idx++]=Escape;
//   }
// }

// //*****************************************************************************
// // Actisense Format:
// // <10><02><93><length (1)><priority (1)><PGN (3)><destination (1)><source (1)><time (4)><len (1)><data (len)><CRC (1)><10><03>
// void tN2kMsg::SendInActisenseFormat(N2kStream *port) const {
//   unsigned long _PGN=PGN;
//   unsigned long _MsgTime=MsgTime;
//   uint8_t msgIdx=0;
//   int byteSum = 0;
//   uint8_t CheckSum;
//   unsigned char ActisenseMsgBuf[MaxActisenseMsgBuf];

//   if (port==0 || !IsValid()) return;
//   // Serial.print("freeMemory()="); Serial.println(freeMemory());

//   ActisenseMsgBuf[msgIdx++]=Escape;
//   ActisenseMsgBuf[msgIdx++]=StartOfText;
//   AddByteEscapedToBuf(MsgTypeN2k,msgIdx,ActisenseMsgBuf,byteSum);
//   AddByteEscapedToBuf(DataLen+11,msgIdx,ActisenseMsgBuf,byteSum); //length does not include escaped chars
//   AddByteEscapedToBuf(Priority,msgIdx,ActisenseMsgBuf,byteSum);
//   AddByteEscapedToBuf(_PGN & 0xff,msgIdx,ActisenseMsgBuf,byteSum); _PGN>>=8;
//   AddByteEscapedToBuf(_PGN & 0xff,msgIdx,ActisenseMsgBuf,byteSum); _PGN>>=8;
//   AddByteEscapedToBuf(_PGN & 0xff,msgIdx,ActisenseMsgBuf,byteSum);
//   AddByteEscapedToBuf(Destination,msgIdx,ActisenseMsgBuf,byteSum);
//   AddByteEscapedToBuf(Source,msgIdx,ActisenseMsgBuf,byteSum);
//   // Time?
//   AddByteEscapedToBuf(_MsgTime & 0xff,msgIdx,ActisenseMsgBuf,byteSum); _MsgTime>>=8;
//   AddByteEscapedToBuf(_MsgTime & 0xff,msgIdx,ActisenseMsgBuf,byteSum); _MsgTime>>=8;
//   AddByteEscapedToBuf(_MsgTime & 0xff,msgIdx,ActisenseMsgBuf,byteSum); _MsgTime>>=8;
//   AddByteEscapedToBuf(_MsgTime & 0xff,msgIdx,ActisenseMsgBuf,byteSum);
//   AddByteEscapedToBuf(DataLen,msgIdx,ActisenseMsgBuf,byteSum);
/*
unsigned char _ActisenseMsgBuf[400];
void EndianSwap(size_t size, bool _byte, bool _order) {
  unsigned char temp[400];
  unsigned char _tempbyte;
  for (int i = 0; i <= size; i++) {
    if (_order) {
      temp[i] = _ActisenseMsgBuf[size - i];
    } else {
      temp[i] = _ActisenseMsgBuf[i];
    }
    if (_byte) {
      _tempbyte = 0;
      if ((temp[i] & 128) == 128) { _tempbyte = _tempbyte | 1; }
      if ((temp[i] & 64) == 64) { _tempbyte = _tempbyte | 2; }
      if ((temp[i] & 32) == 32) { _tempbyte = _tempbyte | 4; }
      if ((temp[i] & 16) == 16) { _tempbyte = _tempbyte | 8; }
      if ((temp[i] & 8) == 8) { _tempbyte = _tempbyte | 16; }
      if ((temp[i] & 4) == 4) { _tempbyte = _tempbyte | 32; }
      if ((temp[i] & 2) == 2) { _tempbyte = _tempbyte | 64; }
      if ((temp[i] & 1) == 1) { _tempbyte = _tempbyte | 128; }
      temp[i] = _tempbyte;
    }
  }
  for (int i = 0; i <= size; i++) {
    _ActisenseMsgBuf[i] = temp[i];
  }
}



*/




//   for (int i = 0; i < DataLen; i++) AddByteEscapedToBuf(Data[i],msgIdx,ActisenseMsgBuf,byteSum);
//   byteSum %= 256;

//   CheckSum = (uint8_t)((byteSum == 0) ? 0 : (256 - byteSum));
//   ActisenseMsgBuf[msgIdx++]=CheckSum;
//   if (CheckSum==Escape) ActisenseMsgBuf[msgIdx++]=CheckSum;

//   ActisenseMsgBuf[msgIdx++] = Escape;
//   ActisenseMsgBuf[msgIdx++] = EndOfText;

//  // if ( port->availableForWrite()>msgIdx ) {  // 16.7.2017 did not work yet
//     port->write(ActisenseMsgBuf,msgIdx);
//  // }
//  //   Serial.print("Actisense data:");
//     //PrintBuf(msgIdx,ActisenseMsgBuf);
//  //   Serial.print("\r\n");
// }