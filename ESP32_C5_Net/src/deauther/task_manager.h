#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define LIMIT_RAM_SSH     (1024 * 1024) // 1MB RAM limit for SSH Terminal tasks
#define LIMIT_RAM_MONITOR (256 * 1024)  // 256KB RAM limit for Resource Dashboard tasks

#define MONITOR_HISTORY_MAX 60 // 60 seconds of history

typedef enum {
    TASK_TYPE_SSH = 0,
    TASK_TYPE_MONITOR,
    TASK_TYPE_MAX
} task_type_t;

typedef enum {
    TASK_STATE_STOPPED = 0,
    TASK_STATE_BACKGROUND,
    TASK_STATE_FOREGROUND
} task_state_t;

// Struct to store a single metric data point (CPU, RAM, Disk percentage)
typedef struct {
    uint8_t cpu;
    uint8_t ram;
    uint8_t disk;
} monitor_data_t;

// Struct representing a multitasking task instance
typedef struct task_instance {
    uint32_t task_id;
    task_type_t type;
    task_state_t state;
    
    // Connection parameters
    char host[64];
    char user[32];
    char pass[64];
    int port;
    
    // Low-level socket & SSH descriptors
    int socket_fd;
    void *ssh_session;     // Pointer to LIBSSH2_SESSION
    void *ssh_channel;     // Pointer to LIBSSH2_CHANNEL
    
    // Log/Terminal ring buffer (specifically for SSH tasks)
    uint8_t *ring_buffer;
    uint32_t buffer_size;
    uint32_t head;
    uint32_t tail;
    
    // Datapoint history for resource monitoring (specifically for Monitor tasks)
    monitor_data_t history[MONITOR_HISTORY_MAX];
    uint32_t history_count;
    uint32_t history_index; // Index to write the next second
    
    uint32_t allocated_ram; // Total RAM consumed by this instance
    
    // Doubly-linked list pointers for FIFO scheduling
    struct task_instance *prev;
    struct task_instance *next;
} task_instance_t;

// API functions
void task_manager_init(void);
task_instance_t* task_create(task_type_t type, const char* host, const char* user, const char* pass, int port);
bool task_kill(uint32_t task_id);
bool task_hide(uint32_t task_id);
task_instance_t* task_resume(uint32_t task_id);
void task_get_list(task_type_t type, char* out_buf, size_t max_len);
void monitor_update_tick(void);
uint32_t get_total_allocated_ram(task_type_t type);
void send_monitor_updates_to_hmi(void);
void send_monitor_history_to_hmi(task_instance_t *task);

#endif // TASK_MANAGER_H
