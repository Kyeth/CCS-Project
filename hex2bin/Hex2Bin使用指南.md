Hex2Bin使用指南



>   [!IMPORTANT]
>
>   CalcHexAddr.exe:用于计算hex文件的使用flash区的起始地址和最大地址,然后会调用hex2bin.exe程序，生成与hex文件同名的bin文件
>
>   参数[0]:CalcHexAddr.exe
>
>   参数[1]:hex文件
>
>   参数[2]:DSP的Flash空间的起始地址
>
>   参数[3]:DSP的Flash空间的结束地址
>
>   参数[4]:hex2bin.exe



>   [!IMPORTANT]
>
>   Hex2Bin.exe:用于将hex文件转成可用于DSP烧写的bin文件
>
>   也可以直接调用，例如
>   .\hex2bin.exe -w -v -s 起始地址 -l 数据长度 -a -p 0xFFFF 目标文件.hex
>   最终会生成一个同名的bin文件

