PREFIX=~/
BINDIR= $(PREFIX)/bin
SUBDIRS =
INCLUDES = 
LIBS = -lpthread
C_INCLUDE_PATH=
CPLUS_INCLUDE_PATH=
OBJC_INCLUDE_PATH=
CPATH=
LIBRARY_PATH=
LD_LIBRARY_PATH=/home/zhyh/Desktop/SISIN/4.9.2_0.9.33/lib
LANG=
#LC_ALL=zh_CN.UTF-8

PROGRAM = $(addsuffix ,$(notdir $(CURDIR)))
#cross compiler path
#CROSS_COMPILER=
#CROSS_COMPILER=/home/zhyh/Desktop/ALT/4.1.0-uclibc-0.9.28/bin/
CROSS_COMPILER=/home/zhyh/Desktop/SISIN/4.9.2_0.9.33/bin/

#CC=$(CROSS_COMPILER)gcc
#CXX=$(CROSS_COMPILER)g++
#CC=$(CROSS_COMPILER)mipsel-linux-uclibc-gcc
#CXX=$(CROSS_COMPILER)mipsel-linux-uclibc-g++
CC=$(CROSS_COMPILER)mipsel-buildroot-linux-uclibc-gcc
CXX=$(CROSS_COMPILER)mipsel-buildroot-linux-uclibc-g++

#used by gcc
CFLAGS= -O3
#used by g++
CXXFLAGS=
#used by linker
LDFLAGS=

CPP_SRCS =  $(wildcard *.cpp)
C_SRCS = $(wildcard *.c)
SRCS = $(C_SRCS) $(CPP_SRCS)
OBJS = $(SRCS:.* = .o)
DEPS = $(OBJS:.o = .d)

all: $(PROGRAM)

# Rules for creating dependency files(.d).
%.d : %.c
	@${CC} -MM -MD $(CFLAGS) $<

%.d : %.cpp
	@${CXX} -MM -MD $(CXXFLAGS) $<

# Rules for compiling source files to object files(.o)
ifeq ($(filter-out %.cpp,$(SRCS)),)              # has cpp code
%.o : %.cpp
	${CXX} -c ${CXXFLAGS} ${INCLUDES} $< -o $@

%.o : %.c
	${CXX} -c ${CFLAGS} ${INCLUDES}  $< -o $@
else
%.o : %.c
	${CC} -c ${CFLAGS} ${INCLUDES}  $< -o $@
endif

$(PROGRAM):  $(OBJS)
ifeq ($(filter-out %.cpp,$(SRCS)),)              # has cpp code
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(OBJS) -o $(PROGRAM) $(LIBS)
else                            # C++ program
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $(PROGRAM) $(LIBS)
endif  


uninstall:
	cd $(BINDIR) && if [ -f "./$(PROGRAM)" ];then rm $(PROGRAM);fi
install:
	cp -f $(PROGRAM) $(BINDIR)
pre-build:
post-build:
	cp $(CURDIR)/$(PROGRAM) $(BIN_PATH)/
clean:
	rm -rf *.o $(PROGRAM)
	
