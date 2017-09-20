build_dir := build

obj := $(build_dir)/server.o $(build_dir)/connection.o $(build_dir)/pika.o $(build_dir)/port.o $(build_dir)/redis_command.o $(build_dir)/redis_db.o $(build_dir)/redis_kv.o $(build_dir)/redis_list.o $(build_dir)/redis_list_meta.o $(build_dir)/redis_set.o $(build_dir)/redis_set_meta.o $(build_dir)/hash.o $(build_dir)/meta_db.o

.PHONY: all

all: pika $(obj)

CC := g++

CFLAGS := -g -lmuduo_net_cpp11 -lmuduo_base_cpp11 -lrocksdb -lz -lbz2 -lforestdb -pthread -Iinclude -std=c++11 -Wall

pika: $(obj)
	$(CC) -o build/pika  $(wildcard build/*.o) $(CFLAGS)

$(build_dir)/%.o: src/%.cc
	@test -d build || mkdir -p build
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@rm -rf build
