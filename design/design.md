### LIST

pika 的list数据结构 meta保存list的头和尾，遍历操作需要遍历每个元素，操作复杂度为O(n)

2017_9_18. 将pika的存储改为二级索引，Lindex复杂度为O(logN)，然而每次lpush需要保存第一级索引和第二个索引两个block，写入数据量大，容易引发stall，压测LPUSH，pika 20000，目前只有6000，将索引全部放在内存，记WAL做持久化，并定时做Compaction

2017_9_19
WAL 改成异步线程刷文件

### SET

set meta 存放bloom filter，判断元素是否存在于集合
