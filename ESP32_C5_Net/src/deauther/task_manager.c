#include "task_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "TASK_MGR";

// Head and tail pointers for the global doubly-linked list of tasks
static task_instance_t *g_head = NULL;
static task_instance_t *g_tail = NULL;
static uint32_t g_next_task_id = 1;

void task_manager_init(void) {
    g_head = NULL;
    g_tail = NULL;
    g_next_task_id = 1;
    ESP_LOGI(TAG, "Task Manager initialized.");
}

uint32_t get_total_allocated_ram(task_type_t type) {
    uint32_t total = 0;
    task_instance_t *curr = g_head;
    while (curr != NULL) {
        if (curr->type == type) {
            total += curr->allocated_ram;
        }
        curr = curr->next;
    }
    return total;
}

// Check RAM usage and close oldest tasks if threshold is exceeded (FIFO Eviction)
static void check_and_evict_fifo(task_type_t type, uint32_t needed_ram) {
    uint32_t limit = (type == TASK_TYPE_SSH) ? LIMIT_RAM_SSH : LIMIT_RAM_MONITOR;
    
    while (get_total_allocated_ram(type) + needed_ram > limit) {
        // Find the oldest task of this type
        task_instance_t *oldest = g_head;
        while (oldest != NULL) {
            if (oldest->type == type) {
                break;
            }
            oldest = oldest->next;
        }
        
        if (oldest == NULL) {
            break; // No more tasks to evict
        }
        
        ESP_LOGW(TAG, "Memory limit exceeded for type %d. Evicting oldest task ID %lu", type, oldest->task_id);
        task_kill(oldest->task_id);
    }
}

task_instance_t* task_create(task_type_t type, const char* host, const char* user, const char* pass, int port) {
    uint32_t ring_buf_size = (type == TASK_TYPE_SSH) ? (60 * 1024) : 0; // 60KB for SSH
    uint32_t needed_ram = sizeof(task_instance_t) + ring_buf_size;
    
    // Evict if needed before allocating memory
    check_and_evict_fifo(type, needed_ram);
    
    // Allocate the task instance in PSRAM
    task_instance_t *task = heap_caps_malloc(sizeof(task_instance_t), MALLOC_CAP_SPIRAM);
    if (task == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory in PSRAM for task instance.");
        return NULL;
    }
    
    memset(task, 0, sizeof(task_instance_t));
    task->task_id = g_next_task_id++;
    task->type = type;
    task->state = TASK_STATE_BACKGROUND; // Created running in background by default
    task->allocated_ram = needed_ram;
    
    strncpy(task->host, host, sizeof(task->host) - 1);
    strncpy(task->user, user, sizeof(task->user) - 1);
    strncpy(task->pass, pass, sizeof(task->pass) - 1);
    task->port = port;
    
    if (type == TASK_TYPE_SSH) {
        // Allocate terminal ring buffer in PSRAM
        task->ring_buffer = heap_caps_malloc(ring_buf_size, MALLOC_CAP_SPIRAM);
        if (task->ring_buffer == NULL) {
            ESP_LOGE(TAG, "Failed to allocate ring buffer in PSRAM.");
            heap_caps_free(task);
            return NULL;
        }
        task->buffer_size = ring_buf_size;
        task->head = 0;
        task->tail = 0;
    } else if (type == TASK_TYPE_MONITOR) {
        task->history_count = 0;
        task->history_index = 0;
    }
    
    // Append to linked list
    if (g_tail == NULL) {
        g_head = task;
        g_tail = task;
    } else {
        g_tail->next = task;
        task->prev = g_tail;
        g_tail = task;
    }
    
    ESP_LOGI(TAG, "Created task ID %lu (Type: %d, Host: %s), allocated %lu bytes", 
             task->task_id, type, host, needed_ram);
    return task;
}

bool task_kill(uint32_t task_id) {
    task_instance_t *curr = g_head;
    while (curr != NULL) {
        if (curr->task_id == task_id) {
            // Remove from list
            if (curr->prev != NULL) {
                curr->prev->next = curr->next;
            } else {
                g_head = curr->next;
            }
            
            if (curr->next != NULL) {
                curr->next->prev = curr->prev;
            } else {
                g_tail = curr->prev;
            }
            
            // Clean up network and library structures if any
            if (curr->socket_fd > 0) {
                // close(curr->socket_fd);
            }
            
            // Free allocated memories
            if (curr->ring_buffer != NULL) {
                heap_caps_free(curr->ring_buffer);
            }
            
            uint32_t freed_ram = curr->allocated_ram;
            heap_caps_free(curr);
            
            ESP_LOGI(TAG, "Killed task ID %lu, freed %lu bytes", task_id, freed_ram);
            return true;
        }
        curr = curr->next;
    }
    return false;
}

