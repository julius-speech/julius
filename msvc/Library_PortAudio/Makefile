
TARGET   = lib/PortAudio.a

SOURCES  = src/src/common/pa_allocation.c \
           src/src/common/pa_converters.c \
           src/src/common/pa_cpuload.c \
           src/src/common/pa_debugprint.c \
           src/src/common/pa_dither.c \
           src/src/common/pa_front.c \
           src/src/common/pa_process.c \
           src/src/common/pa_ringbuffer.c \
           src/src/common/pa_stream.c \
           src/src/common/pa_trace.c \
           src/src/hostapi/alsa/pa_linux_alsa.c \
           src/src/hostapi/oss/pa_unix_oss.c \
           src/src/hostapi/oss/recplay.c \
           src/src/os/unix/pa_unix_hostapis.c \
           src/src/os/unix/pa_unix_util.c

OBJECTS  = $(SOURCES:.c=.o)

CXX      = gcc
AR       = ar
CXXFLAGS = -Wall -O3 -fomit-frame-pointer -fPIC \
           -pthread \
           -DHAVE_LINUX_SOUNDCARD_H=1 \
           -DPA_USE_ALSA=1 \
           -DPA_USE_OSS=1
INCLUDE  = -I src/include \
           -I src/src/common \
           -I src/src/hostapi/alsa \
           -I src/src/hostapi/oss \
           -I src/src/os/unix

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(AR) cru $(TARGET) $(OBJECTS)
	cp src/include/*.h include

.c.o:
	$(CXX) $(CXXFLAGS) $(INCLUDE) -o $(<:.c=.o) -c $<

clean:
	rm -f $(OBJECTS) $(TARGET)
