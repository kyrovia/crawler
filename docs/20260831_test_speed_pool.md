# 测试记录 - 队列 vs 线程池下载总时间 - 2026-08-31

只比端到端墙钟时间。程序：`tests/compare_speed_test.cpp`。  
单线程：`walk` 发现一个文件就立刻下。  
队列：和 `crawler.cpp` 一样，阻塞队列 + N 个工作线程。  
线程池：和 `crawler_concurrent.cpp` 一样，`ThreadPool::submit`；`walk` 结束后靠析构等任务跑完。  
每轮开始前清空输出目录。

## 1. 测试环境

- 分支：`practice`
- 机器：本机，经清华镜像访问
- 指标：从进入爬取到全部结束的毫秒数
- 怎么跑：

```bash
cmake --build ~/crawler/build --target compare_speed_test
mkdir -p /tmp/crawler-speed && cd /tmp/crawler-speed
~/crawler/build/compare_speed_test
# 只看线程池：~/crawler/build/compare_speed_test --gtest_filter='*Pool*'
# 或：ctest --test-dir ~/crawler/build -R CompareSpeed --output-on-failure
# 换地址：CRAWLER_TEST_URL='https://...' ~/crawler/build/compare_speed_test
```

当前默认地址是 GNU ed（约 58 个文件、共约 2MB）：

`https://mirrors.tuna.tsinghua.edu.cn/gnu/ed/`

## 2. 测了什么

| 轮次 | 地址 | 文件数 | 测的实现 |
| --- | --- | --- | --- |
| 1 | 清华 ed | 58 | 单线程；队列 12/10/8/6/4/2；线程池 12/10/8/6/4/2 |

上一份记录（`20260831_test_speed_done.md`）已经比过单线程 vs 队列，以及站点限流、文件太少时加线程没用。这份只补线程池，看它和队列差多少。

## 3. 结果

```text
serial:              18325 ms
queue 12 threads:     2175 ms
queue 10 threads:     2409 ms
queue  8 threads:     2739 ms
queue  6 threads:     3400 ms
queue  4 threads:     4550 ms
queue  2 threads:     9425 ms
pool  12 threads:     2152 ms
pool  10 threads:     2292 ms
pool   8 threads:     2906 ms
pool   6 threads:     3503 ms
pool   4 threads:     4739 ms
pool   2 threads:     9146 ms
```

相对单线程：

| 线程数 | 队列 | 线程池 |
| --- | --- | --- |
| 12 | 8.4x | 8.5x |
| 10 | 7.6x | 8.0x |
| 8 | 6.7x | 6.3x |
| 6 | 5.4x | 5.2x |
| 4 | 4.0x | 3.9x |
| 2 | 1.9x | 2.0x |

同一线程数下，队列和线程池相差一两百毫秒，有时队列快、有时线程池快。相对十秒级的总时间和网络抖动，这点差可以忽略。

两边都是 2～8 提升明显，8→10→12 还能再快一点，但每次只少一两百毫秒。

## 4. 遇到的问题

- 单次网络有抖动；上表是代表性的一趟，不是多次取中位数。同一线程数下谁快谁慢，换一趟可能反过来。
- 线程池没有单独的 `close()`：`walk` 提交完任务后，计时要等到 `ThreadPool` 析构（设停止、把剩下的活干完、join）才停。队列版是显式 `close()` 再 `join`。计时口径都是「全部下完」，所以能比。
- 这次没再测 `ftp.gnu.org` 和 bool。限流、文件太少时加线程没用，上一份已经记过；换实现不会改变这两条。

## 5. 结论

队列和线程池墙钟时间差不多。下载在等网络，不在等任务怎么塞进线程。差的是线程数，不是这两种任务分发写法。

- 同一线程数下，两种实现分不出稳定快慢。
- 线程数的规律和上一份一样：站点允许并发、文件够多时，2～8 提升明显；大约 8～12 进入收益变小。
- 作业默认 8 对两种写法都合理。`crawler.cpp` 和 `crawler_concurrent.cpp` 在这份数据里可以看成同级。