bool task_hide(uint32_t task_id) {
    task_instance_t *curr = g_head;
    while (curr != NULL) {
        if (curr->task_id == task_id) {
            curr->state = TASK_STATE_BACKGROUND;
            ESP_LOGI(TAG, "Hidden task ID %lu", task_id);
            return true;
        }
        curr = curr->next;
    }
    return false;
}

task_instance_t* task_resume(uint32_t task_id) {
    task_instance_t *target = NULL;
    task_instance_t *curr = g_head;
    
    // Find target
    while (curr != NULL) {
        if (curr->task_id == task_id) {
            target = curr;
            break;
        }
        curr = curr->next;
    }
    
    if (target != NULL) {
        // Switch all other tasks of same type to background
        curr = g_head;
        while (curr != NULL) {
            if (curr->type == target->type && curr->task_id != task_id) {
                if (curr->state == TASK_STATE_FOREGROUND) {
                    curr->state = TASK_STATE_BACKGROUND;
                }
            }
            curr = curr->next;
        }
        target->state = TASK_STATE_FOREGROUND;
        ESP_LOGI(TAG, "Resumed task ID %lu to foreground", task_id);
    }
    return target;
}

void task_get_list(task_type_t type, char* out_buf, size_t max_len) {
    snprintf(out_buf, max_len, "TASK_LIST:%d:", type);
    size_t len = strlen(out_buf);
    
    task_instance_t *curr = g_head;
    bool first = true;
    while (curr != NULL) {
        if (curr->type == type) {
            char item[128];
            snprintf(item, sizeof(item), "%s%lu,%s,%d", 
                     first ? "" : ";", 
                     curr->task_id, 
                     curr->host, 
                     curr->state);
            first = false;
            
            if (len + strlen(item) < max_len - 1) {
                strcat(out_buf, item);
                len += strlen(item);
            } else {
                break; // Buffer is full
            }
        }
        curr = curr->next;
    }
}

void monitor_update_tick(void) {
    task_instance_t *curr = g_head;
    while (curr != NULL) {
        if (curr->type == TASK_TYPE_MONITOR && curr->state != TASK_STATE_STOPPED) {
            // Read next history slot
            uint32_t idx = curr->history_index;
            
            // In real device, execute a command on SSH channel: "cat /proc/loadavg" etc.
            // For simulation/mock, we generate realistic load values:
            uint8_t simulated_cpu = 10 + (rand() % 45); // 10% - 55%
            uint8_t simulated_ram = 40 + (rand() % 25); // 40% - 65%
            uint8_t simulated_disk = 12;                // 12%
            
            curr->history[idx].cpu = simulated_cpu;
            curr->history[idx].ram = simulated_ram;
            curr->history[idx].disk = simulated_disk;
            
            curr->history_index = (idx + 1) % MONITOR_HISTORY_MAX;
            if (curr->history_count < MONITOR_HISTORY_MAX) {
                curr->history_count++;
            }
        }
        curr = curr->next;
    }
}

extern void send_to_rp2350(const char *msg);

void send_monitor_updates_to_hmi(void) {
    task_instance_t *curr = g_head;
    while (curr != NULL) {
        if (curr->type == TASK_TYPE_MONITOR && curr->state == TASK_STATE_FOREGROUND) {
            // Send the latest data point
            uint32_t latest_idx = (curr->history_index == 0) ? (MONITOR_HISTORY_MAX - 1) : (curr->history_index - 1);
            char buf[128];
            snprintf(buf, sizeof(buf), "MON_STAT:%lu:%u,%u,%u", 
                     curr->task_id, 
                     curr->history[latest_idx].cpu, 
                     curr->history[latest_idx].ram, 
                     curr->history[latest_idx].disk);
            send_to_rp2350(buf);
        }
        curr = curr->next;
    }
}

void send_monitor_history_to_hmi(task_instance_t *task) {
    if (task->history_count == 0) return;
    
    // Send in chunks of 10 points
    uint32_t count = task->history_count;
    uint32_t start_idx = (task->history_index + MONITOR_HISTORY_MAX - count) % MONITOR_HISTORY_MAX;
    
    for (uint32_t i = 0; i < count; i += 10) {
        uint32_t chunk_size = (count - i > 10) ? 10 : (count - i);
        char buf[128];
        snprintf(buf, sizeof(buf), "MON_HIST:%lu:%lu:%lu:", task->task_id, i, chunk_size);
        
        for (uint32_t j = 0; j < chunk_size; j++) {
            uint32_t idx = (start_idx + i + j) % MONITOR_HISTORY_MAX;
            char pt[16];
            snprintf(pt, sizeof(pt), "%u,%u,%u;", 
                     task->history[idx].cpu, 
                     task->history[idx].ram, 
                     task->history[idx].disk);
            strcat(buf, pt);
        }
        send_to_rp2350(buf);
        // Small delay to prevent queue congestion
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
