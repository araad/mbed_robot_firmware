#ifndef UROS_TRANSPORTS_H
#define UROS_TRANSPORTS_H

// --- micro-ROS Timing ---
extern "C" int clock_gettime(clockid_t unused, struct timespec *tp)
{
    (void)unused;
    struct timeval tv;
    gettimeofday(&tv, 0);
    tp->tv_sec = tv.tv_sec;
    tp->tv_nsec = tv.tv_usec * 1000;
    return 0;
}

extern "C" bool init_transport();

#endif // UROS_TRANSPORTS_H
