#pragma once

#ifdef MUS_DEBUG
  #define MDEBUG_PRINT(x)    Serial.print(x)
  #define MDEBUG_PRINTLN(x)  Serial.println(x)
  #define MDEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
  #define MDEBUG_PRINT(x)
  #define MDEBUG_PRINTLN(x)
  #define MDEBUG_PRINTF(...)
#endif
