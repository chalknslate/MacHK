#ifndef MQUEUE_H
#define MQUEUE_H
#include <ApplicationServices/ApplicationServices.h>
#include <unistd.h>

#include <stdlib.h>
#include <stdarg.h>
void initialize()
{
    CGDisplayShowCursor(kCGDirectMainDisplay);
}
typedef enum
{
    M_Success,
    M_MemoryFailure,
    M_ExceptionNoItemsLeft,
    M_PushFailure,
} MErrorCodes;

#define _iter(_F, ...)   \
    _F(Empty, -1, __VA_ARGS__) \
    _F(MouseUp, 0, __VA_ARGS__) \
    _F(MouseDown, 1, __VA_ARGS__) \
    _F(MouseMove, 2, __VA_ARGS__) \
    _F(MouseClick, 3, __VA_ARGS__) \

#define types_enum(uc, i, ...) \
    MEvent_##uc = i,

typedef enum MEventType {
    _iter(types_enum)
} MEventType;


typedef struct MouseUp { int x, y; CGMouseButton clickType; } MouseUp_t;
typedef struct MouseClick { int x, y; CGMouseButton clickType; } MouseClick_t;
typedef struct MouseDown { int x, y; CGMouseButton clickType; } MouseDown_t;
typedef struct MouseMove { int x, y; float duration; } MouseMove_t;
typedef struct { char empty; } Empty_t;

typedef struct {
    union {
#define unionmem(uc, ...) uc##_t uc;
        _iter(unionmem)
    };

    MEventType type;
} MQueueNode;


struct
{
    MQueueNode* nodes;
    int nodeCount;
} MDataQueue;

MQueueNode create_node(MEventType type, ...)
{
    MQueueNode node;
    va_list args;
    va_start(args, type);
    switch(type)
    {
        default:
        {
            break;
        }
        case MEvent_MouseClick:
        {
            node.MouseClick.x = va_arg(args, int);
            node.MouseClick.y = va_arg(args, int);
            node.MouseClick.clickType = va_arg(args, CGMouseButton);
            break;
        }
        case MEvent_MouseDown:
        {
            node.MouseDown.x = va_arg(args, int);
            node.MouseDown.y = va_arg(args, int);
            node.MouseDown.clickType = va_arg(args, CGMouseButton);
            break;
        }
        case MEvent_MouseUp:
        {
            node.MouseUp.x = va_arg(args, int);
            node.MouseUp.y = va_arg(args, int);
            node.MouseUp.clickType = va_arg(args, CGMouseButton);
            break;
        }
        case MEvent_MouseMove:
        {
            node.MouseMove.x = va_arg(args, int);
            node.MouseMove.y = va_arg(args, int);
            node.MouseMove.duration = va_arg(args, double);
            break;
        }
    }
    va_end(args);

    node.type = type;
    return node;
};
void destroy_nodes()
{
    free(MDataQueue.nodes);
}
int push_node(MQueueNode node)
{
    MQueueNode* new_nodes =
        realloc(MDataQueue.nodes, sizeof(MQueueNode) * (MDataQueue.nodeCount + 1));

    if (!new_nodes) return M_MemoryFailure;

    MDataQueue.nodes = new_nodes;
    MDataQueue.nodes[MDataQueue.nodeCount++] = node;
    return M_Success;
}

MQueueNode pop_node()
{
    if (MDataQueue.nodeCount <= 0) {
        MQueueNode empty;
        empty.Empty.empty = '\0';
        empty.type = MEvent_Empty;
        return empty; 
    }

    MQueueNode node = MDataQueue.nodes[MDataQueue.nodeCount - 1];
    MDataQueue.nodeCount--;

    if (MDataQueue.nodeCount == 0) {
        free(MDataQueue.nodes);
        MDataQueue.nodes = NULL;
    } else {
        MQueueNode* new_nodes =
            realloc(MDataQueue.nodes, sizeof(MQueueNode) * MDataQueue.nodeCount);
        if (new_nodes) MDataQueue.nodes = new_nodes;
    }

    return node;
}



void process()
{
    MQueueNode node = pop_node();
    if(node.type == MEvent_Empty)
    {
        return;
    }
    switch(node.type)
    {
        default:
        {
            break;
        }
        case MEvent_MouseClick: {
            //printf("Clicking...\n");
            CGEventRef click1_down = CGEventCreateMouseEvent(
                NULL, kCGEventLeftMouseDown,
                CGPointMake(node.MouseClick.x, node.MouseClick.y),
                kCGMouseButtonLeft
            );
            CGEventRef click1_up = CGEventCreateMouseEvent(
                NULL, kCGEventLeftMouseUp,
                CGPointMake(node.MouseClick.x, node.MouseClick.y),
                kCGMouseButtonLeft
            );
            CGEventPost(kCGHIDEventTap, click1_down);
            CGEventPost(kCGHIDEventTap, click1_up);
            CFRelease(click1_down);
            CFRelease(click1_up);
        } break;


        case MEvent_MouseDown: {
            //printf("Mouse down...\n");
            CGEventType type =
                (node.MouseDown.clickType == kCGMouseButtonRight)
                    ? kCGEventRightMouseDown
                    : kCGEventLeftMouseDown;

            CGEventRef event = CGEventCreateMouseEvent(
                NULL, type,
                CGPointMake(node.MouseDown.x, node.MouseDown.y),
                node.MouseDown.clickType
            );
            CGEventPost(kCGHIDEventTap, event);
            CFRelease(event);
            return;
        } break;

        case MEvent_MouseUp: {
            //printf("Mouse up...\n");
            CGEventType type =
                (node.MouseUp.clickType == kCGMouseButtonRight)
                    ? kCGEventRightMouseUp
                    : kCGEventLeftMouseUp;

            CGEventRef event = CGEventCreateMouseEvent(
                NULL, type,
                CGPointMake(node.MouseUp.x, node.MouseUp.y),
                node.MouseUp.clickType
            );
            CGEventPost(kCGHIDEventTap, event);
            CFRelease(event);
            return;
        } break;
        case MEvent_MouseMove: {
            //printf("Moving...\n");
            CGEventRef event = CGEventCreateMouseEvent(
                NULL, kCGEventMouseMoved,
                CGPointMake(node.MouseMove.x, node.MouseUp.y),
                kCGMouseButtonLeft
            );
            CGEventPost(kCGHIDEventTap, event);
            CFRelease(event);
        } break;


    }
}


#endif