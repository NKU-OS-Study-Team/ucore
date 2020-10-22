## 练习1及5的相关报告

### 练习1 - 理解通过make生成执行文件的过程

1.ucore.img文件的生成过程<br>
(1)makefile中的具体命令及参数：<br>
```s
# create ucore.img
UCOREIMG	:= $(call totarget,ucore.img)

$(UCOREIMG): $(kernel) $(bootblock)
	$(V)dd if=/dev/zero of=$@ count=10000
	$(V)dd if=$(bootblock) of=$@ conv=notrunc
	$(V)dd if=$(kernel) of=$@ seek=1 conv=notrunc

$(call create_target,ucore.img)
 ```
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


### 练习5 - 实现函数调用堆栈跟踪函数<br>
函数print_stackframe的实现代码如下：<br>
运行结果如下:<br>
对于最后一行参数作出以下解释：<br>
ebp的值为0x00007bf8，查看bootblock.asm文件可得栈顶位置为0x0007c00，可得栈中只能存两个值，第一个位置用于调用者ebp的值，第二个位置存放返回地址值，可推出没有传入参数
eip的值为0x00007d72，查看bootblock.asm文件，可为得bootmain后第一条指令的地址
args，本应存放前四个输入参数的位置地址，但由于这里没有传入参数，所以实际上是其他数据
