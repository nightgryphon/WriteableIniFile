# Writeable IniFile
---

This library allow to parse, read and write name-value pairs within .ini file.
It supports sections, comments, adding and updating values, printing data in ini or json format.

Typical usage can be like this:

```C++

  File iniFile = SPIFFS.open("/default.ini", "r+");
  char buf[128];

  WriteableIniFile config(&iniFile);
  config.setBuffer(buf, sizeof(buf));
  config.fullLineComments = true;

  config.openSection("Section one");
  Serial.printf("The IP is %s\r\n", getValue("IP", "-- NOT SET --"));
  config.setValue("Pass", "newpassword");
  config.printIni(Serial);

  ...

  config.printJson(webclient);

```

Take a look at the examples. They shold be quite self explanary.

To start use WriteableIniFile you should provide an opened File to constructor and 
set the processing buffer by calling setBuffer(buf, sizeof(buf))

You can use any File class which support read/write/seek/size/position functions. Initially
this library work with SPIFFS at ESP8266 but shold work with other platforms as well

All data processing is performed within provided buffer. Buffer shold be large enough to 
read any single line from ini file.

The only case when extra memory allocated is appending new name-value to file or updating
with value longer than available free space within line (value placeholder)


## Methods & properties:


  **`WriteableIniFile(File * file);`**\
    Constructor. file should be the opened File.

  **`bool setBuffer(char * buf, size_t asize, bool check_buf = true);`**\
    set processing buffer. Should be enough to accept any single line from this file

  **`bool fullLineComments;`**\
    Set to 'true' to disable inline comments. Default 'false'

  **`bool openSection(char * section);`**\
    Search for specified section and set it as current. NULL is the root section.

  **`bool nextSection();`**\
    Move to the nearest next section. Can be used to loop through sections. 

  **`void getLastName(char * &aname, size_t &name_len);`**\
    Return the pointer and length of last found name of section or parameter.
    Can be used to loop through sections.

  **`char * getLastName();`**\
    Return null terminated string pointer of last found name of section or parameter.
    The data is located within processing buffer and will be destroyed by next file read.
    Can be used to loop through sections.


  **`bool getValueCopy(char * aname, char * buf, size_t asize);`**\
    Copy the last found value to provided buffer.

  **`char * getValue(char * aname, char * defval);`**\
    Get value by name within current section.
    On success return pointer to null terminated value string within processing buffer.
    On not found or error returns defval
    If aname = NULL read next name-value within current section. Can be used to loop through section values.

  **`void resetSection();`**\
    Used to loop through section values. Restart value loop from section start. 

  **`bool setValue(char * aname, char * new_val, size_t pl_bytes = 0);`**\
    Update or create parameter within current section with new value. 
    If value fits the existing space it is updated. If not than the extra space is created by 
    moving file contents (creates temporary memory buffer while processing)

  **`void printIni(Print & p);`**\
    print data source file to specified printable.

  **`void printJson(Print & p);`**\
    print data in JSON format to specified printable. Can be used with web server routines.


  **`error_t getLastError()`**\
    Get last error code

  **`void resetError()`**\
    Error is not reset automatically to let you check error summary after set of operations.

  **`bool error()`**\
    was there an error?



## File format example

```INI

 ; comment line
 # also a comment

 Baud = 9600
 Web Server Port=80 ; comments within line. beware of ; # within parameter name or value
 ; To disable inline comments set .fullLineComments = true
 SSID = WiFi

 [ Section one ]
 ; use openSection(section) to select current section. NULL for root section
 SSID = Another WiFi
 Pass = password        ; here is some placeholder space left to update value inplace
 IP = 192.168.0.1

 [WiFi AP]
 SSID = Test
 Pass = 12345678
 IP = 192.168.4.1
 MASK = 255.255.255.0
 GW = 192.168.4.1

```

