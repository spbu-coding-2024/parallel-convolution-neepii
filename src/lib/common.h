#ifndef COMMON_COMMON_H
#define COMMON_COMMON_H

#define NUM_OF_TASK_TYPES 3

typedef enum {
  READER,
  COMPUTER,
  WRITER,
} task_type_t;

enum { IDLE, BUSY };

typedef task_type_t work_type_t;

#endif
