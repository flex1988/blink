build_dir := build

obj := $(build_dir)/server.o $(build_dir)/connection.o $(build_dir)/pika.o $(build_dir)/port.o $(build_dir)/command.o $(build_dir)/redisdb.o $(build_dir)/redis_kv.o $(build_dir)/redis_list.o $(build_dir)/meta.o

.PHONY: all

all: pika $(obj)

CC := g++

CFLAGS := -g -lmuduo_net -lmuduo_base -lrocksdb -lz -lbz2 -pthread -Iinclude -std=c++11

pika: $(obj)
	$(CC) -o build/pika  $(wildcard build/*.o) $(CFLAGS)

$(build_dir)/%.o: src/%.cc
	@test -d build || mkdir -p build
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@rm -rf build
