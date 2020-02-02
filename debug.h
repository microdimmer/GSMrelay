#ifdef DEBUGGING
#undef PRINTLNF
#undef PRINTLN
#undef PRINTLNHEX
#undef PRINT
#undef PRINTINFO
#define PRINTLNF(s)       \
  {                       \
    Serial.println(F(s)); \
  }
#define PRINTINFO(s)       \
  {                       \
    Serial.print(F(s)); \
  }  
#define PRINTLN(s, v)   \
  {                     \
    Serial.print(F(s)); \
    Serial.println(v);  \
  }
#define PRINTLNHEX(s, v)    \
  {                         \
    Serial.print(F(s));     \
    Serial.println(v, HEX); \
  }
#define PRINT(s, v)     \
  {                     \
    Serial.print(F(s)); \
    Serial.print(v);    \
  }
#else
#undef PRINTLNF
#undef PRINTLN
#undef PRINTLNHEX
#undef PRINT
#undef PRINTINFO
#define PRINTLNF(s)
#define PRINTLN(s, v)
#define PRINTLNHEX(s, v)
#define PRINT(s, v)
#define PRINTINFO(s)
#endif