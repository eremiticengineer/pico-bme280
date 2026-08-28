#include <stdio.h>
#include "pico/stdlib.h"

#include "FreeRTOS.h"
#include "task.h"

/*
 * Each task is assigned a priority from 0 to ( configMAX_PRIORITIES - 1 ),
 * where configMAX_PRIORITIES is defined within FreeRTOSConfig.h.
 * Low priority numbers denote low priority tasks. The idle task has priority zero (tskIDLE_PRIORITY).
 * The task placed into the Running state is always the highest priority task that is able to run.
 */
#define PRINT_TASK_PRIORITY (tskIDLE_PRIORITY + 2UL)

void print_task(__unused void *params) {
    int count = 1;
    while (true) {
        /*
         * Retrieve the minimum amount of remaining stack space that was available
         * to the task since the task started executing. i.e.
         * The amount of stack that remained unused when the task stack was at its greatest (deepest) value.
         * This is what is referred to as the stack 'high water mark'.
         * If the return value is zero then the task has likely overflowed its stack.
         * If the return value is close to zero then the task has come close to overflowing its stack.
         * Stack usage is like a vertical LED audio meter where the highest
         * sound level bar is left lit. The upper space between that lit bar and the
         * maximum sound level is the value uxTaskGetStackHighWaterMark2 reports.
         */
        UBaseType_t remaining = uxTaskGetStackHighWaterMark(nullptr);
        printf("Remaining stack: %u\n", remaining);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main( void )
{
    stdio_init_all();

    xTaskCreate(print_task, "PrintTask", 512, nullptr, PRINT_TASK_PRIORITY, nullptr);

    vTaskStartScheduler();

    return 0;
}
