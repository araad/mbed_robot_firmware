#include "FreeMem.h"
#include "mbed-trace/mbed_trace.h"
#include "mbed.h"

#ifndef TRACE_GROUP
#define TRACE_GROUP "STAT"
#endif

mbed_stats_heap_t heap_stats;
mbed_stats_stack_t stack_info[MBED_CONF_APP_MAX_THREAD_INFO];
mbed_stats_thread_t thread_stats[MBED_CONF_APP_MAX_THREAD_STATS];

void log_mem_stats()
{
    int count = mbed_stats_thread_get_each(thread_stats, MBED_CONF_APP_MAX_THREAD_STATS);
    for (int i = 0; i < count; i++)
    {
        if (thread_stats[i].id == (uint32_t)(ThisThread::get_id()))
        {
            tr_info("--> T: \"%s\"(0x%08x)", thread_stats[i].name, thread_stats[i].id);
            tr_info("--> S:  %lu/%lu", thread_stats[i].stack_space, thread_stats[i].stack_size);
        }
    }

    mbed_stats_heap_get(&heap_stats);
    tr_info("--> H: %lu/%lu", heap_stats.current_size, heap_stats.max_size);

    mbed_stats_stack_get(&stack_info[0]);
    tr_info("--> S(%d): %lu/%lu", stack_info[0].stack_cnt, stack_info[0].max_size, stack_info[0].reserved_size);

    tr_info("--> F: %d", FreeMem());
}
