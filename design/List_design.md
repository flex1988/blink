1. pika 的list数据结构 meta保存list的头和尾，遍历操作需要遍历每个元素，操作复杂度为O(n)

2. 将pika的存储改为二级索引，Lindex复杂度为O(logN)，然而每次lpush需要保存第一级索引和第二个索引两个block，写入数据量大，容易引发stall，压测LPUSH，pika 20000，目前只有6000

