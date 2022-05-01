#ifndef Utils_h
#define Utils_h

#include <WString.h>

/**
 * Dato un array di uint16 e la sua dimensione 
 * restituisce una String nel formato:
 * "<length>|<array[0]>,<array[1]>,..."
 * 
 * @param array Insieme valori letti dal sensore IR
 * @param length Lunghezza di array
 * @return Stringa con formato "<length>|<array[0]>,<array[1]>,..."
*/
String uint16ArrayToString(uint16_t array[], uint16_t length);

#endif // Utils_h
