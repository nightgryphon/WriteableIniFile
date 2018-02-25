# Writeable IniFile
---

This library allow to parse, read and write name-value pairs within .ini file.
It supports sections, comments, adding and updating values, default values, 
printing data in INI or JSON format.

The library intends to provide convenient way to work with config files and to minimize memory usage. 
All data processing is performed line by line within provided buffer.
The memory required is to hold one line of config file.

The only case when extra memory allocated is appending new name-value to file or updating
with value longer than available free space within line (value placeholder)

Case insensitive search available using 'lowerCaseNames' property. 
When set to true all read parameter and section names converted to lower case before compare.

Typical usage can be like this:

```C++

  File iniFile = SPIFFS.open("/default.ini", "r+");
  char buf[96];

  // Prepare INI file object
  WriteableIniFile config(&iniFile);
  config.setBuffer(buf, sizeof(buf));
  config.inLineComments = true;

  // read value with default
  config.openSection("Section one");
  Serial.printf("The IP is %s\r\n", getValue("IP", "-- NOT SET --"));

  // write value
  config.setValue("Pass", "newpassword");
  config.printIni(Serial);

  ...

  // use in web server handler
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "application/json", "");
  config.printJsonChunks(server.client(), false);

//  client.printf("%X\r\n", config.getJsonSize());
//  config.printJson(client);


```

Take a look at the examples. They shold be quite self explanary.

To start use WriteableIniFile you should provide an opened File to constructor and 
set the processing buffer by calling setBuffer(buf, sizeof(buf)).\
The processing buffer can be shared between multiple WriteableIniFile instances but you should
notice that every call to read/write parameters or sections will overwrite buffer data.

You can use any File class which support read/write/seek/size/position functions. Initially
this library work with SPIFFS at ESP8266 but shold work with other platforms as well


## Methods & properties:


  **`WriteableIniFile(File * file);`**\
    Constructor. file should be the opened File.

  **`bool setBuffer(char * buf, size_t asize, bool check_buf = true);`**\
    set processing buffer. Should be large enough to accept any single line from file

  **`bool inLineComments;`**\
    Set to 'false' to disable inline comments. Default 'true'

  **`bool lowerCaseNames;`**\
    Set to 'true' to convert all names to lower case for case insensitive search. Default 'false'

  **`bool openSection(char * section);`**\
    Search for specified section and set it as current. NULL is the root section.

  **`bool nextSection();`**\
    Move to the nearest next section.\
    Can be used to loop through sections. 

  **`void getLastName(char * &aname, size_t &name_len);`**\
    Return the pointer and length of last found name of section or parameter.\
    Can be used to loop through sections.

  **`char * getLastName();`**\
    Return null terminated string pointer of last found name of section or parameter.\
    The data is located within processing buffer and will be destroyed by next file read.\
    Can be used to loop through sections.


  **`bool getValueCopy(char * aname, char * buf, size_t asize);`**\
    Copy value to provided buffer.

  **`char * getValue(char * aname, char * defval);`**\
    Get value by name within current section.\
    On success return pointer to null terminated value string within processing buffer.\
    On not found or error returns defval\
    If aname = NULL read next name-value within current section.\
    Can be used to loop through section values.

  **`void resetSection();`**\
    Used to loop through section values. Restart value loop from section start. 

  **`bool setValue(char * aname, char * new_val, size_t pl_bytes = 0);`**\
    Update or create parameter within current section with new value.\ 
    If value fits the existing space it is updated. If not than the extra space is created by 
    moving file contents (creates temporary memory buffer while processing)

  **`void printIni(Print & p);`**\
    print data source file to specified printable.

  **`void printJson(Print & p);`**\
    print data as JSON string to specified printable.

  **`size_t getJsonSize();`**\
    calculates the JSON string length. Can be used with printJson to send data as single 
    HTTP chunk to reduce network overhead

  **`void   printJsonChunks(Print & p, bool closeStream = false);`**\
    print JSON data in HTTP chunked transport encoding to output directly to web client stream.
    closeStream = send final empty chunk.

  **`error_t getLastError()`**\
    Get last error code

  **`void resetError()`**\
    Clean errors.\
    Error is not reset automatically to let you check error summary after set of operations.

  **`bool error()`**\
    was there an error?



## File format example

```INI

 ; comment line
 # also a comment

 Baud = 9600
 Web Server Port=80 ; comments within line. beware of ; # within parameter name or value
 ; To disable inline comments set .inLineComments = false
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

