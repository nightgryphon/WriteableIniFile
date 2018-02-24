#include <FS.h>
#include <WriteableIniFile.h>

bool DIE(const char * format, ...) {
    va_list args; 
    va_start(args, format); 
    Serial.printf(format, args); 
    va_end(args); 
    while(true)delay(1); 
};

void setup() {
  Serial.begin(9600);
  SPIFFS.begin();

  File iniFile = SPIFFS.open("/wifi.ini", "r+");
  iniFile || DIE("Failed to open config file");

  char buf[56];

  WriteableIniFile config(&iniFile);
//  config.inLineComments = false;
  config.setBuffer(buf, sizeof(buf)) || DIE("Failed to set buffer");

  // show file contents
  config.printIni(Serial);

  Serial.println("\r\n---------------------------\r\n");

  // read value
  config.openSection("WiFi client");
  Serial.printf("SSID: %s\r\n", config.getValue("SSID", "-DEFAULT-"));
  
  // read non existent (commented out) value
  Serial.printf("IP: %s\r\n", config.getValue("IP", "-NOT SET-"));

  // update value
  config.openSection("WiFi client 1");
  config.setValue("Pass","newpassword");

  // add value
  config.setValue("Comment","Reserve connection");

  Serial.println("\r\n---------------------------\r\n");

  // print contents in JSON format
  config.printJson(Serial);

  Serial.println("\r\n---------------------------\r\n");

  // loop through ini contents
  char * v;

  config.openSection(NULL);
  do {
    Serial.printf("=== %s ===\r\n", config.getLastName());
    while(v = config.getValue(NULL, NULL)) {
        Serial.printf("\t%s\t= %s\r\n", config.getLastName(), v);
      delay(1);
    }
  } while (config.nextSection());

  Serial.println("\r\n---------------------------\r\n");

  // add value to the first section which does not have it
  config.openSection(NULL); // jump to file head
  while (NULL != config.getValue("TestName", NULL))
    config.nextSection();
  config.setValue("TestName","TestValue");

  Serial.println("\r\n---------------------------\r\n");

  // show the result
  config.printIni(Serial);

}
 
void loop() {
  delay(1);
}
