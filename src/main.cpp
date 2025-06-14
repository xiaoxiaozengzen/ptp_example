#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <iostream>
#include <cstring>
#include <cerrno>
#include <chrono>
#include <stdexcept>
#include <cstdint>

/**
 * @brief 动态时钟
 * 
 * 某些硬件提供了计时的能力，可以实现成一个posix clock，
 * 同时，这些硬件又类似USB设备那样可以热拔插，这也就意味着该posix clock不能静态定义。
 * 此外，除了标准的timer和clock相关的操作，这些提供计时能力的硬件还需要一些其他的类似字符设备界面的控制接口。
 * 在这样的需求推动下，内核提供了dynamic posix clock。
 */

/**
 * 例如：PTP（Precision Time Protocol）是一种用于网络同步的协议，它可以提供高精度的时间同步。
 * 在Linux中，PTP通常通过/dev/ptp0设备文件来访问。
 * 
 * 获取时间的基本思路是：
 *  1. 使用open函数打开/dev/ptp0设备文件后，可以获取一个文件描述符。例如：3
 *  2. 使用FD_TO_CLOCKID宏将文件描述符转换为clockid_t类型的时钟ID。例如：-29
 *  3，使用clock_gettime函数获取该时钟ID对应的时间。
 *     a. 这种clockid_t是一个特殊值，内核能识别到是某个fd对应的时钟。
 *     b. 内核会自动还原成对应的fd，因此不可以close该fd，否则会导致后续的clock_gettime调用失败。
 *     c. 内核通过fd访问对应的字符设备，获取时间。
 * 
 */

#define DEV_PATH "/dev/ptp0"
#define FD_TO_CLOCKID(fd) ((clockid_t)((((unsigned int)~fd) << 3) | 3))

#ifndef CLOCK_INVALID
#define CLOCK_INVALID -1
#endif  // CLOCK_INVALID

inline clockid_t GetPtpTime(struct timespec *ts)
{
    static clockid_t clkid = CLOCK_INVALID;
    if (clkid == CLOCK_INVALID) {
        int fd = open(DEV_PATH, O_RDWR);
        printf("open %s, fd = %d\n", DEV_PATH, fd);
        if (fd < 0) {
            std::cerr << "Failed to open " << DEV_PATH << ": " << strerror(errno) << std::endl;
            return CLOCK_INVALID;
        }
        clkid = FD_TO_CLOCKID(fd);
    }

    if (ts != nullptr) {
        memset(ts, 0, sizeof(struct timespec));
        if (clock_gettime(clkid, ts) < 0) {
            std::cerr << "clock_gettime failed: " << strerror(errno) << std::endl;
            return CLOCK_INVALID;
        }
    }

    return clkid;
}

void PrintTime(struct timespec ts) {
    time_t seconds = ts.tv_sec;
    struct tm *tm_info = localtime(&seconds);
    std::cout << tm_info->tm_year + 1900 << "-"
              << tm_info->tm_mon + 1 << "-"
              << tm_info->tm_mday << " "
              << tm_info->tm_hour << ":"
              << tm_info->tm_min << ":"
              << tm_info->tm_sec
              << std::endl;
}

int main() {

    struct timespec ts;
    clockid_t clkid = GetPtpTime(&ts);
    if (clkid == CLOCK_INVALID) {
        std::cerr << "Failed to get clock ID" << std::endl;
        return 1;
    }
    std::cout << "ptp time: " << ts.tv_sec << "s " << ts.tv_nsec << "ns" << std::endl;
    PrintTime(ts);

    struct timespec ts2;
    clock_gettime(CLOCK_REALTIME, &ts2);
    std::cout << "real time: " << ts2.tv_sec << "s " << ts2.tv_nsec << "ns" << std::endl;
    PrintTime(ts2);

    struct timespec ts3;
    std::chrono::nanoseconds duration_ns = std::chrono::system_clock::now().time_since_epoch();
    ts3.tv_sec = std::chrono::duration_cast<std::chrono::seconds>(duration_ns).count();
    ts3.tv_nsec = duration_ns.count() % 1000000000;
    std::cout << "system clock time: " << ts3.tv_sec << "s " << ts3.tv_nsec << "ns" << std::endl;
    PrintTime(ts3);
    

    return 0;
}