/**
 * @file test_msgQueue.c
 * @author Denis Fekete (xfeket01@vutbr.cz)
 * @brief 
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef TEST_MESSAGE_QUEUE
#define TEST_MESSAGE_QUEUE

#include "testUtils.h"
#include "../src/libs/msgQueue.h"

int MessageQueue_test1()
{
    TEST_NAME("queueInit()");

    MessageQueue q;

    queueInit(&q);

    ASSERT(q.first == NULL)
    ON_FAIL("Queue.first is not set to NULL")

    ASSERT(q.last == NULL);
    ON_FAIL("Queue.last is not set to NULL")

    ASSERT(q.len == 0);
    ON_FAIL("Queue.len is not set to 0")

    return 0;
}

int MessageQueue_test2()
{
    TEST_NAME("queueAddMessage()");

    MessageQueue q;
    queueInit(&q);

    Buffer buffer;
    bufferInit(&buffer);
    buffer.data = "This is message that should be copied to the buffer";
    buffer.used = 51 + 1;

    queueAddMessage(&q, &buffer);

    ASSERT(q.first != NULL)
    ON_FAIL("Queue.first is NULL")

    ASSERT(q.last != NULL)
    ON_FAIL("Queue.last is NULL")

    ASSERT(q.len == 1)
    ON_FAIL("Queue.len is not 1")

    return 0;
}

int MessageQueue_test3()
{
    TEST_NAME("queueGetMessage()");

    MessageQueue q;
    queueInit(&q);

    return 0;
}

int MessageQueue_testrun()
{
    unsigned failed = 0;

    failed += MessageQueue_test1();
    failed += MessageQueue_test2();

    return failed;
}

#endif /*TEST_MESSAGE_QUEUE*/