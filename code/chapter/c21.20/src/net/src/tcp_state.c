﻿/**
 * @file tcp_state.c
 * @author lishutong
 * @brief TCP输入处理, 对包进行各种检查，并处理状态机相关事项
 * @version 0.1
 * @date 2022-10-22
 *
 * @copyright Copyright (c) 2022
 * 
 * 处理TCP的各种状态转换过程
 */
#include "dbg.h"
#include "mblock.h"
#include "nlist.h"
#include "tcp_out.h"
#include "tcp_state.h"
#include "tools.h"
#include "socket.h"
#include "protocol.h"
#include "tcp_in.h"
#include "tcp_out.h"
#include "tcp.h"

/**
 * @brief 获取TCP状态名称 
 */
const char * tcp_state_name (tcp_state_t state) {
    static const char * state_name[] = {
        [TCP_STATE_FREE] = "FREE",
        [TCP_STATE_CLOSED] = "CLOSED",
        [TCP_STATE_LISTEN] = "LISTEN",
        [TCP_STATE_SYN_SENT] = "SYN_SENT",
        [TCP_STATE_SYN_RECVD] = "SYN_RCVD",
        [TCP_STATE_ESTABLISHED] = "ESTABLISHED",
        [TCP_STATE_FIN_WAIT_1] = "FIN_WAIT_1",
        [TCP_STATE_FIN_WAIT_2] = "FIN_WAIT_2",
        [TCP_STATE_CLOSING] = "CLOSING",
        [TCP_STATE_TIME_WAIT] = "TIME_WAIT",
        [TCP_STATE_CLOSE_WAIT] = "CLOSE_WAIT",
        [TCP_STATE_LAST_ACK] = "LAST_ACK",

        [TCP_STATE_MAX] = "UNKNOWN",
    };

    if (state >= TCP_STATE_MAX) {
        state = TCP_STATE_MAX;
    }
    return state_name[state];
}

/**
 * @brief 设置TCP状态并显示
 */
void tcp_set_state (tcp_t * tcp, tcp_state_t state) {
    tcp->state = state;

    tcp_show_info("tcp set state", tcp);
}

/**
 * @brief 使TCP进入2MS等待状态，即等待一段时间后删除
 * 为防止关闭过程中最后的ACK丢弃，导致另一端发送FIN的对方以为没有收到FIN
 * 因此，需要保留TCP一段时间，从而使得对方能够在ACK丢弃后，重发FIN并收到回应，从而完成关闭过程
 * 不过这里带来的问题就是TCP会保留比较长的时间
 */
void tcp_time_wait (tcp_t * tcp) {
}

/**
 * @brief 关闭状态下时来了一个数据报
 * 当输入的报文没有tcp处理时，或者tcp被关闭后，或者刚刚创建时
 * 此时，任何输入的报文都不应当被处理，直接给对方发送rst报文强制其复位
 */
net_err_t tcp_closed_in(tcp_t *tcp, tcp_seg_t *seg) {
    return NET_ERR_OK;
}

/**
 * @brief SYN已经发送时，收到的包处理
 * 正常的包：SYN+ACK包(自己先打开)，或者只收到SYN包（对方同时打开）
 * 其它包：丢弃或者发送RST后丢弃
 * 在该连接阶段，不支持附带发送任何数据，也不接收任何数据
 *
 * 触发方式：由用户调用connect后进入该状态
 */
