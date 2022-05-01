#include <WString.h>
#include "Utils.hpp"

void split(String str, char delimiter, String* strs, int length) {
  split(str, delimiter, strs, length, length);
}

void split(String str, char delimiter, String* strs, int length, int max) {
  int start = 0;
  int i = 0;
  while (i < max && i < length) {
    int pos = str.indexOf(delimiter, start);
    if (pos == -1) {
      break;
    }
    strs[i++] = str.substring(start, pos);
    start = pos + 1;
  }
  strs[i] = str.substring(start); // TODO: move in the loop
}
