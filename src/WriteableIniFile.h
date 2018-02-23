/*
 * Writeable INI file 
 * by Mikhail Klimushin aka Night_Gryphon
 * ngryph@gmail.com
 * v 1.0
 */
#ifndef W_INIFILE_H
#define W_INIFILE_H

#include <FS.h>

//#define WIF_DEBUG
 
#ifdef WIF_DEBUG
#include <HardwareSerial.h>
#define WIF_DEBUG_OUT(...) { Serial.printf(__VA_ARGS__); Serial.flush(); }
#endif

#ifndef WIF_DEBUG_OUT
#define WIF_DEBUG_OUT(...)
#endif

class WriteableIniFile {
public:
  // ToDo: check setting errorNoError
  enum error_t {
    errorNoError = 0,
//    errorNotFound, // is not an error. function return false when all available data successfully read but no suitable result found
    errorOutOfBuffer,
    errorFileSeek,
    errorFileRead,
    errorFileWrite,
    errorMalloc,
    errorInvalidParams,
    errorUnknownError,
  };

  WriteableIniFile(File * file);

  bool inLineComments;

  // set processing buffer
  bool setBuffer(char * buf, size_t asize, bool check_buf = true);

  // get last found name pointer and length
  void getLastName(char * &aname, size_t &name_len);
  // get last found name as null termminated string. modify buffer
  char * getLastName();

  // move _section_start to specified section 
  // or EOF if not found
  bool openSection(char * section);
  bool nextSection();

  // copy value to external buffer. keep work buffer intact
  bool getValueCopy(char * aname, char * buf, size_t asize);

  // if found 
  //    modify work buffer to null terminated string and return value pointer
  // else
  //    return default value pointer
  char * getValue(char * aname, char * defval);
  // restart section values scan
  void resetSection();

  bool setValue(char * aname, char * new_val, size_t pl_bytes = 0);

  void printIni(Print & p);
  void printJson(Print & p);

  error_t getLastError() {
    return _lastError;
  }

  void resetError() {
    _lastError = errorNoError;
  }

  bool error() {
    return errorNoError != _lastError;
  }
  
protected:
  File * _file;

  char * _buf; // processing buffer
  size_t _buf_size;

  char * _name; // last found name pointer and length
  size_t _name_len;

  uint32_t _section_start; // current section start file pos
  uint32_t _value_pos; // file pos to get next -ANY- value

  error_t _lastError;

  // check if current file lines compatible with current buffer
  bool validateBuffer();
  // read line from file. move position to next line. return valuable data boundaries pointers and EOL pointer
  bool readLine(uint32_t &file_pos, char * &data_begin, char * &data_end, char * &line_end);
  

  // tools
  bool seekChar(const char * chars, char * &pos, char * boundary);
  bool seekChar(const char * chars, char * &pos);
  bool seekCharNot(const char * chars, char * &pos, char * boundary);
  bool seekCharNot(const char * chars, char * &pos);

  // search for section.
  // found => set section_pos, return true
  // NULL = any next section (db - de contail section name)
  bool seekSection(
           char * section,  // section to find
           uint32_t &file_pos ); // file_pos - section start or EOF

  // search current section for value
  // NULL = any value
  bool seekValue(
           char * aname,  // value to find
           char * &value, size_t &value_len,
           uint32_t &placeholder, size_t &placeholder_len); // value placeholder file position and length
};

#endif
