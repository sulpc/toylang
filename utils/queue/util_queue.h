/**
 * @file util_queue.h
 * @author sulpc
 * @brief
 * @version 0.1
 * @date 2024-05-30
 *
 * @copyright Copyright (c) 2024
 *
 */
#ifndef _UITL_QUEUE_H_
#define _UITL_QUEUE_H_

typedef struct util_queue_node_t util_queue_node_t;
struct util_queue_node_t {
    util_queue_node_t* prev;
    util_queue_node_t* next;
};

// init queue node
#define util_queue_init(node)                                                                                          \
    do {                                                                                                               \
        (node)->prev = (node);                                                                                         \
        (node)->next = (node);                                                                                         \
    } while (0)

// insert new node before node
#define util_queue_insert(node, new_node)                                                                              \
    do {                                                                                                               \
        (new_node)->prev   = (node)->prev;                                                                             \
        (new_node)->next   = (node);                                                                                   \
        (node)->prev->next = (new_node);                                                                               \
        (node)->prev       = (new_node);                                                                               \
    } while (0)

// remove node from queue
#define util_queue_remove(node)                                                                                        \
    do {                                                                                                               \
        (node)->prev->next = (node)->next;                                                                             \
        (node)->next->prev = (node)->prev;                                                                             \
    } while (0)

#define util_queue_empty(node) (((node)->prev == (node)) && ((node)->next == (node)))

#define util_queue_foreach(node_, queue_)                                                                              \
    for (util_queue_node_t* node_ = (queue_)->next; node_ != (queue_); node_ = node_->next)

#endif
