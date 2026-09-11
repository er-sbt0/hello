#ifndef APP_COMMANDS_H
#define APP_COMMANDS_H

#include "core/scheduler.h"
#include "core/task.h"

int cmd_register_all(Scheduler *s, void *app);

int cmd_greet(Task *self, void *ctx);
int cmd_dump_config(Task *self, void *ctx);
int cmd_word_count(Task *self, void *ctx);
int cmd_fibonacci(Task *self, void *ctx);
int cmd_parity_scan(Task *self, void *ctx);
int cmd_collatz(Task *self, void *ctx);
int cmd_memory_stats(Task *self, void *ctx);
int cmd_event_stats(Task *self, void *ctx);
int cmd_always_fails(Task *self, void *ctx);

long math_fib(int n);
long math_gcd(long a, long b);
int math_is_even(int n);
int math_is_odd(int n);
int math_collatz_steps(long n);

#endif /* APP_COMMANDS_H */
