#include "core/event.h"

#include "ds/vec.h"
#include "util/log.h"
#include "util/mem.h"

typedef struct Subscriber {
	EventHandlerFn fn;
	void *ctx;
	unsigned invocations;
} Subscriber;

struct EventBus {
	Vec *subscribers[EVENT_KIND_COUNT];
	unsigned emitted[EVENT_KIND_COUNT];
};

static const char *const g_event_names[EVENT_KIND_COUNT] = {
	"startup", "task-begin", "task-end", "error", "shutdown"
};

static int event_kind_valid(EventKind kind)
{
	return kind >= 0 && kind < EVENT_KIND_COUNT;
}

static Subscriber *subscriber_new(EventHandlerFn fn, void *ctx)
{
	Subscriber *sub = mem_calloc(1, sizeof(Subscriber));
	sub->fn = fn;
	sub->ctx = ctx;
	return sub;
}

static void subscriber_free(void *item)
{
	mem_free(item);
}

static int event_dispatch(Vec *subscribers, const Event *ev)
{
	size_t i;
	int delivered = 0;

	for (i = 0; i < vec_len(subscribers); i++) {
		Subscriber *sub = vec_get(subscribers, i);
		if (sub == NULL || sub->fn == NULL)
			continue;
		sub->invocations++;
		sub->fn(ev, sub->ctx);
		delivered++;
	}
	return delivered;
}

EventBus *event_bus_new(void)
{
	EventBus *bus = mem_calloc(1, sizeof(EventBus));
	int i;

	for (i = 0; i < EVENT_KIND_COUNT; i++)
		bus->subscribers[i] = vec_new();
	LOG_DEBUG("event bus created with %d channels", EVENT_KIND_COUNT);
	return bus;
}

void event_bus_free(EventBus *bus)
{
	int i;

	if (bus == NULL)
		return;
	for (i = 0; i < EVENT_KIND_COUNT; i++)
		vec_free_items(bus->subscribers[i], subscriber_free);
	mem_free(bus);
}

int event_subscribe(EventBus *bus, EventKind kind, EventHandlerFn fn, void *ctx)
{
	if (bus == NULL || fn == NULL || !event_kind_valid(kind))
		return 0;
	vec_push(bus->subscribers[kind], subscriber_new(fn, ctx));
	LOG_DEBUG("subscribed handler to '%s'", event_kind_name(kind));
	return 1;
}

int event_subscribe_all(EventBus *bus, EventHandlerFn fn, void *ctx)
{
	int i;
	int count = 0;

	for (i = 0; i < EVENT_KIND_COUNT; i++)
		count += event_subscribe(bus, (EventKind)i, fn, ctx);
	return count;
}

int event_emit(EventBus *bus, const Event *ev)
{
	if (bus == NULL || ev == NULL || !event_kind_valid(ev->kind))
		return 0;
	bus->emitted[ev->kind]++;
	return event_dispatch(bus->subscribers[ev->kind], ev);
}

int event_emitf(EventBus *bus, EventKind kind, const char *source,
		const char *message, int code)
{
	Event ev;

	ev.kind = kind;
	ev.source = source;
	ev.message = message;
	ev.code = code;
	return event_emit(bus, &ev);
}

const char *event_kind_name(EventKind kind)
{
	return event_kind_valid(kind) ? g_event_names[kind] : "unknown";
}

size_t event_subscriber_count(const EventBus *bus, EventKind kind)
{
	if (bus == NULL || !event_kind_valid(kind))
		return 0;
	return vec_len(bus->subscribers[kind]);
}

unsigned event_emitted_count(const EventBus *bus, EventKind kind)
{
	if (bus == NULL || !event_kind_valid(kind))
		return 0u;
	return bus->emitted[kind];
}

void event_bus_report(const EventBus *bus)
{
	int i;

	if (bus == NULL)
		return;
	for (i = 0; i < EVENT_KIND_COUNT; i++) {
		EventKind kind = (EventKind)i;
		LOG_INFO("event %-11s subscribers=%zu emitted=%u",
			 event_kind_name(kind), event_subscriber_count(bus, kind),
			 event_emitted_count(bus, kind));
	}
}
