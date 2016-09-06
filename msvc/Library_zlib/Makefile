
TARGET   = lib/zlib.a

SOURCES  = src/adler32.c \
           src/compress.c \
           src/crc32.c \
           src/contrib/minizip/ioapi.c \
           src/contrib/minizip/unzip.c \
           src/deflate.c \
           src/gzclose.c \
           src/gzlib.c \
           src/gzread.c \
           src/gzwrite.c \
           src/infback.c \
           src/inffast.c \
           src/inflate.c \
           src/inftrees.c \
           src/trees.c \
           src/uncompr.c \
           src/zutil.c

OBJECTS  = $(SOURCES:.c=.o)

CXX      = gcc
AR       = ar
CXXFLAGS = -Wall -O3 -fomit-frame-pointer -fPIC \
           -D_LARGEFILE64_SOURCE=1 \
           -DNO_VIZ
INCLUDE  = -I src

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(AR) cru $(TARGET) $(OBJECTS)
	cp src/*.h include
	cp src/contrib/minizip/*.h include

.c.o:
	$(CXX) $(CXXFLAGS) $(INCLUDE) -o $(<:.c=.o) -c $<

clean:
	rm -f $(OBJECTS) $(TARGET)
