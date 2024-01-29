﻿/**
 * @brief TCP/IP核心线程通信模块
 * 此处运行了一个核心线程，所有TCP/IP中相关的事件都交由该线程处理
 * @author lishutong (527676163@qq.com)
 * @version 0.1
 * @date 2022-08-19
 * @copyright Copyright (c) 2022
 * @note
 */
#ifndef EXMSG_H
#define EXMSG_H

#include "net_err.h"

net_err_t exmsg_init(void);
net_err_t exmsg_start(void);

#endif // EXMSG_H