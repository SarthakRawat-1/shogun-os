#include "async_executor.h"
#include "terminal.h"
#include "io.h"
#include "memory.h"
#include "rtc.h"  // Include rtc.h to get access to monotonic_time functions
#include <stdatomic.h>

// Global executor instance and flags
static Executor g_executor;
static atomic_bool g_should_poll = true;
static atomic_uint_fast32_t g_monotonic_ticks = 0;

// Initialize the async executor
void executor_init(Executor* executor) {
    executor->task_queue = NULL;
    executor->task_count = 0;
    atomic_store(&executor->should_poll, true);
    output_string("Async executor initialized\n");
}

// Create a new waker
static Waker* waker_create(void (*wake_func)(Waker*), void* data) {
    Waker* waker = (Waker*)malloc(sizeof(Waker));
    if (waker) {
        waker->wake = wake_func;
        waker->data = data;
        atomic_store(&waker->ref_count, 1);
    }
    return waker;
}

// Wake function for the executor
static void executor_waker_wake(Waker* waker) {
    atomic_store(&g_should_poll, true);
}

// Add a task to the executor queue
void executor_spawn(Executor* executor, Future* future) {
    Task* task = (Task*)malloc(sizeof(Task));
    if (task) {
        task->future = future;
        
        // Create a waker for this task
        task->future->waker = waker_create(executor_waker_wake, NULL);
        
        // Add to the front of the queue
        task->next = executor->task_queue;
        executor->task_queue = task;
        
        atomic_fetch_add(&executor->task_count, 1);
        
        // Wake up the executor to process the new task
        atomic_store(&g_should_poll, true);
        
        output_string("Task spawned\n");
    }
}

// Poll a single task
static bool poll_task(Task* task) {
    if (task->future->is_completed) {
        return true; // Already completed, can be removed
    }
    
    // Call the poll function from the vtable
    FutureState state = task->future->vtable->poll(task->future, NULL);
    
    if (state == FUTURE_READY) {
        task->future->is_completed = true;
        return true; // Completed, can be removed
    }
    
    return false; // Still pending
}

// Run the main execution loop
void executor_run(Executor* executor) {
    output_string("Starting async executor loop\n");

    while (1) {
        bool has_ready_tasks = false;

        // Check if there are any tasks to process
        if (executor->task_queue != NULL) {
            // Poll all tasks
            Task* current = executor->task_queue;
            Task** prev_ptr = &executor->task_queue;

            while (current) {
                bool should_remove = poll_task(current);

                if (should_remove) {
                    // Remove completed task
                    *prev_ptr = current->next;
                    Task* to_remove = current;
                    current = current->next;

                    // Cleanup the future
                    if (to_remove->future->vtable->cleanup) {
                        to_remove->future->vtable->cleanup(to_remove->future);
                    }

                    // Free the waker if it exists
                    if (to_remove->future->waker) {
                        uint32_t ref_count = atomic_fetch_sub(&to_remove->future->waker->ref_count, 1);
                        if (ref_count == 1) {
                            free(to_remove->future->waker);
                        }
                    }

                    free(to_remove->future);
                    free(to_remove);

                    atomic_fetch_sub(&executor->task_count, 1);
                } else {
                    has_ready_tasks = true;
                    prev_ptr = &current->next;
                    current = current->next;
                }
            }
        }

        // If no tasks are ready and should_poll is false, we can halt the CPU efficiently
        bool should_poll_current = atomic_load(&g_should_poll);
        if (!has_ready_tasks && !should_poll_current) {
            // CPU-efficient halt - wait for interrupt
            __asm__ volatile ("sti; hlt; cli");
        } else {
            // Reset the flag after polling, but keep polling if there are tasks ready
            atomic_store(&g_should_poll, false);
        }
    }
}

// Wake up the executor (called from interrupt handlers)
void executor_wake_up(void) {
    atomic_store(&g_should_poll, true);
}

