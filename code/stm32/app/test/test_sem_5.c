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
os_sem_t * task1_sem;

void task1_entry (void * arg) {
	task1_flag = 0;
	
	// 等待第一个任务发信号量号才进行翻转
	os_sem_wait(task1_sem, 0);
	for (;;) {
		task1_flag ^= 1;
		os_task_sleep(10);
	}
}

void task2_entry (void * arg) {
	task2_flag = 0;
	
	// 等待任务1在信号量上等
	os_task_sleep(100);
	os_sem_free(task1_sem);

	for (;;) {
		// 每20ms翻转一次信号量
		task2_flag ^= 1;
		os_task_sleep(20);
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

int first_flags;
void os_app_init (void) {
	first_flags = 0;

	task1_sem = os_sem_create(0, 10);

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
	first_flags ^= 1;	
	os_task_sleep(100);
}

#endif
