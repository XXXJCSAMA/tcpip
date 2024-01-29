/**
 * 信号量的简单测试
 * 创建信号量、等待超时，并使用信号量实现任务间的同步
 */
#if 0

#include "os_api.h"

int task1_flag;
int task2_flag;
int task3_flag;
int task4_flag;

os_eflags_t * task1_eflags;

#define TASK1_INIT_FLAGS		((1 << 3) | (1 << 5))			// 0x28

void task1_entry (void * arg) {
	task1_flag = 0;
	
	// 不满足
	os_err_t err;
	os_flags_t flags = os_eflags_wait(task1_eflags, 1000, OS_EOPT_SET_ANY, (1 << 8), &err);
	if (err < 0) {
		os_dbg("eflags[0]: wait tmo\n");
	}
	os_dbg("eflags[0]: %x\n", os_eflags_flags(task1_eflags, OS_NULL));

	// 满足
	flags = os_eflags_wait(task1_eflags, 1000, OS_EOPT_SET_ANY, (1 << 3), &err);
	if (err < 0) {
		os_dbg("eflags[1]: wait tmo\n");
	}
	os_dbg("eflags[1]: %x\n", os_eflags_flags(task1_eflags, OS_NULL));
	
	// 满足, 虽然多了一位
	flags = os_eflags_wait(task1_eflags, 1000, OS_EOPT_SET_ANY, (1 << 3) | (1 << 8), &err);
	if (err < 0) {
		os_dbg("eflags[1]: wait tmo\n");
	}
	os_dbg("eflags[1]: %x\n", os_eflags_flags(task1_eflags, OS_NULL));

	/// 下面测试消耗
	// 满足，消息1个位
	flags = os_eflags_wait(task1_eflags, 1000, OS_EOPT_SET_ANY | OS_EOPT_EXIT_CLEAR, (1 << 3) | (1 << 8), &err);
	if (err < 0) {
		os_dbg("eflags[4]: wait tmo\n");
	}
	os_dbg("eflags[4]: %x\n", os_eflags_flags(task1_eflags, OS_NULL));

	// 不满足，不消耗
	flags = os_eflags_wait(task1_eflags, 1000, OS_EOPT_SET_ANY | OS_EOPT_EXIT_CLEAR, (1 << 3), &err);
	if (err < 0) {
		os_dbg("eflags[5]: wait tmo, err: -------------\n");
	}
	os_dbg("eflags[5]: %x\n", os_eflags_flags(task1_eflags, OS_NULL));

	// 满足，消耗，全部为0了
	flags = os_eflags_wait(task1_eflags, 1000, OS_EOPT_SET_ANY | OS_EOPT_EXIT_CLEAR, (1 << 5), &err);
	if (err < 0) {
		os_dbg("eflags[5]: wait tmo, err: -------------\n");
	}
	os_dbg("eflags[5]: %x\n", os_eflags_flags(task1_eflags, OS_NULL));

	for (;;) {
		os_task_sleep(10);
		task1_flag ^= 1;
	}
}

void task2_entry (void * arg) {
	task2_flag = 0;
	
	for (;;) {
		os_task_sleep(20);
		task2_flag ^= 1;
	}
}

void task3_entry (void * arg) {
	task3_flag = 0;
	
	for (;;) {
		os_task_sleep(30);
		task3_flag ^= 1;
	}
}

void task4_entry (void * arg) {
	task4_flag = 0;
	
	for (;;) {
		os_task_sleep(40);
		task4_flag ^= 1;
	}
}

int first_flag;
os_eflags_t first_eflags;
void os_app_init (void) {
	first_flag = 0;

	os_eflags_init(&first_eflags, 0);
	task1_eflags = os_eflags_create(TASK1_INIT_FLAGS);

	os_task_t * task1 = os_task_create("task1", task1_entry, OS_NULL, 0, 1024);
	os_task_start(task1);
	os_task_t * task2 = os_task_create("task2", task2_entry, OS_NULL, 0, 1024);
	os_task_start(task2);
	os_task_t * task3 = os_task_create("task3", task3_entry, OS_NULL, 0, 1024);
	os_task_start(task3);
	os_task_t * task4 = os_task_create("task4", task4_entry, OS_NULL, 0, 1024);
	os_task_start(task4);
}

void os_app_loop(void) {
	os_err_t err;
	os_flags_t result = os_eflags_wait(&first_eflags, 60, OS_EOPT_SET_ALL, 0x33, &err);
	os_task_sleep(40);
	first_flag ^= 1;	
}

#endif
