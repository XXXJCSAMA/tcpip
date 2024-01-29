﻿/**
 * @file mblock.c
 * @author lishutong (527676163@qq.com)
 * @brief 存储块管理器
 * 用于将固定大小的存储块组织成块链表，并允许从中申请和释放块
 * @version 0.1
 * @date 2022-10-25
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include "sys_plat.h"
#include "mblock.h"
#include "dbg.h"

/**
 * @brief 初始化存储块管理器
 * 将mem开始的内存区域划分成多个相同大小的内存块，然后用链表链接起来
 */
net_err_t mblock_init (mblock_t* mblock, void * mem, int blk_size, int cnt, nlocker_type_t share_type) {
    // 链表使用了nlist_node结构，所以大小必须合适
    dbg_assert(blk_size >= sizeof(nlist_node_t), "size error");

    // 将缓存区分割成一块块固定大小内存，插入到队列中
    uint8_t* buf = (uint8_t*)mem;
    nlist_init(&mblock->free_list);
    for (int i = 0; i < cnt; i++, buf += blk_size) {
        nlist_node_t* block = (nlist_node_t*)buf;
        nlist_node_init(block);
        nlist_insert_last(&mblock->free_list, block);
    }

    // 初始化锁
    nlocker_init(&mblock->locker, share_type);

    // 创建分配同步用的信号量，由于线程访问处理
    if (share_type != NLOCKER_NONE) {
        mblock->alloc_sem = sys_sem_create(cnt);
        if (mblock->alloc_sem == SYS_SEM_INVALID) {
            dbg_error(DBG_MBLOCK, "create sem failed.");
            nlocker_destroy(&mblock->locker);
            return NET_ERR_SYS;
        }
    }


    return NET_ERR_OK;
}
