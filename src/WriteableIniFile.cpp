#include "WriteableIniFile.h"

const char * space_chars = " \t";
const char * eol_chars = "\r\n";
const char * line_stop_chars = ";#\r\n"; // stop read line at comment or eol
const char * value_separator_chars = ":=";

WriteableIniFile::WriteableIniFile(File * file)
: _file(file)
, _buf(NULL)
, _buf_size(0)
, _section_start(0)
, _value_pos(0)
, _name(NULL)
,_name_len(0)
,fullLineComments(false)
,_lastError(errorNoError)
{
}


// set processing buffer
bool WriteableIniFile::setBuffer(char * buf, size_t asize, bool check_buf) {
  _buf = buf;
  _name = buf;
  _buf_size = asize;
  if ( (! check_buf) || validateBuffer()) 
    return true;

  WIF_DEBUG_OUT("Validate buffer FAIL");
  _buf = NULL;
  _buf_size = 0;
  return false;
}

// check if current file lines compatible with current buffer
bool WriteableIniFile::validateBuffer() {
  uint32_t pos = 0;
  char * db;
  char * de;
  char * le;
//  _lastError = errorNoError;
  while (readLine(pos, db, de, le));
  return (pos >= _file->size()); // EOF reached?
}

// get last found name pointer and length
void WriteableIniFile::getLastName(char * &aname, size_t &name_len) { 
  aname = _name;
  name_len = _name_len;
}

// get last found name as null termminated string. modify buffer
char * WriteableIniFile::getLastName() {
    if (NULL == _name)
      _name = _buf; 
    _name[_name_len] = '\0';
    return _name;
}


// ------------------ Read Line ---------------
// read line from file. move position to next line. return valuable data boundaries pointers and EOL pointer
bool WriteableIniFile::readLine(uint32_t &file_pos, char * &data_begin, char * &data_end, char * &line_end) {
//WIF_DEBUG_OUT("readLine@%d => %d[%d]\r\n",file_pos, _buf, _buf_size);

  if ( (_buf_size < 4) || (NULL == _buf) ) {// the shortest usable is "a=x\n"
    WIF_DEBUG_OUT("Line FAIL: buf too small\r\n");
    _lastError = errorOutOfBuffer;
    return false;
  }

//WIF_DEBUG_OUT("Seek %d => %d\r\n", _file->position(), file_pos);
  if (!_file->seek(file_pos, SeekSet)) {
    WIF_DEBUG_OUT("Line FAIL: seek %d fail\r\n", file_pos);
    _lastError = errorFileSeek;
    return false;
  }

//WIF_DEBUG_OUT("Read to %d[%d]\r\n", _buf, _buf_size);
  size_t bytesRead = _file->read((uint8_t *)_buf, _buf_size);
  if (!bytesRead) {
    WIF_DEBUG_OUT("Line FAIL: readed 0 bytes. EOF?\r\n");
//    _lastError = errorNotFound;
    return false;
  }

  // add EOL char at EOF
  if (bytesRead < _buf_size)
    _buf[bytesRead] = '\n';

//WIF_DEBUG_OUT("Line@%d: %.*s\r\n",file_pos, bytesRead, _buf);
  char * buf_end = _buf + bytesRead;
  data_begin = _buf-1;

  seekCharNot(space_chars, data_begin, buf_end); // skip leading spaces

  // Seek EOL or comment
  data_end = data_begin;
  if ( strchr(line_stop_chars, *data_end) || 
       ( fullLineComments ? seekChar(eol_chars, data_end, buf_end) : seekChar(line_stop_chars, data_end, buf_end) ) ) {
    line_end = data_end-1;
    if (seekChar(eol_chars, line_end, buf_end)) {
      // line complete
      seekCharNot(eol_chars, line_end, buf_end); // skip all EOL chars
      //while ( (++line_end < buf_end) && (NULL != strchr(eol_chars, *line_end )) ); // skip all EOL chars
      file_pos += line_end - _buf;
      WIF_DEBUG_OUT("Line TRUE: pos %d line data '%.*s'\r\n", file_pos, data_end-data_begin, data_begin);
      return true; 
    }
  }
  WIF_DEBUG_OUT("Line FAIL: no EOL found\r\n");
  _lastError = errorOutOfBuffer;
  return false;
}


// ---------------- utils ----------------

bool WriteableIniFile::seekChar(const char * chars, char * &pos, char * boundary) {
  while (++pos < boundary)
    if (NULL != strchr(chars, *pos)) {
      return true;
    }
  return false;
}