net_err_t tcp_syn_sent_in(tcp_t *tcp, tcp_seg_t *seg) {
    tcp_hdr_t *tcp_hdr = seg->hdr;

    // 与其它状态不同，这里没有办法先检查序号，因为不知道对方的序号正确范围（对方没给自己发过包）

    // 第一步：检查ACK位，如果设置了的话, 通过ACK来判断该包是否应该被自己接收处理
    // 此时不能检查序号，因为之前没有收到对方的报文，即不知道此次接收的报文序号应该是多少
    // 当然，这个过程，可能收到ACK(如错误的包、RST包）)；也可能没收到ACK（如同时打开收到对方的SYN报文）
    // 因此，这里的ACK检查是在ACK存在的情况下进行检查
    if (tcp_hdr->f_ack) {
        // 下面这里有点复杂，其实只需要判断：seg->ack != tsk->snd_nxt即可
        // 因为初始SYN不带数据(普通做法)，即snd_nxt = iss+1，对方发来的ACK肯定得是这一个
        if ((tcp_hdr->ack - tcp->snd.iss <= 0) || (tcp_hdr->ack - tcp->snd.nxt > 0)) {
            dbg_warning(DBG_TCP, "%s: ack incorrect", tcp_state_name(tcp->state));
            return tcp_send_reset(seg);
        }
    }

    // 第二步：检查RST位, RST包的ACK必须置位且通过检查，不能随便什么复位报文都响应处理
    // 如果检查通过，意味着去连续对方时，对方不接受该连接。所以我们要结束这个连接并告诉应用程序
    if (tcp_hdr->f_rst) {
        if (!tcp_hdr->f_ack) {
            return NET_ERR_OK;
        }

        // 中止连接，所有任务被唤醒并收到RESET错误
        dbg_warning(DBG_TCP, "%s: recieve a rst", tcp_state_name(tcp->state));
        return tcp_abort(tcp, NET_ERR_RESET);
    }

    // 第三步， 检查SYN位（此时的包是非RST包，如果有ACK且通过了ACK检查）的包
    // 此时的报文可能有两种：SYN（同时打开对方发来的）或者SYN+ACK（对我们发送的SYN报文的回应）
    if (tcp_hdr->f_syn) {
        // 不管是是SYN还是SYN+ACK，包里都有对方的起始序列号
        tcp->rcv.iss = tcp_hdr->seq;            // syn包含了接收初始序号
        tcp->rcv.nxt = tcp_hdr->seq + 1;        // 收到了syn算1个序号
        tcp->flags.irs_valid = 1;               // 已经收到了对方的初始序号, 不是首次通信了

        // 对方同时给了ACK确认，则确认之前发送的SYN
        if (tcp_hdr->f_ack) {
            tcp_ack_process(tcp, seg);
        }

        // una > iss，前面已经确认收到了ack，即syn已经被确认对方收到
        // 为什么一定收到了ACK？因为如果没收到的话，una==iss
        if (tcp->snd.una - tcp->snd.iss > 0) {  // 可以判断ack=1
            // 对对方发来的SYN发送ACK响应
            tcp_send_ack(tcp, seg);

            // 进入已经连接的状态，通知执行连接的程序TCP已经完成连接
            tcp_set_state(tcp, TCP_STATE_ESTABLISHED);
            sock_wakeup(&tcp->base, SOCK_WAIT_CONN, NET_ERR_OK);
        } else {
            // una <= iss，双方同时打开，要求双方互相知道对方的端口情况，且同时去连。
            // 可以在以后测试绑定本地某个端口，然后再connect远端固定端口，在发送syn包时，先断点停下来
            // 待远端使用同样的端口配置时，先连接，然后我们再全速跑。TODO: 以后测试
            tcp_set_state(tcp, TCP_STATE_SYN_RECVD);

            // 收到的是SYN包，即同时打开，发送SYN+ACK通知对方. 之后等待对方发回ACK
            tcp_send_syn(tcp);      // 内部会自动调整发送SYN包
        }
    }

    // 其它类型的包，忽略掉
    return NET_ERR_OK;
}

/**
 * @brief 已经建立连接时的状态处理
 * 这个状态下，双方已经建立连接，可以互相发送数据
 */
net_err_t tcp_established_in(tcp_t *tcp, tcp_seg_t *seg) {
    tcp_hdr_t *tcp_hdr = seg->hdr;

    // 检查RST位
    if (tcp_hdr->f_rst) {
        dbg_warning(DBG_TCP, "%s: recieve a rst", tcp_state_name(tcp->state));
        return tcp_abort(tcp, NET_ERR_RESET);
    }

    // 检查SYN位。如果发现以同样的端口重新连接，可能是对方自行断开重连；此时状态出错复位
    if (tcp_hdr->f_syn) {
        dbg_warning(DBG_TCP, "%s: recieve a syn", tcp_state_name(tcp->state));
        tcp_send_reset(seg);
        return tcp_abort(tcp, NET_ERR_RESET);
    }

    // 处理ACK，对已发未确认的数据进行确认
    if (tcp_ack_process(tcp, seg) < 0) {
        dbg_warning(DBG_TCP, "%s:  ack process failed", tcp_state_name(tcp->state));
        return NET_ERR_UNREACH;
    }

    // 处理收到的数据, 其中包括FIN标志位等，这个过程可能会发送ACK
    tcp_data_in(tcp, seg);

    // 不能在这里直接检查FIN，因为TCP的数据可能是无序、丢包，可能前面的数据还没收到
    // 就收到了后面的FIN，因此要等到前面所有的TCP数据全部收到时，才进入CLOSE_WAIT状态
    if (tcp_hdr->f_fin) {
        tcp_set_state(tcp, TCP_STATE_CLOSE_WAIT);
    }

    return NET_ERR_OK;
}

