#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

// 整个文件头占据 32 个 Bytes
#pragma pack (push, 1)           // 1 字节对齐，防止填充
typedef struct
{
uint16\_t bChipType:8;                 // 芯片类型
uint16\_t bFirmwareType:8;             // 固件类型
uint32\_t u32FileSize;                 // 文件大小
uint32\_t u32FileCrc;                  // 32 位 CRC，不包括文件头
uint16\_t u16Rsv \[11\];                  // 保留位
} ST\_UPGRADE\_FILE\_HEADER;
#pragma pack (pop)

// CAN 总线 CRC32 计算函数
uint32\_t CanBusCrc32 (uint8\_t \*data, uint32\_t length)
{
uint32\_t crc = 0xFFFFFFFF;  // 初始值 0xFFFFFFFF

for (uint32\_t i = 0; i < length; i++)
{
// 处理当前字节（8 位）
crc ^= ((uint32\_t) data \[i\] << 24);  // 将字节移到最高位

// 处理当前字节的 8 位
for (uint8\_t bit = 0; bit < 8; bit++)
{
if (crc & 0x80000000) {
crc = (crc << 1) ^ 0x04C11DB7;  // 多项式异或
} else {
crc <<= 1;  // 普通移位
}
}
}

return crc;

}

// 计算文件的 CRC 值
uint32\_t CalculateFileCrc (const char \*filename)
{
FILE \*file = fopen (filename, "rb");
if (!file) {
perror ("无法打开文件");
return 0;
}

// 获取文件大小
fseek (file, 0, SEEK\_END);
long file\_size = ftell (file);
fseek (file, 0, SEEK\_SET);

if (file\_size <= 0) {
fclose (file);
fprintf (stderr, "空文件或无效文件大小 \\n");
return 0;
}

// 分配内存读取文件内容
uint8\_t \*buffer = (uint8\_t \*) malloc (file\_size);
if (!buffer) {
fclose (file);
fprintf (stderr, "内存分配失败 \\n");
return 0;
}

// 读取整个文件
size\_t bytes\_read = fread (buffer, 1, file\_size, file);
fclose (file);

if (bytes\_read != (size\_t) file\_size) {
free (buffer);
fprintf (stderr, "文件读取不完整 \\n");
return 0;
}

// 计算 CRC
uint32\_t crc = CanBusCrc32 (buffer, bytes\_read);
free (buffer);

return crc;

}

int main(int argc, char *argv\[\])
{
const char* filename = argv\[3\];

char ChipType = 0;
char FirmwareType = 0;
unsigned int FileSize = 0;
unsigned char buffer \[4096\];
int ReadSize = 0;
ST\_UPGRADE\_FILE\_HEADER stUpgradeFileheader;     // 目标头

memset(&stUpgradeFileheader, 0, sizeof(ST\_UPGRADE\_FILE\_HEADER));

if (argc < 4)
{
printf("parameter error\\n");
printf("argv\[1\]:ChipType argv\[2\]:FirmwareType argv\[3\]:Source file \\n");
return 1;
}

ChipType = strtol(argv\[1\], NULL, 10);
if (ChipType == 0) {
printf("Please enter a valid integer argv\[1\]:1~255\\n");
return 1;
}

FirmwareType = strtol (argv \[2\], NULL, 10);
if (FirmwareType == 0) {     /\* 不是纯数字 \*/
printf ("Please enter a valid integer argv \[2\]:1~255\\n");
return 1;
}

// 1. 打开原始文件
FILE\* orig\_file = fopen (filename, "rb");
if (!orig\_file) {
printf ("Original file failed to open");
return 1;
}
else
{
fseek (orig\_file, 0, SEEK\_END);
FileSize = ftell (orig\_file);
fseek (orig\_file, 0, SEEK\_SET);

printf("Current file size: %d bytes\\r\\n", FileSize);
}

// 拿到 bin 文件的名字
const char \*full\_filename  = argv \[3\];
uint32\_t crc =  CalculateFileCrc (full\_filename);

// 创建不带后缀的文件名副本
char base\_filename \[256\] = {0};
strncpy (base\_filename, full\_filename, sizeof (base\_filename) - 1);

// 查找并移除 .bin 后缀
char \*dot\_position = strrchr (base\_filename, '.');  // 查找最后一个点
if (dot\_position) {
if (strcasecmp (dot\_position, ".bin") == 0) {  // 不区分大小写比较
strcpy (dot\_position, "\_UpgradeCRC32.bin");  // 替换为\_UpgradeCRC.bin
}
}
else {
strncat (base\_filename, "\_UpgradeCRC32.bin", sizeof (base\_filename) - strlen (base\_filename) - 1);
}

stUpgradeFileheader.u32FileCrc = crc;
stUpgradeFileheader.bChipType = ChipType;
stUpgradeFileheader.bFirmwareType = FirmwareType;
stUpgradeFileheader.u32FileSize = FileSize;

// 2. 创建临时文件
FILE\* temp\_file = fopen (base\_filename, "wb");
if (!temp\_file) {
printf ("Temporary file creation failed");
fclose (orig\_file);
return 1;
}

if (fwrite(&stUpgradeFileheader, sizeof(ST\_UPGRADE\_FILE\_HEADER), 1, temp\_file) != 1)
{
printf("Failed to write file header\\r\\n");
fclose(orig\_file);
fclose(temp\_file);
remove(temp\_file);
return 1;
}

while ((ReadSize = fread(buffer, sizeof(unsigned char), sizeof(buffer), orig\_file)) > 0)
{
if (fwrite(buffer, sizeof(unsigned char), ReadSize, temp\_file) != ReadSize)
{
printf("File write failed\\r\\n");
fclose(orig\_file);
fclose(temp\_file);
remove(temp\_file);
return 1;
}
}

fclose(orig\_file);
if (fclose(temp\_file) != 0)
{
printf("Temporary file closure failed");
remove(temp\_file);
return 1;
}

printf("File %s CRC32: 0x%08X\\n", base\_filename, crc);
printf("File %s ChipType: 0x%04X(%d)\\n", base\_filename, ChipType, ChipType);
printf("File %s FirmwareType: 0x%04X(%d)\\n", base\_filename, FirmwareType, FirmwareType);
printf("File %s FileSize: 0x%04X(%d) bytes\\n", base\_filename, FileSize + sizeof(ST\_UPGRADE\_FILE\_HEADER), FileSize + sizeof(ST\_UPGRADE\_FILE\_HEADER));

}