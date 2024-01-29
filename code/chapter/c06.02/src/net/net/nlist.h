/**
 * @file clist.h
 * @author lishutong (527676163@qq.com)
 * @brief 通用链表结构
 * 整个网络协议栈使用最多的基本数据结构。
 * 对于任何要使用链表的地方，只需要在相应的结构体中使用nlist_node_t
 * 然后即可将其作为链表结点即可
 * @version 0.1
 * @date 2022-10-25
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#ifndef NLIST_H
#define NLIST_H

/**
 * @brief 链表结点类型
 * 采用的是双向链表，方便结点的删除。为节省内存资源，可考虑简化为单向链表
 */
typedef struct _nlist_node_t {
    struct _nlist_node_t* next;         // 前驱结点
    struct _nlist_node_t* pre;          // 后继结点
}nlist_node_t;

/**
 * @brief 头结点的初始化
 */
static inline void nlist_node_init(nlist_node_t *node) {
    node->pre = node->next = (nlist_node_t *)0;
}

/**
 * @brief 获取结点的后继结点
 */
static inline nlist_node_t * nlist_node_next(nlist_node_t* node) {
    return node->next;
}

/**
 * @brief 获取结点的前一结点
 */
static inline nlist_node_t * nlist_node_pre(nlist_node_t *node) {
    return node->pre;
}

/**
 * @brief 设置结点的后一结点 
 */
static inline void nlist_node_set_next(nlist_node_t* pre, nlist_node_t* next) {
    pre->next = next;
}

/**
 * @brief 通用链表结构
 */
typedef struct _nlist_t {
    nlist_node_t * first;            // 头结点
    nlist_node_t * last;             // 尾结点
    int count;                      // 结点数量
}nlist_t;

void nlist_init(nlist_t *list);

/**
 * 判断链表是否为空
 * @param list 判断的链表
 * @return 1 - 空，0 - 非空
 */
static inline int nlist_is_empty(nlist_t *list) {
    return list->count == 0;
}

/**
 * 获取链表的结点数量
 * @param list 查询的链表
 * @return 结果的数据
 */
static inline int nlist_count(nlist_t *list) {
    return list->count;
}

/**
 * 获取指定链表的第一个表项
 * @param list 查询的链表
 * @return 第一个表项
 */
static inline nlist_node_t* nlist_first(nlist_t *list) {
    return list->first;
}

/**
 * 获取指定链接的最后一个表项
 * @param list 查询的链表
 * @return 最后一个表项
 */
static inline nlist_node_t* nlist_last(nlist_t *list) {
    return list->last;
}

#endif /* NLIST_H */