/**
 * @brief 被动关闭时状态处理
 * 此时，连接已经被远程主动发起关闭，即远端已经发送过FIN通知我们他的数据发完了
 * 这个时候我们可以继续发送数据给对方，但是不需要再接收数据了
 */
net_err_t tcp_close_wait_in (tcp_t * tcp, tcp_seg_t * seg) {
    tcp_hdr_t *tcp_hdr = seg->hdr;

    // 检查RST位
    if (tcp_hdr->f_rst) {
        dbg_warning(DBG_TCP, "%s: recieve a rst", tcp_state_name(tcp->state));
        return tcp_abort(tcp, NET_ERR_RESET);
    }

    // 检查SYN位。如果发现以同样的端口重新连接，可能是对方自行断开重连；此时状态出错复位
    if (tcp_hdr->f_syn) {
        dbg_warning(DBG_TCP, "%s: recieve a syn", tcp_state_name(tcp->state));
        tcp_send_reset(seg);
        return tcp_abort(tcp, NET_ERR_RESET);
    }

    // 处理ACK，对已发未确认的数据进行确认
    if (tcp_ack_process(tcp, seg) < 0) {
        dbg_warning(DBG_TCP, "%s:  dump ack %d ?", tcp_state_name(tcp->state), seg->hdr->ack);
        return NET_ERR_UNREACH;
    }

    // 此状态下是被动关闭，仍然可以发送数据并更新窗口
    tcp_transmit(tcp);

    // 不检查FIN，维持在该状态
    return NET_ERR_OK;
}

/**
 * @brief last ack状态下的输入处理
 * 此时，对方先发起了关闭；我们确认后也发送了FIN。该状态下，当对方的ACK到来时，
 * 即确认完成了整个连接的关闭。在这个过程中不再接收任何数据
 */
net_err_t tcp_last_ack_in (tcp_t * tcp, tcp_seg_t * seg) {
    tcp_hdr_t *tcp_hdr = seg->hdr;

    // 检查RST位
    if (tcp_hdr->f_rst) {
        dbg_warning(DBG_TCP, "%s: recieve a rst", tcp_state_name(tcp->state));
        return tcp_abort(tcp, NET_ERR_RESET);
    }

    // 检查SYN位。如果发现以同样的端口重新连接，可能是对方自行断开重连；此时状态出错复位
    if (tcp_hdr->f_syn) {
        dbg_warning(DBG_TCP, "%s: recieve a syn", tcp_state_name(tcp->state));
        tcp_send_reset(seg);
        return tcp_abort(tcp, NET_ERR_RESET);
    }

    // 对收到的ACK进行处理
    if (tcp_ack_process(tcp, seg) < 0) {
        dbg_warning(DBG_TCP, "%s:  ack process failed", tcp_state_name(tcp->state));
        return NET_ERR_UNREACH;
    }

    // 当前的ack可能是对已发数据的ACK，并不一定是对FIN的
    // 只有当所有的数据都发送完，包括FIN也发送完之后，终止连接，由应用上的close去释放
    // 暂时直接处理，不考虑丢包重传等问题
    return tcp_abort(tcp, NET_ERR_CLOSE);
}

/**
 * @brief TCP FIN-wait 1状态处理
 * 在该状态下，对方未关闭，但是我们自己先主动关闭，表明自己不需要再发送数据。但是仍然可以接收数据
 */
