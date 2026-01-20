#ifndef MEVENT_H_
#define MEVENT_H_

typedef enum {
    MEventTypeKeyDown,
    MEventTypeKeyUp,
    MEventTypeMouseDown,
    MEventTypeMouseUp,
    MEventTypeMouseMove,
    MEventTypeScroll,
    MEventTypeOther
} MEventType;
typedef struct MEvent {
    MEventType type;
} MEvent;

#endif