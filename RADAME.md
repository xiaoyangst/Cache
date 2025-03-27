# 流程梳理



# QPS测试

缓存大小：10000，请求次数：100000

```
ThreadNum: 1, Total Requests: 100000, Time: 3.68221s, QPS: 27157.6
ThreadNum: 3, Total Requests: 100000, Time: 1.3465s, QPS: 74266.7
ThreadNum: 5, Total Requests: 100000, Time: 1.22608s, QPS: 81560.5
ThreadNum: 7, Total Requests: 100000, Time: 1.23152s, QPS: 81200.2
ThreadNum: 8, Total Requests: 100000, Time: 1.26065s, QPS: 79324.2
ThreadNum: 10, Total Requests: 100000, Time: 1.27635s, QPS: 78348.7
```

缓存大小：100000，请求次数：500000

```
ThreadNum: 1, Total Requests: 500000, Time: 18.5089s, QPS: 27014.1
ThreadNum: 3, Total Requests: 500000, Time: 7.21055s, QPS: 69342.8
ThreadNum: 5, Total Requests: 500000, Time: 6.49282s, QPS: 77008.1
ThreadNum: 7, Total Requests: 500000, Time: 6.46909s, QPS: 77290.6
ThreadNum: 8, Total Requests: 500000, Time: 6.52012s, QPS: 76685.7
ThreadNum: 10, Total Requests: 500000, Time: 6.44678s, QPS: 77558.1
```

# 缓存命中率测试

