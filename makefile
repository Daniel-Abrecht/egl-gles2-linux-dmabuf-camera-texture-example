
SOURCES += src/main.c
SOURCES += src/egl_x11.c
SOURCES += src/engine.c

OBJECTS = $(addprefix build/,$(addsuffix .o,$(SOURCES)))

all: bin/test

bin/test: $(OBJECTS)
	mkdir -p $(dir $@)
	gcc $^ -lGLESv2 -lEGL -lX11 -o $@

build/%.c.o: %.c
	mkdir -p $(dir $@)
	gcc -g -Og -I include -std=c11 -Wall -Wextra -pedantic -Werror $< -c -o $@

clean:
	rm -rf build bin
