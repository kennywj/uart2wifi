#
# 'make depend' to automatically generate dependences
# 'make' to rebuild object codes
# 'make clean' remove object files
#
ODIR = ../build
LIB = $(ODIR)/libfatfs.a
INCS =  -I./src -I./port -I./src/option -I../include -I../FreeRTOS_Posix    \
    -I../FreeRTOS_Posix/FreeRTOS_Kernel/include -I../port -I../shell
VPATH = ./src:./src/option:./port
	
SRCS = diskio.c ff.c cc950.c syscall.c unicode.c    \
    fattime.c fs_port.c ramdisk.c
	
OBJS = $(patsubst %.c,$(ODIR)/%.o,$(SRCS))

.PHONY: clean depend

all: $(LIB)

$(LIB): $(OBJS)
	@$(AR) cr $@ $^
	@echo "    Archive    $(notdir $@)"

$(ODIR)/%.o: %.c
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@  

clean:
	rm -f $(OBJS) $(LIB)

depend: $(SRCS)
	makedepend $(INCS) $<
