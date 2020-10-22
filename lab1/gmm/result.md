## 练习1及5的相关报告

### 练习2 - 理解通过make生成执行文件的过程

1.ucore.img文件的生成过程<br>
(1)makefile中的具体命令及参数：<br>
(2)使用Make V=设置标记来详细展现执行过程:<br>
  a)GCC将源文件编译为目标文件<br>
  b)链接器将目标文件转换为可执行文件<br>
  c)dd将文件拷贝至虚拟硬盘ucore.img count中，qemu会基于虚拟硬盘中数据执行代码<br>
  d)生成ucore.img需要先生成kernel和bootblock，创建一个大小为10000字节的块，然后再将bootblock，kernel拷贝过去。<br>
bootloader创建过程<br>
由上代码可得，到要生成bootblock，首先需要生成bootasm.o、bootmain.o、sign
用sign工具处理bootblock.out，生成bootblock
bin/sign obj/bootblock.out bin/bootblock<br>
kernel创建过程<br>
2.符合规范的硬盘主引导扇区的特征<br>
tools\sign.c文件完成了特征的标记，代码如下：<br>
可得要求硬盘主引导扇区的大小是512字节，还需要第510个字节是0x55,第511个字节为0xAA<br>
