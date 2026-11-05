这是我生成 bin 文件时 ccs 编译器会调用的 exe 的代码：

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

#define MAX\_LINE\_LENGTH 256

// 严格验证 HEX 记录格式
int is\_valid\_hex\_record (const char \*line) {
if (line \[0\] != ':') return 0;

size\_t len = strlen(line);
if (len < 11 || len % 2 != 1) return 0;

for (size\_t i = 1; i < len; i++) {
if (!isxdigit(line\[i\])) return 0;
}

char byte\_count\_str\[3\] = {line\[1\], line\[2\], '\\0'};
long byte\_count = strtol(byte\_count\_str, NULL, 16);
if (byte\_count < 0 || byte\_count > 255) return 0;

size\_t expected\_len = 1 + 2 + 4 + 2 + byte\_count \* 2 + 2;
return len == expected\_len;

}

// 解析十六进制字符串
uint32\_t parse\_hex (const char \*str, int num\_chars) {
char buffer \[9\] = {0};
strncpy (buffer, str, num\_chars);
return (uint32\_t) strtoul (buffer, NULL, 16);
}

// 解析十六进制或十进制参数
uint32\_t parse\_address\_arg (const char \*str) {
if (strncmp (str, "0x", 2) == 0 || strncmp (str, "0X", 2) == 0) {
return (uint32\_t) strtoul (str + 2, NULL, 16);
}
return (uint32\_t) strtoul (str, NULL, 10);
}

