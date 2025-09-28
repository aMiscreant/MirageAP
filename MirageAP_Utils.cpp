// MirageAP_Utils.cpp
#include "MirageAP_Utils.h"

String urlDecode(String input) {
  String decoded = "";
  char temp[] = "0x00";
  unsigned int len = input.length();
  unsigned int i = 0;
  while (i < len) {
    char c = input.charAt(i);
    if (c == '+') {
      decoded += ' ';
    } else if (c == '%') {
      if (i + 2 < len) {
        temp[2] = input.charAt(i + 1);
        temp[3] = input.charAt(i + 2);
        decoded += (char) strtol(temp, NULL, 16);
        i += 2;
      }
    } else {
      decoded += c;
    }
    i++;
  }
  return decoded;
}

String bytesToStr(const uint8_t* b, uint32_t size) {
  String str;
  const char ZERO = '0';
  const char DOUBLEPOINT = ':';
  for (uint32_t i = 0; i < size; i++) {
    if (b[i] < 0x10) str += ZERO;
    str += String(b[i], HEX);
    if (i < size - 1) str += DOUBLEPOINT;
  }
  return str;
}
