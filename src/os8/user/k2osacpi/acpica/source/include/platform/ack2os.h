#ifndef __ACK2OS_H__
#define __ACK2OS_H__

#include <kern/k2oskern.h>

#define ACPI_MACHINE_WIDTH      32

#define ACPI_CPU_FLAGS          BOOL

#define ACPI_SPINLOCK           K2OS_CRITSEC *

#define ACPI_MUTEX_TYPE         ACPI_OSL_MUTEX
#define ACPI_MUTEX              K2OS_CRITSEC *

#define ACPI_SEMAPHORE          K2OS_SEMAPHORE_TOKEN

#endif /* __ACK2OS_H__ */
