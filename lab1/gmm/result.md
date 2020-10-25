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
 可得若需生成ucore.img，需生成bootblock与kernel;<br>
 &emsp;a)创建bootblock;
 ```s
 # create bootblock
bootfiles = $(call listf_cc,boot)
$(foreach f,$(bootfiles),$(call cc_compile,$(f),$(CC),$(CFLAGS) -Os -nostdinc))

bootblock = $(call totarget,bootblock)

$(bootblock): $(call toobj,$(bootfiles)) | $(call totarget,sign)
	@echo + ld $@
	$(V)$(LD) $(LDFLAGS) -N -e start -Ttext 0x7C00 $^ -o $(call toobj,bootblock)
	@$(OBJDUMP) -S $(call objfile,bootblock) > $(call asmfile,bootblock)
	@$(OBJDUMP) -t $(call objfile,bootblock) | $(SED) '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $(call symfile,bootblock)
	@$(OBJCOPY) -S -O binary $(call objfile,bootblock) $(call outfile,bootblock)
	@$(call totarget,sign) $(call outfile,bootblock) $(bootblock)

$(call create_target,bootblock)
 ```
  &emsp;在创建过程中，需先对boot文件编译生成对象文件；bootblock的生成依赖与boot文件的对象文件与sign文件,首先，使用ld链接器链接生成bootblock.o文件，设置起始地址为0X7C00；接下来使用objdump进行反汇编生成asm文件，然后将对象文件生成out文件，最后使用sign将out文件生成为bootblock.<br>
  &emsp;b)创建kernel;
   ```s
# create kernel target
kernel = $(call totarget,kernel)

$(kernel): tools/kernel.ld

$(kernel): $(KOBJS)
	@echo + ld $@
	$(V)$(LD) $(LDFLAGS) -T tools/kernel.ld -o $@ $(KOBJS)
	@$(OBJDUMP) -S $@ > $(call asmfile,kernel)
	@$(OBJDUMP) -t $@ | $(SED) '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $(call symfile,kernel)

$(call create_target,kernel)
 ```
在创建过程中，kernel依赖于kernel.ld以及KOBJS（kernel libs,即kern文件夹下文件）,以kernel.ld为链接器脚本使用ld生成文件,接下来使用objdump反汇编出kernel的汇编代码，并将带有符号表的反汇编结果作为sed命令的标准输入进行处理，输出到sym文件，完成kernel的创建.<br>
&emsp;c)生成ucore.img文件
```s
# create ucore.img
UCOREIMG	:= $(call totarget,ucore.img)

$(UCOREIMG): $(kernel) $(bootblock)
	$(V)dd if=/dev/zero of=$@ count=10000
	$(V)dd if=$(bootblock) of=$@ conv=notrunc
	$(V)dd if=$(kernel) of=$@ seek=1 conv=notrunc

$(call create_target,ucore.img)
 ```
使用dd指令，从/dev/zero文件中获取10000个block用于ucore.img，从bootblock文件中获取数据，从kernel文件中获取数据，并且输出到目标文件ucore.img中, 并且跳过第一个block，输出到ucore.img文件中

执行结果如下
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
![代码结果](https://github.com/NKU-OS-Study-Team/ucore/blob/main/lab1/gmm/images/img11.png)<br>
对于最后一行参数作出以下解释：<br>
ebp的值为0x00007bf8，为栈底地址，查看bootblock.asm文件可得栈顶位置为0x0007c00，可得栈中只能存两个值，第一个位置用于调用者ebp的值，第二个位置存放返回地址值，可推出没有传入参数
eip的值为0x00007d72，为调用栈上的下一个函数指令的返回地址，查看bootblock.asm文件，可得为bootmain后第一条指令的地址
args，本应存放前四个输入参数的位置地址，但由于这里没有传入参数，所以实际上无实用意义
