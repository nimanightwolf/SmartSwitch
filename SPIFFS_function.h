#include "FS.h"
#include "SPIFFS.h"

String FSlistDir(String dirname, uint8_t levels)
{
  String FSdata;
  File root = SPIFFS.open(dirname);
  File file = root.openNextFile();
  while (file)
  {
    FSdata += "\n";
    if (file.isDirectory())
    {
      FSdata += "DIR : " + String(file.name());
      if (levels)FSlistDir(file.name(), levels - 1);
    }
    else FSdata += "FILE: " + String(file.name()) + "\tSIZE: " + String(file.size());
    file = root.openNextFile();
  }
  return FSdata;
}

String FSreadFile(String path)
{
  char ChFsData;
  String FsData;
  File file = SPIFFS.open(path);
  if (!file || file.isDirectory()) return "Error";
  while (file.available())
  {
    ChFsData = file.read();
    FsData += String(ChFsData);
  }
  file.close();
  return FsData;
}

String FSwriteFile(String path, String message)
{
  File file = SPIFFS.open(path, FILE_WRITE);
  if (!file) return "Error";
  if (file.print(message)) return "- file written";
  else return "- write failed";
  file.close();
}

String FSappendFile(String path, String message)
{
  File file = SPIFFS.open(path, FILE_APPEND);
  if (!file) return "Error";
  if (file.print(message)) return "- message appended";
  else return "- append failed";
  file.close();
}

String FSrenameFile(String path1, String path2)
{
  if (SPIFFS.rename(path1, path2)) return "OK";
  else return "Error";
}

String FSdeleteFile(String path)
{
  if (SPIFFS.remove(path)) return "OK";
  else return "Error";
}

String FStestFileIO(String path)
{
  static uint8_t buf[512];
  size_t len = 0;
  File file = SPIFFS.open(path, FILE_WRITE);
  if (!file) return "Error";

  size_t i;
  Serial.print("- writing" );
  uint32_t start = millis();
  for (i = 0; i < 2048; i++)
  {
    if ((i & 0x001F) == 0x001F)
    {
      Serial.print(".");
    }
    file.write(buf, 512);
  }
  Serial.println("");
  uint32_t end = millis() - start;
  Serial.printf(" - %u bytes written in %u ms\r\n", 2048 * 512, end);
  file.close();

  file = SPIFFS.open(path);
  start = millis();
  end = start;
  i = 0;
  if (file && !file.isDirectory())
  {
    len = file.size();
    size_t flen = len;
    start = millis();
    Serial.print("- reading" );
    while (len)
    {
      size_t toRead = len;
      if (toRead > 512)
      {
        toRead = 512;
      }
      file.read(buf, toRead);
      if ((i++ & 0x001F) == 0x001F)
      {
        Serial.print(".");
      }
      len -= toRead;
    }
    Serial.println("");
    end = millis() - start;
    Serial.printf("- %u bytes read in %u ms\r\n", flen, end);
    file.close();
  }
  else
  {
    Serial.println("- failed to open file for reading");
  }
}
