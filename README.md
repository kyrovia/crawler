# C++ 网页爬虫

用 C++17 重写的简单爬虫，演示线程队列与线程池两种并发下载方式。目标站点仍是原来的目录页：<http://my.fit.edu/~vkepuska/Android%20Programming/>。

## 两种实现

`crawler` 对应原来的 `crawler.py`：主线程 DFS 解析目录，8 个工作线程从阻塞队列取任务下载文件。

`crawler_concurrent` 对应原来的 `crawler_concurrent.py`：同样 DFS，但把每个文件提交给线程池（等价于 Python 的 `ThreadPoolExecutor`）。C++ 线程没有 GIL，这两种方式都能真正并行等待网络 I/O。

目录判断规则与原先一致：URL 以 `/` 结尾视为目录，否则视为文件。本地路径从 URL 里的 `Android%20Programming/` 起截取。忽略以 `/~vkepuska` 或 `?` 开头的链接。

## 构建

依赖：C++17 编译器、CMake 3.16+、libcurl。

```bash
cmake -S . -B build
cmake --build build
```

## 运行

```bash
./build/crawler
./build/crawler_concurrent
```

可选参数覆盖起始 URL：

```bash
./build/crawler 'http://my.fit.edu/~vkepuska/Android%20Programming/'
```

文件会下载到当前工作目录下的 `Android%20Programming/`。
