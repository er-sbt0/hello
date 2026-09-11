#ifndef CORE_EVENT_H
#define CORE_EVENT_H

#include <stddef.h>

typedef enum EventKind {
	EVENT_STARTUP = 0,
	EVENT_TASK_BEGIN,
	EVENT_TASK_END,
	EVENT_ERROR,
	EVENT_SHUTDOWN,
	EVENT_KIND_COUNT
} EventKind;

typedef struct Event {
	EventKind kind;
	const char *source;
	const char *message;
	int code;
} Event;

typedef struct EventBus EventBus;
typedef void (*EventHandlerFn)(const Event *ev, void *ctx);

EventBus *event_bus_new(void);
void event_bus_free(EventBus *bus);
int event_subscribe(EventBus *bus, EventKind kind, EventHandlerFn fn, void *ctx);
int event_subscribe_all(EventBus *bus, EventHandlerFn fn, void *ctx);
int event_emit(EventBus *bus, const Event *ev);
int event_emitf(EventBus *bus, EventKind kind, const char *source,
		const char *message, int code);
const char *event_kind_name(EventKind kind);
size_t event_subscriber_count(const EventBus *bus, EventKind kind);
unsigned event_emitted_count(const EventBus *bus, EventKind kind);
void event_bus_report(const EventBus *bus);

#endif /* CORE_EVENT_H */
