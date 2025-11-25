#ifndef ASYNC_EXECUTOR_H
#define ASYNC_EXECUTOR_H

#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <stddef.h>
#include "idt.h"

typedef struct Future Future;
typedef struct Waker Waker;
typedef struct Executor Executor;

typedef enum {
    FUTURE_READY,
    FUTURE_PENDING
} FutureState;

typedef struct {
    FutureState (*poll)(Future* future, void* context);
    void (*cleanup)(Future* future);
} FutureVTable;

struct Future {
    const FutureVTable* vtable;
    bool is_completed;
    Waker* waker;  
};

struct Waker {
    void (*wake)(Waker* waker);
    atomic_uint_fast32_t ref_count;
    void* data;
};

typedef struct Task {
    Future* future;
    struct Task* next;
} Task;

struct Executor {
    Task* task_queue;
    atomic_bool should_poll;
    atomic_uint_fast32_t task_count;
};

void executor_init(Executor* executor);

void executor_spawn(Executor* executor, Future* future);

void executor_run(Executor* executor);

void executor_wake_up(void);

void monotonic_time_init(void);
uint32_t monotonic_time_get_ticks(void);
void monotonic_time_increment(void);

typedef struct {
    Future base;
    uint32_t target_tick;
} SleepFuture;

Future* sleep_future_create(uint32_t ticks);

void async_init(void);

Executor* get_global_executor(void);

typedef struct {
    Future base;
    const char* data;
    size_t len;
    size_t written;
} AsyncSerialWriteFuture;

Future* async_serial_write_create(const char* data, size_t len);

void async_serial_interrupt_handler(void);

#endif