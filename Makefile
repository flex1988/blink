build_dir := build

obj := $(build_dir)/server.o $(build_dir)/connection.o $(build_dir)/port.o $(build_dir)/command.o $(build_dir)/db.o $(build_dir)/kv.o $(build_dir)/list.o $(build_dir)/list_meta.o $(build_dir)/set.o $(build_dir)/set_meta.o $(build_dir)/hash.o $(build_dir)/meta_base.o $(build_dir)/aof.o

tests = \
	$(build_dir)/db_test \
	$(build_dir)/meta_test

.PHONY: all

all: pika $(obj) $(test)

CC := g++

CFLAGS := -g -lmuduo_net_cpp11 -lmuduo_base_cpp11 -lrocksdb -lz -lbz2 -lgtest -pthread -Iinclude -std=c++11 -Wall

pika: $(obj)
	$(CC) -o build/pika src/pika.cc $(wildcard build/*.o) $(CFLAGS)

check: $(tests)
	@rm -rf /tmp/db
	@mkdir /tmp/db
	@for t in $(notdir $(tests)); do echo "***** Running $$t"; $(build_dir)/$$t || exit 1; done

$(build_dir)/db_test: $(obj)
	$(CC) src/db_test.cc $(obj) $(CFLAGS) -o $(build_dir)/db_test

$(build_dir)/meta_test: $(obj)
	$(CC) src/meta_test.cc $(obj) $(CFLAGS) -o $(build_dir)/meta_test

$(build_dir)/%.o: src/%.cc
	@test -d build || mkdir -p build
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@rm -rf build

tclean:
	@rm build/*test
