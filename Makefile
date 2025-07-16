CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
TARGET = test_malloc
SOURCES = malloc.c syscall_table.c test_malloc.c
OBJECTS = $(SOURCES:.c=.o)

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJECTS) $(TARGET)

# 运行内存泄漏检查（需要valgrind）
memcheck: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)

# 生成文档
docs:
	doxygen Doxyfile

# 安装（如果需要）
install: $(TARGET)
	cp malloc.h /usr/local/include/
	cp malloc.o /usr/local/lib/libmalloc.a

# 卸载
uninstall:
	rm -f /usr/local/include/malloc.h
	rm -f /usr/local/lib/libmalloc.a 