bool WriteableIniFile::seekChar(const char * chars, char * &pos) {
  return seekChar(chars, pos, _buf + _buf_size);
}


bool WriteableIniFile::seekCharNot(const char * chars, char * &pos, char * boundary) {
  while (++pos < boundary)
    if (NULL == strchr(chars, *pos))
      return true;
  return false;
}

bool WriteableIniFile::seekCharNot(const char * chars, char * &pos) {
  return seekCharNot(chars, pos, _buf + _buf_size );
}

// ------------------ sections -------------

bool WriteableIniFile::seekSection(
           char * section,  // section to find
           uint32_t &file_pos ) { // file_pos - section start or EOF
  // search for section.
  // found => set section_pos, return true
  // NULL = seek any next section

  _name = _buf;
  _name_len = 0;
  char * de; // data end ptr
  char * le; // line end ptr
  char * p;

  WIF_DEBUG_OUT("seekSection@%d: ", file_pos);
  if (section) {
    WIF_DEBUG_OUT("%s\r\n", section);
  } else {
    WIF_DEBUG_OUT("-ANY-\r\n");
  }

  while ( readLine(file_pos, _name, de, le) ) { // read file lines
      if ('[' == *_name) { // section header?
      seekCharNot(space_chars, _name, de); // skip spaces before section name
      p = _name-1; // name points to first non-spaace after '['
      if (seekChar("]", p, de)) { // find closing bracket
        // section header detected
        // skip trailing spaces. p points to ']'
	      while (strchr(space_chars, *(--p)) && (p >= _name) );
        _name_len = p - _name + 1;
        WIF_DEBUG_OUT("Section: %.*s\r\n", _name_len, _name);
        if ( (NULL == section) || 
             ( (strlen(section) == _name_len) && (0 == strncmp(section, _name, _name_len)) )
           ) {
          // section found
          WIF_DEBUG_OUT("Success!\r\n");
          return true;
	      }
      }
    }
  }
  // all read but no section
  WIF_DEBUG_OUT("EOF reached\r\n");
  _name_len = 0;
  return false;
}


bool WriteableIniFile::openSection(char * section) {
  // move _section_start to specified section 
  // or EOF if not found
//  _lastError = errorNoError;
  _section_start = 0; // main section
  _value_pos = 0;
  _name = _buf; // main section have no name
  _name_len = 0;
  if (NULL == section) 
    return true;
    
  bool res = seekSection(section, _section_start);
  _value_pos = _section_start; // next param seek from THIS section start (or EOF)
  return res;
}


bool WriteableIniFile::nextSection() {
//  _lastError = errorNoError;
  bool res = seekSection(NULL, _section_start);
  _value_pos = _section_start;
  return res;
}

void WriteableIniFile::resetSection() {
  _value_pos = _section_start;
}

// ----------------- values ---------------

bool WriteableIniFile::seekValue(
           char * aname,  // value to find
           char * &value, size_t &value_len,
           uint32_t &placeholder, size_t &placeholder_len) { // value placeholder file position and length
  // search current section for value
  // NULL = any value

  if (aname == NULL) {
    WIF_DEBUG_OUT("seekValue@%d: -ANY-\r\n", _value_pos /*fp*/);
  } else {
    _value_pos = _section_start;
    WIF_DEBUG_OUT("seekValue@%d: %s\r\n", _value_pos /*fp*/, aname);
  }

  char * ds; // data start ptr
  char * de; // data end ptr
  char * le; // line end ptr
  char * p;

  _name = _buf;
  _name_len = 0;
  value = _buf;
  value_len = 0;
  placeholder_len = 0;
  placeholder = _value_pos; //fp;

  while ( readLine(_value_pos, ds, de, le) ) { // read file lines
    if ('[' == *ds) {  //section end
      _name_len = 0;
      WIF_DEBUG_OUT("seekValue FAIL: Section end reached\r\n");
      return false;
    }

    _name = ds;
    p = ds;
    if (seekChar(value_separator_chars, p, de)) { 
      // name-value separator found
      value = p;
      // skip name trailing spaces. p points to separator char
      while (strchr(space_chars, *--p) && (p >= ds) );
      _name_len = p - ds + 1;
      WIF_DEBUG_OUT("Value: name '%.*s' val '%.*s'\r\n", _name_len, _name, de-value, value+1);
      if ( (NULL == aname) || 
           ( (strlen(aname) == _name_len) && (0 == strncmp(aname, _name, _name_len)) )
         ) {
        // name found, getting value
        placeholder += value - _buf + 1;
        placeholder_len = de - value - 1;
        seekCharNot(space_chars, value, de); // skip value leading spaces
        while (strchr(space_chars, *--de) && (de >= value) ); // skip line trailing spaces
        value_len = de - value + 1;
        WIF_DEBUG_OUT("seekValue SUCCESS: '%.*s' = '%.*s'\r\n", _name_len, _name, value_len, value);
        return true;
      }
    }
    placeholder = _value_pos; // next line start file position
  }
  // all read but no value
  WIF_DEBUG_OUT("seekValue FAIL: EOF reached\r\n");
  _name_len = 0;
  return false;
}