uint32\_t Crc32 (uint8\_t \*data, uint32\_t Length)
{
uint32\_t crc = 0xFFFFFFFF;  // 初始化为 0xFFFFFFFF

while (Length--)
{
// 处理当前字节（8 位）
crc ^= ((uint32\_t)(\*data++) << 24);  // 将字节移到最高位

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

int main(int argc, char \*argv\[\]) {

setvbuf (stdout, NULL, \_IONBF, 0); // 可选：防止缓存导致顺序错乱

if (argc < 5) {
fprintf(stderr, "usage: %s  \[The minimum address available for DSP FLASH\] \[The maximum address available for DSP FLASH\] \[hex2bin.exe\]\\n", argv\[0\]);
fprintf(stderr, "Example: %s firmware.hex 0x00080000 0x00A00000 .\\hex2bin.exe", argv\[0\]);
return EXIT\_FAILURE;
}</hex>

// 解析范围参数
uint32\_t range\_min = 0;
uint32\_t range\_max = UINT32\_MAX;
int range\_specified = 0;

if (argc >= 4) {
range\_min = parse\_address\_arg(argv\[2\]);
range\_max = parse\_address\_arg(argv\[3\]);
range\_specified = 1;

if (range\_min > range\_max) {
fprintf(stderr, "Error: minimum address (0x%08X) > maximum address (0x%08X)\\n", range\_min, range\_max);
return EXIT\_FAILURE;
}
}

FILE \*file = fopen(argv\[1\], "r");
if (!file) {
perror("Open file error");
return EXIT\_FAILURE;
}

uint32\_t global\_min\_addr = UINT32\_MAX;
uint32\_t global\_max\_addr = 0;
uint32\_t range\_min\_addr = UINT32\_MAX;
uint32\_t range\_max\_addr = 0;
uint32\_t current\_ext\_addr = 0;
char line \[MAX\_LINE\_LENGTH\];
int line\_num = 0;
int eof\_found = 0;
int data\_record\_found = 0;
int range\_data\_found = 0;
int debug\_output = 0;  // 启用调试输出

while (fgets(line, sizeof(line), file)) {
line\_num++;

// 移除所有空白字符
size\_t len = strlen (line);
size\_t j = 0;
for (size\_t i = 0; i < len; i++) {
if (!isspace ((unsigned char) line \[i\])) {
line \[j++\] = line \[i\];
}
}
line \[j\] = '\\0';
len = j;

if (len == 0) continue;

if (!is\_valid\_hex\_record(line)) {
printf("error(Row %d): Invalid hex record: %s\\n", line\_num, line);
continue;
}

uint8\_t byte\_count = parse\_hex(line + 1, 2);
uint16\_t offset\_addr = parse\_hex(line + 3, 4);
uint8\_t record\_type = parse\_hex(line + 7, 2);

switch (record\_type) {
case 0x00: { // 数据记录
data\_record\_found = 1;
uint32\_t base\_addr = current\_ext\_addr + offset\_addr;
uint32\_t end\_addr = base\_addr + (byte\_count/2);

// 更新全局最小地址
if (base\_addr < global\_min\_addr) {
global\_min\_addr = base\_addr;
if (debug\_output) printf ("row % d: New global minimum address 0x%08X\\n", line\_num, global\_min\_addr);
}

// 更新全局最大地址
if (end\_addr> global\_max\_addr) {
global\_max\_addr = end\_addr;
if (debug\_output) printf ("row % d: New global maximum address 0x%08X\\n", line\_num, global\_max\_addr);
}

// 范围查找逻辑
if (range\_specified) {
// 检查记录是否完全或部分在指定范围内
if ((base\_addr >= range\_min && base\_addr <= range\_max) ||
(end\_addr >= range\_min && end\_addr <= range\_max) ||
(base\_addr <= range\_min && end\_addr >= range\_max)) {

range\_data\_found = 1;

// 更新范围内的最小地址
if (base\_addr>= range\_min && base\_addr < range\_min\_addr) {
range\_min\_addr = base\_addr;
if (debug\_output) printf ("row % d: Minimum address in the new scope 0x%08X\\n", line\_num, range\_min\_addr);
}

// 更新范围内的最大地址
if (end\_addr <= range\_max && end\_addr> range\_max\_addr) {
range\_max\_addr = end\_addr;
if (debug\_output) printf ("row % d: Maximum address in the new scope 0x%08X\\n", line\_num, range\_max\_addr);
}

// 如果记录跨越范围边界，则更新边界值
if (base\_addr < range\_min && end\_addr>= range\_min && end\_addr > range\_max\_addr) {
range\_max\_addr = (end\_addr > range\_max) ? range\_max : end\_addr;
}
if (end\_addr > range\_max && base\_addr <= range\_max && base\_addr < range\_min\_addr) {
range\_min\_addr = (base\_addr < range\_min) ? range\_min : base\_addr;
}
}
}
break;
}

case 0x04: // 扩展线性地址记录
if (byte\_count == 2) {
current\_ext\_addr = parse\_hex (line + 9, 4) << 16;
if (debug\_output) printf ("row % d: Set extended address 0x%08X\\n", line\_num, current\_ext\_addr);
}
break;

case 0x01: // 文件结束记录
eof\_found = 1;
if (debug\_output) printf ("row % d: EOF\\n", line\_num);
break;

case 0x02: // 扩展段地址记录
if (byte\_count == 2) {
uint16\_t seg\_addr = parse\_hex (line + 9, 4);
current\_ext\_addr = seg\_addr << 4;
if (debug\_output) printf ("行 % d: 设置段地址 0x%04X (基址: 0x%05X0)\\n",
line\_num, seg\_addr, seg\_addr);
}
break;

default:
if (debug\_output) printf ("行 % d: 忽略记录类型 0x%02X\\n", line\_num, record\_type);
break;
}

if (eof\_found) break;
}

fclose(file);

printf("\\n=== HEX File address analysis results ===\\n");

if (!data\_record\_found) {
printf("Error: No valid data record found\\n");
return EXIT\_FAILURE;
}

printf("Global address range:\\n");
printf("  Minnimum address: 0x%08X\\n", global\_min\_addr);
printf("  Maximum address: 0x%08X\\n", global\_max\_addr);
printf("  Total size: %u Bytes\\n", (global\_max\_addr - global\_min\_addr)\*2);

if (range\_specified) {
printf("\\nSpecify address range \[0x%08X - 0x%08X\] analysis:\\n", range\_min, range\_max);

if (range\_data\_found) {
printf("  The minimum address within the range: 0x%08X\\n", range\_min\_addr);
printf("  The maximum address within the range: 0x%08X\\n", range\_max\_addr);
printf("  Actual size within the range: %u Bytes\\n", (range\_max\_addr - range\_min\_addr)\*2);

uint32\_t range\_total = range\_max - range\_min + 1;
uint32\_t range\_used = range\_max\_addr - range\_min\_addr + 1;
printf("  Scope utilization rate: %.2f%%\\n", ((float)range\_used / range\_total) \* 100);

// 检查范围是否完全包含在全局范围内
if (range\_min\_addr < global\_min\_addr || range\_max\_addr> global\_max\_addr) {
printf ("Warning: The specified range is not entirely within the global address range\\n");
}
} else {
printf ("No data found within the specified range\\n");
printf ("Global address range: 0x%08X - 0x%08X\\n", global\_min\_addr, global\_max\_addr);

// 检查范围是否完全在全局范围之外
if (range\_min> global\_max\_addr) {
printf ("Tip:  Global address range prompt: The specified range is after the global address range (minimum specified address 0x%08X > global maximum address 0x%08X)\\n",
range\_min, global\_max\_addr);
} else if (range\_max < global\_min\_addr) {
printf ("Tip: The specified range is before the global address range (maximum specified address 0x%08X < global minimum address 0x%08X)\\n",
range\_max, global\_min\_addr);
} else {
printf ("Tip: The specified range overlaps with the global address range but no data records were found\\n");
printf ("Global address range: 0x%08X - 0x%08X\\n", global\_min\_addr, global\_max\_addr);
printf ("Specified range:    0x%08X - 0x%08X\\n", range\_min, range\_max);
}
}
}

// 只有输出 5 个参数时，才生成 bin 文件和 CRC 文件
if (argc == 5)
{
char command \[256\];

sprintf(command, "%s -w -v -s 0x%08X -l 0x%08X -a -p 0xFFFF %s > NUL 2>&1", argv\[4\], range\_min\_addr, (range\_max\_addr - range\_min\_addr)\*2, argv\[1\]);
printf("Converting...\\r\\n");
system(command);

printf("Hex2BinFile conversion completed\\r\\nPlease check if a. bin file with the same name exists, with a file size of: %d Bytes\\r\\n", (range\_max\_addr - range\_min\_addr)\*2);

}

return EXIT\_SUCCESS;

}