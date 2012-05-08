
OBJ		= obj-${shell $(CC) -dumpmachine}
LIBS 	+=  -lm
CFLAGS 	+= -std=gnu99 -O2 -g -Wall
DESTDIR	= /usr/local
VPATH	+= src

all: obj ${OBJ}/gcodepp

obj:
	mkdir -p ${OBJ}

${OBJ}/%.o : src/%.c
	$(CC) -MMD $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

${OBJ}/gcodepp: ${OBJ}/g_filter_processor.o
${OBJ}/gcodepp: ${OBJ}/g_filter_antidot.o
${OBJ}/gcodepp: ${OBJ}/g_filter_cleaner.o
${OBJ}/gcodepp: ${OBJ}/g_filter_antijerk.o
${OBJ}/gcodepp: ${OBJ}/gcode.o
${OBJ}/gcodepp: ${OBJ}/gcode_parser.o
${OBJ}/gcodepp: ${OBJ}/gcoderead.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -rf ${OBJ}

install:
	mkdir -p $(DESTDIR)/bin/
	cp ${OBJ}/gcodepp $(DESTDIR)/bin/
 
-include ${wildcard ${OBJ}/*.d}