// copy value to external buffer. keep work buffer intact
bool WriteableIniFile::getValueCopy(char * aname, char * buf, size_t asize) {
  char * v;
  size_t v_len;
  uint32_t pl;
  size_t pl_len;

//  _lastError = errorNoError;

  if ( seekValue(aname, v, v_len, pl, pl_len) &&
       (asize < v_len) ) {
    memcpy(buf, v, v_len);
    buf[v_len+1] = '\0';
    return true;
  }
  return false;
}

// if found 
//    modify work buffer to null terminated string and return value pointer
// else
//    return default value pointer
char * WriteableIniFile::getValue(char * aname, char * defval) {
  char * v;
  size_t v_len;
  uint32_t pl;
  size_t pl_len;

//  _lastError = errorNoError;

  WIF_DEBUG_OUT("getValue:");
  if (aname)
    WIF_DEBUG_OUT(" name '%s'", aname);
  if (defval)
    WIF_DEBUG_OUT(" default '%s'", defval);
  WIF_DEBUG_OUT("\r\n");

  if ( seekValue(aname, v, v_len, pl, pl_len) ) {
    v[v_len] = '\0';
    WIF_DEBUG_OUT("Placeholder: %d @ %04X\t", pl_len, pl);
    return v;
  }
  return defval;
}


