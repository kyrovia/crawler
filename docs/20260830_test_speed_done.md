# 测试记录 - 单线程 vs 多线程下载总时间 - 2026-08-30

只比端到端墙钟时间。程序：`tests/compare_speed_test.cpp`。  
单线程：`walk` 发现一个文件就立刻下。  
多线程：和 `crawler.cpp` 一样，队列 + N 个工作线程。每轮开始前清空输出目录。

## 1. 测试环境

- 分支：`practice`
- 机器：本机，经清华镜像访问（`ftp.gnu.org` 那次会走限制更多的出口）
- 指标：从进入爬取到全部结束的毫秒数
- 怎么跑：

```bash
cmake --build ~/crawler/build --target compare_speed_test
mkdir -p /tmp/crawler-speed && cd /tmp/crawler-speed
~/crawler/build/compare_speed_test
# 或：ctest --test-dir ~/crawler/build -R CompareSpeed --output-on-failure
# 换地址：CRAWLER_TEST_URL='https://...' ~/crawler/build/compare_speed_test
```

当前默认地址是 GNU ed（约 58 个文件、共约 2MB）：

`https://mirrors.tuna.tsinghua.edu.cn/gnu/ed/`

## 2. 测了什么


| 轮次  | 地址                                               | 文件数 | 测的线程           |
| --- | ------------------------------------------------ | --- | -------------- |
| 1   | `https://ftp.gnu.org/gnu/bool/`                  | 6   | 单线程 vs 8       |
| 2   | `https://mirrors.tuna.tsinghua.edu.cn/gnu/bool/` | 6   | 单线程 vs 8/6/4/2 |
| 3   | `https://mirrors.tuna.tsinghua.edu.cn/gnu/ed/`   | 58  | 单线程 vs 8/6/4/2 |
| 4   | 同上 ed                                            | 58  | 再加上 12、10      |




## 3. 结果



### 3.1 `ftp.gnu.org`：不允许并发

```text
serial:    8817 ms
8 threads: 60843 ms
```

8 线程里多数请求 `Timeout was reached`（curl 超时 60 秒）。并行反而慢很多。

### 3.2 清华 bool：允许并发，但只有 6 个文件

```text
serial:    1746 ms
8 threads:  660 ms
6 threads:  615 ms
4 threads:  923 ms
2 threads: 1213 ms
```

比单线程快，但 6 和 8 几乎一样。

### 3.3 清华 ed：文件更多，差别清楚

```text
serial:     14296 ms
12 threads:  1708 ms
10 threads:  1961 ms
8 threads:   2081 ms
6 threads:   2951 ms
4 threads:   3973 ms
2 threads:   7370 ms
```

相对单线程大约是 8.4x / 7.3x / 6.9x / 4.8x / 3.6x / 1.9x。  
8→10→12 还能再快一点，但每次只少一两百毫秒。

## 4. 遇到的问题

- 开始用的 `ftp.gnu.org`（或前面的代理）不让同时开多条连接。多余线程被挂住，一直等到 60 秒超时，看起来像「并行没用」，其实是对面没接。
- 换成清华 bool 之后并行明显变快，但目录只有 6 个文件，6 线程和 8 线程看不出差别：多出来的人没活干。
- 换成 ed（58 个文件）之后，2～8 基本按线程数变快。再测 10、12，收益已经很小，容易觉得「再加线程看不出来」。
- 单次网络有抖动；上表是代表性的一趟，不是多次取中位数。



## 5. 结论

多线程快，是因为多条下载的等待叠在一起，前提是站点允许并发，并且文件数不少于线程数。

- 站点限连接时，加线程会更慢（`ftp.gnu.org`）。
- 文件太少时，线程超过文件数就不再快（bool 的 6 vs 8）。
- 文件足够、镜像配合时，2～8 提升明显；大约 8～12 进入收益变小，再往上多半是平台期，或再次碰到限流。
- 线程不是越多越好。合适数量大约是「对方愿意同时接的连接数」和「能并行的文件数」里较小的那个。这份作业默认 8 合理。

