#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

typedef struct {
    pthread_mutex_t mutex;  // 保护共享数据的互斥锁
    pthread_cond_t cond;     // 用于线程同步的条件变量
    int count;              // 当前到达障栅的线程数
    int total;              // 需要等待的总线程数
    int generation;         // 代际标识（防止信号错乱）
} barrier_t;

void barrier_init(barrier_t *b, int total) {
    pthread_mutex_init(&b->mutex, NULL);    // 初始化互斥锁
    pthread_cond_init(&b->cond, NULL);      // 初始化条件变量
    b->count = 0;           // 初始化到达线程计数器
    b->total = total;       // 设置需要等待的总线程数
    b->generation = 0;      // 初始代际标识为0
}

void barrier_wait(barrier_t *b) {
    pthread_mutex_lock(&b->mutex);  // 进入临界区
    
    int gen = b->generation;  // 记录当前代际
    b->count++;               // 增加到达线程计数器
    
    /* 当最后一个线程到达时 */
    if (b->count == b->total) {
        b->count = 0;                // 重置计数器
        b->generation++;            // 更新代际标识
        pthread_cond_broadcast(&b->cond); // 唤醒所有等待线程
    } 
    /* 非最后一个线程进入等待 */
    else {
        // 循环检查防止假唤醒（必须满足代际改变的条件）
        while (gen == b->generation) {
            pthread_cond_wait(&b->cond, &b->mutex); // 释放锁并等待
        }
    }
    
    pthread_mutex_unlock(&b->mutex); // 离开临界区
}

#define THREAD_COUNT 4  // 总线程数（必须与障栅初始化参数一致）

void *thread_func(void *arg) {
    barrier_t *barrier = (barrier_t *)arg;
    
    // 第一阶段操作（障栅前）
    printf("Thread %ld: before barrier\n", (long)pthread_self());
    sleep(1);  // 模拟耗时操作
    
    // 等待所有线程到达障栅
    barrier_wait(barrier);
    
    // 第二阶段操作（障栅后）
    printf("Thread %ld: after barrier\n", (long)pthread_self());
    return NULL;
}

int main() {
    barrier_t barrier;
    barrier_init(&barrier, THREAD_COUNT);  // 初始化4线程障栅

    pthread_t threads[THREAD_COUNT];
    // 创建工作线程
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_create(&threads[i], NULL, thread_func, &barrier);
    }

    // 等待所有线程完成
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }

    // 清理资源
    pthread_mutex_destroy(&barrier.mutex);
    pthread_cond_destroy(&barrier.cond);
    
    return 0;
}