﻿/**
 * @file arp.h
 * @author lishutong (527676163@qq.com)
 * @brief ARP协议支持
 * @version 0.1
 * @date 2022-10-29
 * 
 * @copyright Copyright (c) 2022
 * 
 * IPv4协议使用的是IPv4地址，其目标是设计跨越不同类型物理网络的分组交换。
 * 而以太网使用的是mac地址，需要使用正确的mac地址才能保证对方接受到数据包。
 * 因此，使用ARP协议(RFC826)用于在IPv4地址和网络硬件地址进行转换。
 * 
 * ARP协议被设计成兼容多种不同类型的物理网络，但实际总是被用于IPv4和mac地址之间的转换
 * IPv6不使用ARP协议，而使用ICMPv6中的邻居发现协议。
 * 另外还有一种RARP协议，用于缺少磁盘的系统，现在几乎很少使用
 */
#include <string.h>
#include "arp.h"
#include "net_cfg.h"
#include "mblock.h"
#include "dbg.h"
#include "nlocker.h"
#include "protocol.h"
#include "pktbuf.h"
#include "tools.h"

static arp_entry_t cache_tbl[ARP_CACHE_SIZE];    // arp缓存
static mblock_t cache_mblock;                // 空闲arp分配结构
static nlist_t cache_list;                   // 动态表
static const uint8_t empty_hwaddr[] = {0, 0, 0, 0, 0, 0};   // 空闲硬件地址

/**
 * @brief 初始化ARP缓存表及链表
 * ARP缓存用于维护每个接口中得到的IPv4地址到mac地址之间的转换关系
 */
static net_err_t cache_init(void) {
    nlist_init(&cache_list);
    
    // 建立缓存链
    plat_memset(cache_tbl, 0, sizeof(cache_tbl));
    net_err_t err = mblock_init(&cache_mblock, cache_tbl, sizeof(arp_entry_t), ARP_CACHE_SIZE, NLOCKER_NONE);
    if (err < 0) {
        return err;
    }

    return NET_ERR_OK;
}

/**
 * @brief ARP模块初始化
 */
net_err_t arp_init(void) {
    // ARP缓存初始化
    net_err_t err = cache_init();
    if (err < 0) {
        dbg_error(DBG_ARP, "arp cache init failed.");
        return err;
    }

    return NET_ERR_OK;
}


/**
 * @brief 向指定网口上发送ARP查询请求
 */
net_err_t arp_make_request(netif_t* netif, const ipaddr_t* pro_addr) {
    // 分配ARP包的空间
    pktbuf_t* buf = pktbuf_alloc(sizeof(arp_pkt_t));
    if (buf == NULL) {
        dbg_dump_ip(DBG_ARP, "allocate arp packet failed. ip:", pro_addr);
        return NET_ERR_NONE;
    }

    // 这里看起来似乎什么必要，不过还是加上吧
    pktbuf_set_cont(buf, sizeof(arp_pkt_t));

    // 填充ARP包，请求包，写入本地的ip和mac、目标Ip，目标硬件写空值
    arp_pkt_t* arp_packet = (arp_pkt_t*)pktbuf_data(buf);
    arp_packet->htype = x_htons(ARP_HW_ETHER);
    arp_packet->ptype = x_htons(NET_PROTOCOL_IPv4);
    arp_packet->hlen = ETH_HWA_SIZE;
    arp_packet->plen = IPV4_ADDR_SIZE;
    arp_packet->opcode = x_htons(ARP_REQUEST);
    plat_memcpy(arp_packet->send_haddr, netif->hwaddr.addr, ETH_HWA_SIZE);
    ipaddr_to_buf(&netif->ipaddr, arp_packet->send_paddr);
    plat_memcpy(arp_packet->target_haddr, empty_hwaddr, ETH_HWA_SIZE);
    ipaddr_to_buf(pro_addr, arp_packet->target_paddr);

    // 发广播通知所有主机
    net_err_t err = ether_raw_out(netif, NET_PROTOCOL_ARP, ether_broadcast_addr(), buf);
    if (err < 0) {
        pktbuf_free(buf);
    }
    return err;
}