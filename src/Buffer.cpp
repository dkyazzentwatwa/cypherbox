// Buffer.cpp - Double-buffered PCAP Writer Implementation for Cypherbox V2

#include "Buffer.h"
#include "SD.h"

Buffer::Buffer(){
  bufA = (uint8_t*)malloc(BUF_SIZE);
  bufB = (uint8_t*)malloc(BUF_SIZE);
}

void Buffer::open(fs::FS* fs){
  int i = 0;
  do {
    fileName = "/" + (String)i + ".pcap";
    i++;
  } while (fs->exists(fileName));

  Serial.println(fileName);
  file = fs->open(fileName, FILE_WRITE);
  file.close();

  bufSizeA = 0;
  bufSizeB = 0;
  writing = true;

  // Write PCAP header
  file = fs->open(fileName, FILE_APPEND);
  uint32_t magic = 0xa1b2c3d4;
  file.write((uint8_t*)&magic, 4);
  uint16_t verMaj = 2, verMin = 4;
  file.write((uint8_t*)&verMaj, 2);
  file.write((uint8_t*)&verMin, 2);
  int32_t thiszone = 0;
  uint32_t sigfigs = 0, snaplen = SNAP_LEN, linktype = 105;
  file.write((uint8_t*)&thiszone, 4);
  file.write((uint8_t*)&sigfigs, 4);
  file.write((uint8_t*)&snaplen, 4);
  file.write((uint8_t*)&linktype, 4);
  file.close();

  useSD = true;
}

void Buffer::close(fs::FS* fs){
  if (!writing) return;
  forceSave(fs);
  writing = false;
  Serial.println("PCAP file closed");
}

void Buffer::addPacket(uint8_t* buf, uint32_t len){
  if ((useA && bufSizeA + len >= BUF_SIZE && bufSizeB > 0) ||
      (!useA && bufSizeB + len >= BUF_SIZE && bufSizeA > 0)) {
    Serial.print(";"); // drop packet
    return;
  }

  if (useA && bufSizeA + len + 16 >= BUF_SIZE && bufSizeB == 0) {
    useA = false;
  } else if (!useA && bufSizeB + len + 16 >= BUF_SIZE && bufSizeA == 0) {
    useA = true;
  }

  uint32_t microSeconds = micros();
  uint32_t seconds = (microSeconds / 1000) / 1000;
  microSeconds -= seconds * 1000 * 1000;

  write(seconds);
  write(microSeconds);
  write(len);
  write(len);
  write(buf, len);
}

void Buffer::save(fs::FS* fs){
  if (!useA && bufSizeA > 0) {
    file = fs->open(fileName, FILE_APPEND);
    file.write(bufA, bufSizeA);
    file.close();
    bufSizeA = 0;
    saving = true;
  }
  saving = false;
}

void Buffer::write(int32_t n){
  uint8_t buf[4];
  buf[0] = n;
  buf[1] = n >> 8;
  buf[2] = n >> 16;
  buf[3] = n >> 24;
  write(buf, 4);
}

void Buffer::write(uint32_t n){
  uint8_t buf[4];
  buf[0] = n;
  buf[1] = n >> 8;
  buf[2] = n >> 16;
  buf[3] = n >> 24;
  write(buf, 4);
}

void Buffer::write(uint16_t n){
  uint8_t buf[2];
  buf[0] = n;
  buf[1] = n >> 8;
  write(buf, 2);
}

void Buffer::write(uint8_t* buf, uint32_t len){
  if (!writing) return;
  uint8_t* target = useA ? bufA : bufB;
  uint32_t& targetSize = useA ? bufSizeA : bufSizeB;
  if (targetSize + len < BUF_SIZE) {
    memcpy(target + targetSize, buf, len);
    targetSize += len;
  }
}

void Buffer::forceSave(fs::FS* fs){
  saving = true;
  if (bufSizeA > 0) {
    file = fs->open(fileName, FILE_APPEND);
    file.write(bufA, bufSizeA);
    file.close();
    bufSizeA = 0;
  }
  if (bufSizeB > 0) {
    file = fs->open(fileName, FILE_APPEND);
    file.write(bufB, bufSizeB);
    file.close();
    bufSizeB = 0;
  }
  saving = false;
}
