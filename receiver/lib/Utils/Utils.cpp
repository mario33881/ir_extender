#include <WString.h>
#include "Utils.hpp"


String uint16ArrayToString(uint16_t array[], uint16_t length) {

    String res = String(length) + "|";
    
    for (uint16_t i = 0; i < length; i++){
        
        res += String(array[i]);
        
        if (i != length-1) {
            res += ",";
        }
    }

    return res;
}
