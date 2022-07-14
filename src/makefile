
.PHONY : all clean rebuild

KERNEL_SRC := kmain.c      \
              screen.c     \
              kernel.c     \
              utility.c    \
              task.c       \
              interrupt.c  \
              ihandler.c   \
              list.c       \
              queue.c      \
              memory.c     \
              mutex.c      \
              keyboard.c   \
              event.c      \
              sysinfo.c    \
              hdraw.c      \
              fs.c
              
APP_SRC :=    screen.c     \
              utility.c    \
              list.c       \
              queue.c      \
              memory.c     \
              syscall.c    \
              demo1.c      \
              demo2.c      \
              shell.c      \
              app.c

KERNEL_ADDR := B000
APP_ADDR := 12000
IMG := B.L.OS
IMG_PATH := /mnt/hgfs

DIR_DEPS := deps
DIR_EXES := exes
DIR_OBJS := objs

DIRS := $(DIR_DEPS) $(DIR_EXES) $(DIR_OBJS)

KENTRY_SRC := kentry.asm
AENTRY_SRC := aentry.asm
BLFUNC_SRC := blfunc.asm
BOOT_SRC   := boot.asm
LOADER_SRC := loader.asm
COMMON_SRC := common.asm

BOOT_OUT   := boot
LOADER_OUT := loader
KERNEL_OUT := kernel
APP_OUT    := app
KENTRY_OUT := $(DIR_OBJS)/kentry.o
AENTRY_OUT := $(DIR_OBJS)/aentry.o

KERNEL_EXE := kernel.out
KERNEL_EXE := $(addprefix $(DIR_EXES)/, $(KERNEL_EXE))

KERNEL_OBJS := $(KERNEL_SRC:.c=.o)
KERNEL_OBJS := $(addprefix $(DIR_OBJS)/, $(KERNEL_OBJS))
KERNEL_DEPS := $(KERNEL_SRC:.c=.dep)
KERNEL_DEPS := $(addprefix $(DIR_DEPS)/, $(KERNEL_DEPS))

APP_EXE := app.out
APP_EXE := $(addprefix $(DIR_EXES)/, $(APP_EXE))

APP_OBJS := $(APP_SRC:.c=.o)
APP_OBJS := $(addprefix $(DIR_OBJS)/, $(APP_OBJS))
APP_DEPS := $(APP_SRC:.c=.dep)
APP_DEPS := $(addprefix $(DIR_DEPS)/, $(APP_DEPS))

all : $(DIR_OBJS) $(DIR_EXES) $(IMG) $(BOOT_OUT) $(LOADER_OUT) $(KERNEL_OUT) $(APP_OUT)
	@echo "Build Success ==> B.L.OS!"
	
ifeq ("$(MAKECMDGOALS)", "all")
-include $(KERNEL_DEPS)
-include $(APP_DEPS)
endif

ifeq ("$(MAKECMDGOALS)", "")
-include $(KERNEL_DEPS)
-include $(APP_DEPS)
endif

$(IMG) :
	bximage $@ -q -fd -size=1.44
	
$(BOOT_OUT) : $(BOOT_SRC) $(BLFUNC_SRC)
	nasm $< -o $@
	dd if=$@ of=$(IMG) bs=512 count=1 conv=notrunc
	
$(LOADER_OUT) : $(LOADER_SRC) $(COMMON_SRC) $(BLFUNC_SRC)
	nasm $< -o $@
	sudo mount -o loop $(IMG) $(IMG_PATH)
	sudo cp $@ $(IMG_PATH)/$@
	sudo umount $(IMG_PATH)
	
$(KENTRY_OUT) : $(KENTRY_SRC) $(COMMON_SRC)
	nasm -f elf $< -o $@
    
$(KERNEL_OUT) : $(KERNEL_EXE)
	./elf2kobj -c$(KERNEL_ADDR) $< $@
	sudo mount -o loop $(IMG) $(IMG_PATH)
	sudo cp $@ $(IMG_PATH)/$@
	sudo umount $(IMG_PATH)
	
$(KERNEL_EXE) : $(KENTRY_OUT) $(KERNEL_OBJS)
	ld -s $^ -o $@

$(AENTRY_OUT) : $(AENTRY_SRC) $(COMMON_SRC)
	nasm -f elf $< -o $@
    
$(APP_OUT) : $(APP_EXE)
	./elf2kobj -c$(APP_ADDR) $< $@
	sudo mount -o loop $(IMG) $(IMG_PATH)
	sudo cp $@ $(IMG_PATH)/$@
	sudo umount $(IMG_PATH)
	
$(APP_EXE) : $(AENTRY_OUT) $(APP_OBJS)
	ld -s $^ -o $@
		
$(DIR_OBJS)/%.o : %.c
	gcc -fno-builtin -fno-stack-protector -c $(filter %.c, $^) -o $@

$(DIRS) :
	mkdir $@

ifeq ("$(wildcard $(DIR_DEPS))", "")
$(DIR_DEPS)/%.dep : $(DIR_DEPS) %.c
else
$(DIR_DEPS)/%.dep : %.c
endif
	@echo "Creating $@ ..."
	@set -e; \
	gcc -MM -E $(filter %.c, $^) | sed 's,\(.*\)\.o[ :]*,objs/\1.o $@ : ,g' > $@
	
clean :
	rm -fr $(IMG) $(BOOT_OUT) $(LOADER_OUT) $(KERNEL_OUT) $(APP_OUT) $(DIRS)
	
rebuild :
	@$(MAKE) clean
	@$(MAKE) all