net_err_t tcp_fin_wait_1_in(tcp_t * tcp, tcp_seg_t * seg) {
    tcp_hdr_t *tcp_hdr = seg->hdr;

    // 检查RST位
    if (tcp_hdr->f_rst) {
        dbg_warning(DBG_TCP, "%s: recieve a rst", tcp_state_name(tcp->state));
        return tcp_abort(tcp, NET_ERR_RESET);
    }

    // 检查SYN位。如果发现以同样的端口重新连接，可能是对方自行断开重连；此时状态出错复位
    if (tcp_hdr->f_syn) {
        dbg_warning(DBG_TCP, "%s: recieve a syn", tcp_state_name(tcp->state));
        tcp_send_reset(seg);
        return tcp_abort(tcp, NET_ERR_RESET);
    }

    // 对收到的ACK进行处理
    if (tcp_ack_process(tcp, seg) < 0) {
        dbg_warning(DBG_TCP, "%s:  ack process failed", tcp_state_name(tcp->state));
        return NET_ERR_UNREACH;
    }

    // 处理收到的数据, 其中包括FIN标志位等，这个过程可能会发送ACK
    tcp_data_in(tcp, seg);

    // 虽然已经主动关闭，但是可能缓存中仍然有数据没有发完，所以此状态下可以继续发送
    tcp_transmit(tcp);

    // 处理ACK，对已发未确认的数据进行确认.这其中可能有数据需要发送，此时将会发送一个数据包
    // 即在FIN-WAIT-1仍然是有数据可以发送的，只不过不再允许应用程序写发送缓存
    // 所有序列已经被发送并确认, 即发出去的FIN已经被确认
    if (tcp_hdr->f_fin) {
        // 同时收到了对方的FIN，即对方也想要关闭，进入TIME-WAIT状态
        tcp_time_wait(tcp);
    } else {
        // 没有FIN，只有对我们的FIN的响应，进入FIN-WAIT-2状态
        tcp_set_state(tcp, TCP_STATE_FIN_WAIT_2);

        // TODO：如果想支持半关闭，可以这么干
        //sock_wakeup(tcp, SOCK_WAIT_CONN, NET_ERR_OK);
    }

    return NET_ERR_OK;
}

/**
 * @brief TCP FIN-wait 2状态处理
 * 在该状态下，本机已经关闭了发送；但还可以接收远端的数据
 */
net_err_t tcp_fin_wait_2_in(tcp_t * tcp, tcp_seg_t * seg) {
    tcp_hdr_t *tcp_hdr = seg->hdr;

    // 检查RST位
    if (tcp_hdr->f_rst) {
        dbg_warning(DBG_TCP, "%s: recieve a rst", tcp_state_name(tcp->state));
        return tcp_abort(tcp, NET_ERR_RESET);
    }

    // 检查SYN位。如果发现以同样的端口重新连接，可能是对方自行断开重连；此时状态出错复位
    if (tcp_hdr->f_syn) {
        dbg_warning(DBG_TCP, "%s: recieve a syn", tcp_state_name(tcp->state));
        tcp_send_reset(seg);
        return tcp_abort(tcp, NET_ERR_RESET);
    }

    // ACK处理
    if (tcp_ack_process(tcp, seg) < 0) {
         dbg_warning(DBG_TCP, "%s:  ack process failed", tcp_state_name(tcp->state));
       return NET_ERR_UNREACH;
    }

    // 处理数据和FIN等
    tcp_data_in(tcp, seg);

    // 尝试发送数据并更新窗口
    tcp_transmit(tcp);

    // 当前的ack可能是对已发数据的ACK，并不一定是对FIN的
    // 因为进入该状态时，是用户主动申请发送FIN就立即进入，此时FIN可能还在发送缓存中（之前有数据要发）
    // 因此，此时即便是收到FIN，也不能立即进入TIME-WAIT，因为可能是之前数据的ACK
    // 此外，即便我们发的FIN已经被确认，也不能不检查当前报文的FIN标志来决定是否立即切换
    // 因为有可能报文乱序，前面还有数据未到达，我们要等前面的数据到达后处理，最后处理到FIN时再切换
    // 特别是对于只是自己关闭了发送的情况（目前未支持），应用还是可以继续接收处理的。
    if (tcp_hdr->f_fin) {
        tcp_time_wait(tcp);
    }

    return NET_ERR_OK;
}

/**
 * @brief TCP closing状态下的处理
 * 在此状态下，已经收到了对方关闭的请求并做了响应，等待对方对自己的关闭请求进行响应
 * 此时不接受任何数据处理，收到ACK后，立即进入time-wait状态
 */
net_err_t tcp_closing_in (tcp_t * tcp, tcp_seg_t * seg) {
    return NET_ERR_OK;
}

/**
 * @brief 进入time wait时的状态处理
 * 在该状态下，连接已经被关闭。tcp只是用来重传ack使用，即对对方的FIN进行回应
 */
net_err_t tcp_time_wait_in (tcp_t * tcp, tcp_seg_t * seg) {
    return NET_ERR_OK;
}