bool WriteableIniFile::setValue(char * aname, char * new_val, size_t pl_bytes) {
  char * v;
  char * b;
  char * e;
  size_t v_len;
  uint32_t pl;
  size_t pl_len;

//  _lastError = errorNoError;

  WIF_DEBUG_OUT("setValue:");
  if (  (NULL == aname) || 
        (NULL == new_val) ) {
    _lastError = errorInvalidParams;
    return false;
  }
         
  if (  (strlen(new_val) > _buf_size) ||
        (pl_bytes > _buf_size) ) {
    _lastError = errorOutOfBuffer;
    return false;
  }
  
  WIF_DEBUG_OUT(" name '%s' new value '%s'\r\n", aname, new_val);

  if ( seekValue(aname, v, v_len, pl, pl_len) ) {
    // update existing parameter
    WIF_DEBUG_OUT("Placeholder found for %s: %d bytes @ 0x%04X\r\n", aname, pl_len, pl);
    //prepare data to write
    memset(_buf, ' ', max(pl_len, pl_bytes));
    memcpy(_buf, new_val, strlen(new_val));

    if (strlen(new_val) <= pl_len) {
      // data fits in to existing placeholder
      WIF_DEBUG_OUT("Store %d bytes to existing placeholder @ 0x%04X\r\n", pl_len, pl);
      // seek to data insert point
      if (!_file->seek(pl, SeekSet)) {
        WIF_DEBUG_OUT("setVal FAIL: value replace seek %d fail\r\n", pl);
        _lastError = errorFileSeek;
        return false;
      }
      if ( _file->write((uint8_t *)_buf, pl_len) != pl_len ) {
        WIF_DEBUG_OUT("setVal FAIL: write update %d bytes fail\r\n", pl_len);
        _lastError = errorFileWrite;
        return false;
      }
      return true;
    } else {
      // not fit. insert at placeholder start. keep all after placeholder
      WIF_DEBUG_OUT("Value not fit $d bytes @ 0x%04X\r\n", pl_len, pl);
      if (!_file->seek(pl+pl_len, SeekSet)) {
        WIF_DEBUG_OUT("setVal FAIL: seek after placeholder 0x%04X fail\r\n", pl+pl_len);
        _lastError = errorFileSeek;
        return false;
      }
      // extend num bytes to store
      if (pl_bytes > pl_len)
        pl_len = pl_bytes;
    }
  } else {
      // new parameter
      WIF_DEBUG_OUT("New parameter '%s'\r\nLook section end from %04X\r\n", aname, _section_start);
      // seek section end to minimize temporary mem buffer      
      pl = _section_start;
      _value_pos = pl;
      while ( readLine(pl, b, e, v)) {
        WIF_DEBUG_OUT("Got line '%.*s'\r\nChar '%c'\r\n", e-b, b, *b);
        if ('[' == *b) {
          WIF_DEBUG_OUT(" '[' @ 0x%04X return to line start @ 0x%04X\r\n", pl, _value_pos);
          pl = _value_pos;
          break;
        }
        _value_pos = pl;
      }
      
      if (!_file->seek(pl, SeekSet)) {
        WIF_DEBUG_OUT("setVal FAIL: seek section end 0x%04X fail\r\n", pl);
        _lastError = errorFileSeek;
        return false;
      }

      //prepare data to write
      pl_len = strlen(aname)+1+max(strlen(new_val), pl_bytes)+2; // try to extend placeholder
      
      if (pl_len > _buf_size)
        pl_len = strlen(aname)+1+strlen(new_val)+2; // try minimum bytes
        
      if (pl_len > _buf_size) {
        _lastError = errorOutOfBuffer;
        return false;
      }
        
      memset(_buf, ' ', pl_len);
      memcpy(_buf, aname, strlen(aname));
      _buf[strlen(aname)] = '=';
      memcpy(_buf+strlen(aname)+1, new_val, strlen(new_val));
      _buf[pl_len-2] = '\r';
      _buf[pl_len-1] = '\n';
  }

  b = NULL;
  v_len = _file->size()-_file->position();
  if (v_len > 0) {
    // keep the rest file data
    WIF_DEBUG_OUT("Storing data %d bytes @ 0x%04X\r\n",v_len, _file->position());
    b = (char *)malloc(v_len);
    if (NULL == b) {
      _lastError = errorMalloc;
      return false;
    }
    if (_file->read((uint8_t *)b, v_len) != v_len) {
      free(b);
      _lastError = errorFileRead;
      return false;
    };
    
  }

  WIF_DEBUG_OUT("Write 0x%04X '%.*s'\r\n",pl, pl_len, _buf);
  if (!_file->seek(pl, SeekSet)) {
    WIF_DEBUG_OUT("setVal FAIL: seek insert data pos 0x%04X fail\r\n", pl);
    _lastError = errorFileSeek;
    return false;
  }
  if (_file->write((uint8_t *)_buf, pl_len) != pl_len ) {
    WIF_DEBUG_OUT("setVal FAIL: write value %d bytes fail\r\n", pl_len);
    _lastError = errorFileWrite;
    return false;
  }

  if (v_len > 0) {
    // restore 
    WIF_DEBUG_OUT("Restore %d bytes @ 0x%04X\r\n",v_len, _file->position());
    if (_file->write((uint8_t *)b, v_len) != v_len ) {
      WIF_DEBUG_OUT("setVal FAIL: write restore %d bytes fail\r\n", v_len);
      _lastError = errorFileWrite;
      return false;
    }

    free(b);
  }

  return true;
}

// -------------------- Print ---------------
void WriteableIniFile::printIni(Print & p) {
  if ( (_buf_size < 1) || (NULL == _buf) ) {
    WIF_DEBUG_OUT("printIni FAIL: buf too small\r\n");
    return;
  }

  if (!_file->seek(0, SeekSet)) {
    WIF_DEBUG_OUT("printIni FAIL: seek begin\r\n");
    return;
  }

  size_t bytesRead;
  while ( (bytesRead = _file->read((uint8_t *)_buf, _buf_size)) > 0) {
    p.write(_buf, bytesRead);    
  }
}


void WriteableIniFile::printJson(Print & p) {
  p.write('{');

  _section_start = 0; // main section
  _value_pos = 0;
  _name_len = 0;
  bool s;
  bool cv = false; // comma between values
  char * v;
  do {
    if (cv)
      p.write(",\r\n");
    if (s = (_name_len > 0)) { // is section
      p.printf("\"%s\":{\r\n", getLastName());
    }
    cv = false;
    while(v = getValue(NULL, NULL)) {
      if (cv)
        p.write(",\r\n");
      p.printf("\t\"%s\":\"%s\"", getLastName(), v);
      cv = true;
      delay(1);
    }
    if (s)
      p.printf("\r\n}", getLastName());
  } while (nextSection());
  
  p.write('}');
}
