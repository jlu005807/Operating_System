#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>

#define BUFFER_SIZE 5        // 缓冲区容量
#define PRODUCERS 2          // 生产者线程数量
#define CONSUMERS 2          // 消费者线程数量
#define ITEMS_PER_PRODUCER 10 // 每个生产者生产的项目数
#define ITEMS_PER_CONSUMER 10 // 每个消费者消费的项目数

bool buffer[BUFFER_SIZE];     // 共享缓冲区(环形队列结构)
int in = 0;                  // 生产者写入位置索引(下一个空槽位置)
int out = 0;                 // 消费者读取位置索引(下一个数据位置)

sem_t empty;                 // 空槽信号量(初始值为BUFFER_SIZE)
sem_t full;                  // 满槽信号量(初始值为0)
pthread_mutex_t mutex;       // 互斥锁，保护缓冲区的操作

void *producer(void *arg) {
    for (int i = 0; i < ITEMS_PER_PRODUCER; i++) {

        sem_wait(&empty);    // 等待空槽：empty信号量-1（若empty=0则阻塞）
        pthread_mutex_lock(&mutex); // 加锁，保护缓冲区操作
        
        /* 临界区开始 */
        buffer[in] = true;   // 数据放入缓冲区
        printf("Producer %ld produced at position %d\n", 
              (long)pthread_self(), in); // 打印生产信息
        in = (in + 1) % BUFFER_SIZE; // 环形队列索引更新
        /* 临界区结束 */
        
        pthread_mutex_unlock(&mutex); // 解锁
        sem_post(&full);     // 增加满槽：full信号量+1
    }
    return NULL;
}

void *consumer(void *arg) {
    for (int i = 0; i < ITEMS_PER_CONSUMER; i++) {
        sem_wait(&full);     // 等待满槽：full信号量-1（若full=0则阻塞）
        pthread_mutex_lock(&mutex); // 加锁
        
        /* 临界区开始 */
        buffer[in] = false;
        printf("Consumer %ld consumed from position %d\n",
              (long)pthread_self(), out); // 打印消费信息
        out = (out + 1) % BUFFER_SIZE; // 更新索引
        /* 临界区结束 */
        
        pthread_mutex_unlock(&mutex); // 解锁
        sem_post(&empty);    // 增加空槽：empty信号量+1
    }
    return NULL;
}

int main() {
    pthread_t producers[PRODUCERS], consumers[CONSUMERS];
    
    // 初始化信号量
    sem_init(&empty, 0, BUFFER_SIZE);  // 空槽初始数量=缓冲区大小
    sem_init(&full, 0, 0);             // 满槽初始数量=0
    pthread_mutex_init(&mutex, NULL);  // 初始化互斥锁

    // 创建生产者线程
    for (int i = 0; i < PRODUCERS; i++) {
        pthread_create(&producers[i], NULL, producer, NULL);
    }
    // 创建消费者线程
    for (int i = 0; i < CONSUMERS; i++) {
        pthread_create(&consumers[i], NULL, consumer, NULL);
    }

    // 等待所有生产者完成
    for (int i = 0; i < PRODUCERS; i++) {
        pthread_join(producers[i], NULL);
    }
    // 等待所有消费者完成
    for (int i = 0; i < CONSUMERS; i++) {
        pthread_join(consumers[i], NULL);
    }

    // 销毁同步对象
    sem_destroy(&empty);
    sem_destroy(&full);
    pthread_mutex_destroy(&mutex);
    
    return 0;
}