// Time management functions
void monotonic_time_init(void) {
    atomic_store(&g_monotonic_ticks, 0);
    output_string("Monotonic time initialized\n");
}

uint32_t monotonic_time_get_ticks(void) {
    return atomic_load(&g_monotonic_ticks);
}

void monotonic_time_increment(void) {
    atomic_fetch_add(&g_monotonic_ticks, 1);
}

// Sleep future implementation
static FutureState sleep_future_poll(Future* future, void* context) {
    SleepFuture* sleep_future = (SleepFuture*)future;
    uint32_t current_tick = monotonic_time_get_ticks_global();

    if (current_tick >= sleep_future->target_tick) {
        return FUTURE_READY;
    }

    // If this is the first time polling and we're pending, register with wake-up list
    // if not already registered
    if (sleep_future->target_tick > 0) {
        // The wake-up list will call executor_wake_up() when the time is reached
        // which will set the g_should_poll flag and wake the executor loop
    }

    return FUTURE_PENDING;
}

static void sleep_future_cleanup(Future* future) {
    // No special cleanup needed for sleep future
    (void)future;
}

static const FutureVTable sleep_future_vtable = {
    .poll = sleep_future_poll,
    .cleanup = sleep_future_cleanup
};

// Callback function to wake up the executor when sleep is complete
static void sleep_future_callback(void* data) {
    (void)data; // Unused parameter
    executor_wake_up();
}

Future* sleep_future_create(uint32_t ticks) {
    SleepFuture* sleep_future = (SleepFuture*)malloc(sizeof(SleepFuture));
    if (!sleep_future) {
        return NULL;
    }

    sleep_future->base.vtable = &sleep_future_vtable;
    sleep_future->base.is_completed = false;
    sleep_future->base.waker = NULL;
    sleep_future->target_tick = monotonic_time_get_ticks_global() + ticks;

    // Register with wake-up list to wake up the executor when sleep is complete
    wake_up_list_add(sleep_future->target_tick, sleep_future_callback, NULL);

    return (Future*)sleep_future;
}

// Initialize the async executor system
void async_init(void) {
    executor_init(&g_executor);
    monotonic_time_init();

    output_string("Async system initialized\n");
}

// Get the global executor instance
Executor* get_global_executor(void) {
    return &g_executor;
}

// Async serial functionality
static FutureState async_serial_write_poll(Future* future, void* context) {
    AsyncSerialWriteFuture* serial_future = (AsyncSerialWriteFuture*)future;

    // Check if data transmission is ready by checking the serial port status
    if (serial_future->written < serial_future->len) {
        // Check if the transmit register is empty
        if (serial_is_transmit_empty()) {
            // Write the next character
            write_serial(serial_future->data[serial_future->written]);
            serial_future->written++;

            // If there's still more to write, return pending
            if (serial_future->written < serial_future->len) {
                return FUTURE_PENDING;
            }
        } else {
            // The serial port is not ready, return pending and wait for interrupt
            return FUTURE_PENDING;
        }
    }

    // All data has been written
    return FUTURE_READY;
}

static void async_serial_write_cleanup(Future* future) {
    // No special cleanup needed for async serial write future
    (void)future;
}

static const FutureVTable async_serial_write_vtable = {
    .poll = async_serial_write_poll,
    .cleanup = async_serial_write_cleanup
};

Future* async_serial_write_create(const char* data, size_t len) {
    AsyncSerialWriteFuture* serial_future = (AsyncSerialWriteFuture*)malloc(sizeof(AsyncSerialWriteFuture));
    if (!serial_future) {
        return NULL;
    }

    serial_future->base.vtable = &async_serial_write_vtable;
    serial_future->base.is_completed = false;
    serial_future->base.waker = NULL;
    serial_future->data = data;
    serial_future->len = len;
    serial_future->written = 0;

    return (Future*)serial_future;
}

void async_serial_interrupt_handler(void) {
    // Wake up the executor since the serial port may be ready for more data
    executor_wake_up();
}