#include <stdio.h>
#include <CoreGraphics/CoreGraphics.h>
#include "MQueue.h"
#include "MInterpreter.h"
#include <signal.h>
volatile sig_atomic_t running = 1; 

void handle_sigint(int sig) {
    printf("\nCaught Ctrl-C (SIGINT), exiting...\n");
    running = 0; 
}
int main() {
    init_globals();
    MFile mf = read_file("/Users/codinggenius/MacHK/src/files/test_script/script.msr");
    MScript script = parse_script(&mf);
    print_script(&script);
    print_vars();
    CGEventMask mask = CGEventMaskBit(kCGEventKeyDown);
    CFMachPortRef tap = CGEventTapCreate(
        kCGSessionEventTap,
        kCGHeadInsertEventTap,
        kCGEventTapOptionDefault,
        mask,
        hotkey_callback,
        &script
    );

    if (!tap) { fprintf(stderr, "Failed to create event tap\n"); return 1; }

    CFRunLoopAddSource(CFRunLoopGetCurrent(), CFMachPortCreateRunLoopSource(kCFAllocatorDefault, tap, 0), kCFRunLoopCommonModes);
    CGEventTapEnable(tap, true);

    while(running)
    {
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.1, true);
        process();
    }

    free_script(&script);
    free_mfile(&mf);
    destroy_nodes();
    return 0;
}