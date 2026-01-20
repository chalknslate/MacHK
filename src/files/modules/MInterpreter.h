#ifndef MINTERPRETER_H
#define MINTERPRETER_H
#define MAX_STACK 256
#define MAX_VARS 256

typedef struct {
    char name[32];
    int value;
} MVar;

typedef struct {
    MVar vars[MAX_VARS];
    int count;
} MVarFrame;

// --- globals + locals support ---

static MVarFrame vstack[MAX_STACK];
static int vsp = -1;

// call this once at startup
static void init_globals() {
    vsp = 0;
    vstack[0].count = 0; // frame 0 = globals
}

static void push_frame() {
    vsp++;
    vstack[vsp].count = 0;
}

static void pop_frame() {
    if (vsp > 0) vsp--; // never pop globals
}

// find local first, then global
static int* find_var(const char *name) {
    // locals (top -> 1)
    for (int i = vsp; i >= 1; i--) {
        for (int j = 0; j < vstack[i].count; j++) {
            if (strcmp(vstack[i].vars[j].name, name) == 0)
                return &vstack[i].vars[j].value;
        }
    }

    // globals (frame 0)
    for (int j = 0; j < vstack[0].count; j++) {
        if (strcmp(vstack[0].vars[j].name, name) == 0)
            return &vstack[0].vars[j].value;
    }

    return NULL;
}
static void print_vars() {
    printf("=== VARIABLES ===\n");

    // globals
    printf("[globals]\n");
    for (int j = 0; j < vstack[0].count; j++) {
        printf("  %s = %d\n",
               vstack[0].vars[j].name,
               vstack[0].vars[j].value);
    }

    // locals (each frame)
    for (int i = 1; i <= vsp; i++) {
        printf("[frame %d]\n", i);
        for (int j = 0; j < vstack[i].count; j++) {
            printf("  %s = %d\n",
                   vstack[i].vars[j].name,
                   vstack[i].vars[j].value);
        }
    }

    printf("=================\n");
}

// assign: prefer existing local, then existing global, else create local
static void set_var(const char *name, int value) {
    // update local if exists
    for (int i = vsp; i >= 1; i--) {
        for (int j = 0; j < vstack[i].count; j++) {
            if (strcmp(vstack[i].vars[j].name, name) == 0) {
                vstack[i].vars[j].value = value;
                return;
            }
        }
    }

    // update global if exists
    for (int j = 0; j < vstack[0].count; j++) {
        if (strcmp(vstack[0].vars[j].name, name) == 0) {
            vstack[0].vars[j].value = value;
            return;
        }
    }

    // create new var in current frame (or globals if no locals yet)
    int target = (vsp >= 1) ? vsp : 0;
    MVarFrame *f = &vstack[target];

    strcpy(f->vars[f->count].name, name);
    f->vars[f->count].value = value;
    f->count++;
}
static void set_global_var(const char *name, int value) {
    for (int j = 0; j < vstack[0].count; j++) {
        if (strcmp(vstack[0].vars[j].name, name) == 0) {
            vstack[0].vars[j].value = value;
            return;
        }
    }

    MVarFrame *f = &vstack[0];
    strcpy(f->vars[f->count].name, name);
    f->vars[f->count].value = value;
    f->count++;
}


#include <ApplicationServices/ApplicationServices.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    const char *name;
    CGKeyCode code;
} KeyLookup;

static KeyLookup key_table[] = {
    {"A", 0}, {"S", 1}, {"D", 2}, {"F", 3},
    {"H", 4}, {"G", 5}, {"Z", 6}, {"X", 7},
    {"C", 8}, {"V", 9}, {"B", 11}, {"Q", 12},
    {"W", 13}, {"E", 14}, {"R", 15}, {"Y", 16},
    {"T", 17}, {"1", 18}, {"2", 19}, {"3", 20},
    {"4", 21}, {"6", 22}, {"5", 23}, {"=", 24},
    {"9", 25}, {"7", 26}, {"-", 27}, {"8", 28},
    {"0", 29}, {"RightBracket", 30}, {"O", 31}, {"U", 32},
    {"LeftBracket", 33}, {"I", 34}, {"P", 35}, {"L", 37},
    {"J", 38}, {"'", 39}, {"K", 40}, {";", 41},
    {"\\", 42}, {",", 43}, {"/", 44}, {"N", 45},
    {"M", 46}, {".", 47}, {"Tab", 48}, {"Space", 49},
    {"Backtick", 50}, {"Escape", 53},
    {"F1", 122}, {"F2", 120}, {"F3", 99}, {"F4", 118},
    {"F5", 96}, {"F6", 97}, {"F7", 98}, {"F8", 100},
    {"F9", 101}, {"F10", 109}, {"F11", 103}, {"F12", 111}
};

static CGKeyCode get_keycode(const char *key) {
    for (size_t i = 0; i < sizeof(key_table)/sizeof(key_table[0]); i++) {
        if (strcmp(key, key_table[i].name) == 0) return key_table[i].code;
    }
    return UINT16_MAX;
}

typedef struct {
    char **lines;
    size_t line_count;
} MFile;

static MFile read_file(const char *filename) {
    MFile mf = {0};
    FILE *f = fopen(filename, "r");
    if (!f) { perror("Failed to open file"); return mf; }

    size_t capacity = 16;
    mf.lines = (char**)malloc(capacity * sizeof(char*));
    if (!mf.lines) { fclose(f); return mf; }

    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&line, &len, f)) != -1) {
        if (read > 0 && line[read - 1] == '\n') line[read - 1] = '\0';

        if (mf.line_count >= capacity) {
            capacity *= 2;
            char **new_lines = (char**)realloc(mf.lines, capacity * sizeof(char*));
            if (!new_lines) break;
            mf.lines = new_lines;
        }

        mf.lines[mf.line_count] = strdup(line);
        if (!mf.lines[mf.line_count]) break;
        mf.line_count++;
    }

    free(line);
    fclose(f);
    return mf;
}

static void free_mfile(MFile *mf) {
    for (size_t i = 0; i < mf->line_count; i++) free(mf->lines[i]);
    free(mf->lines);
    mf->lines = NULL;
    mf->line_count = 0;
}
#define _cmd_iter(_F, ...)             \
    _F(CursorMove, 0, __VA_ARGS__)    \
    _F(KeyPress, 1, __VA_ARGS__)      \
    _F(KeyRelease, 2, __VA_ARGS__) \
    _F(HMouseClick, 3, __VA_ARGS__) \
    _F(SetVar, 4, __VA_ARGS__) \

#define htypes_enum(name, val, ...) MCommandType_##name = val,

typedef enum { _cmd_iter(htypes_enum) } MCommandType;

typedef struct CursorMove { 
    char expr_x[32]; 
    char expr_y[32]; 
    float duration; 
} CursorMove_t;

typedef struct KeyPress   { char key; } KeyPress_t;
typedef struct KeyRelease { char key; } KeyRelease_t;
typedef struct HMouseClick { 
    char expr_x[32]; 
    char expr_y[32]; 
    float clickType;
} HMouseClick_t;
typedef struct SetVar { char name[32]; char expr[64]; } SetVar_t;
static int eval_expr(const char *expr) {
    char buf[64];
    strcpy(buf, expr);

    char *t = strtok(buf, " ");
    int result = 0;
    char op = 0;

    while (t) {
        int val = 0;
        if (isdigit(*t) || (*t == '-' && isdigit(t[1]))) {
            val = atoi(t);
        } else {
            int *v = find_var(t);
            val = v ? *v : 0;
        }

        if (!op) result = val;
        else if (op == '+') result += val;
        else if (op == '-') result -= val;

        t = strtok(NULL, " ");
        if (t) { op = *t; t = strtok(NULL, " "); }
    }

    return result;
}


typedef struct {
    union {
#define hunionmem(name, val, ...) name##_t name;
        _cmd_iter(hunionmem)
    };
    MCommandType type;
} MCommand;

typedef struct {
    char key[16];
    MCommand *commands;
    size_t cmd_count;
} MHotkey;

typedef struct {
    MHotkey *hotkeys;
    size_t hotkey_count;
} MScript;

static MScript parse_script(const MFile *mf) {
    MScript script = {0};
    MHotkey *current_hotkey = NULL;

    for (size_t i = 0; i < mf->line_count; i++) {
        char *line = mf->lines[i];
        if (line[0] == '#' || strlen(line) == 0) continue;

        char *trim_line = line;
        while (*trim_line && isspace(*trim_line)) trim_line++;
        if (strncmp(trim_line, "varint", 6) == 0 || strncmp(trim_line, "global varint", 13) == 0) {
            char name[32];
            int val;
            int is_global = 0;

            if (strncmp(trim_line, "global varint", 13) == 0) {
                is_global = 1;
                if (sscanf(trim_line + 13, " %31s = %d", name, &val) == 2) {
                    if (is_global) set_global_var(name, val);
                    else set_var(name, val); // redundant, but keeps API consistent
                }
            } else {
                if (sscanf(trim_line + 6, " %31s = %d", name, &val) == 2) {
                    set_var(name, val);
                }
            }
            continue;
        }


        if (strncmp(trim_line, "hotkey", 6) == 0) {
            script.hotkeys = (MHotkey*)realloc(script.hotkeys, (script.hotkey_count + 1) * sizeof(MHotkey));
            current_hotkey = &script.hotkeys[script.hotkey_count];
            current_hotkey->commands = NULL;
            current_hotkey->cmd_count = 0;
            script.hotkey_count++;

            char *key_start = trim_line + 6;
            while (*key_start && isspace(*key_start)) key_start++;
            char *key_end = strstr(key_start, "->");
            if (!key_end) continue;

            size_t key_len = key_end - key_start;
            if (key_len >= sizeof(current_hotkey->key)) key_len = sizeof(current_hotkey->key)-1;
            strncpy(current_hotkey->key, key_start, key_len);
            current_hotkey->key[key_len] = '\0';
            continue;
        }

        if (current_hotkey) {
            MCommand cmd;
            memset(&cmd, 0, sizeof(MCommand));

            if (strncmp(trim_line, "CursorMove,", 11) == 0) {
                char xs[32], ys[32];
                float dur = 0.0f;
                int n = sscanf(trim_line, "CursorMove, %31[^,], %31[^,], %f", xs, ys, &dur);
                if (n >= 2) {
                    strncpy(cmd.CursorMove.expr_x, xs, 31);
                    strncpy(cmd.CursorMove.expr_y, ys, 31);
                    cmd.CursorMove.expr_x[31] = 0;
                    cmd.CursorMove.expr_y[31] = 0;

                    cmd.CursorMove.duration = (n == 3) ? dur : 0.0f;
                    cmd.type = MCommandType_CursorMove;
                } else continue;
            }

            else if (strncmp(trim_line, "set ", 4) == 0) {
                char name[32], expr[64];
                if (sscanf(trim_line, "set %31s = %63[^\n]", name, expr) == 2) {
                    cmd.type = MCommandType_SetVar;
                    strcpy(cmd.SetVar.name, name);
                    strcpy(cmd.SetVar.expr, expr);
                } 
                else 
                {
                    continue;
                }
            }
            else if (strncmp(trim_line, "KeyPress,", 9) == 0) {
                if (sscanf(trim_line, "KeyPress, %c", &cmd.KeyPress.key) == 1) cmd.type = MCommandType_KeyPress;
                else continue;
            }
            else if (strncmp(trim_line, "MouseClick,", 11) == 0) {
                char xs[32], ys[32];
                float dur = 0.0f;
                int n = sscanf(trim_line, "MouseClick, %31[^,], %31[^,], %f", xs, ys, &dur);
                if (n >= 2) {
                    strncpy(cmd.HMouseClick.expr_x, xs, 31);
                    strncpy(cmd.HMouseClick.expr_y, ys, 31);
                    cmd.HMouseClick.expr_x[31] = 0;
                    cmd.HMouseClick.expr_y[31] = 0;

                    cmd.HMouseClick.clickType = 0;
                    cmd.type = MCommandType_HMouseClick;
                } else continue;
            }
            else if (strncmp(trim_line, "KeyRelease,", 11) == 0) {
                if (sscanf(trim_line, "KeyRelease, %c", &cmd.KeyRelease.key) == 1) cmd.type = MCommandType_KeyRelease;
                else continue;
            }
            else continue;

            current_hotkey->commands = (MCommand*)realloc(current_hotkey->commands, (current_hotkey->cmd_count + 1) * sizeof(MCommand));
            current_hotkey->commands[current_hotkey->cmd_count++] = cmd;
        }
    }

    return script;
}

static void free_script(MScript *script) {
    for (size_t i = 0; i < script->hotkey_count; i++)
        free(script->hotkeys[i].commands);
    free(script->hotkeys);
    script->hotkeys = NULL;
    script->hotkey_count = 0;
}
static void print_script(const MScript *script) {
    for (size_t i = 0; i < script->hotkey_count; i++) {
        printf("Hotkey: %s\n", script->hotkeys[i].key);
        for (size_t j = 0; j < script->hotkeys[i].cmd_count; j++) {
            MCommand cmd = script->hotkeys[i].commands[j];
            switch (cmd.type) {
                case MCommandType_CursorMove:
                    printf("  CursorMove: %s, %s, %.2f\n",
                           cmd.CursorMove.expr_x,
                           cmd.CursorMove.expr_y,
                           cmd.CursorMove.duration);
                    break;

                case MCommandType_KeyPress:
                    printf("  KeyPress: %c\n", cmd.KeyPress.key);
                    break;

                case MCommandType_KeyRelease:
                    printf("  KeyRelease: %c\n", cmd.KeyRelease.key);
                    break;

                case MCommandType_HMouseClick:
                    printf("  MouseClick: %s, %s, %.2f\n",
                           cmd.HMouseClick.expr_x,
                           cmd.HMouseClick.expr_y,
                           cmd.HMouseClick.clickType);
                    break;

                case MCommandType_SetVar:
                    printf("  SetVar: %s = %s\n",
                           cmd.SetVar.name,
                           cmd.SetVar.expr);
                    break;

                default: break;
            }
        }
    }
}


static void execute_command(MCommand *cmd) {
    switch (cmd->type) {
        case MCommandType_SetVar: {
            int v = eval_expr(cmd->SetVar.expr);
            set_var(cmd->SetVar.name, v);
            break;
        }

        case MCommandType_CursorMove: {
            int x = eval_expr(cmd->CursorMove.expr_x); // <- new: store expr as string
            int y = eval_expr(cmd->CursorMove.expr_y);
            push_node(create_node(MEvent_MouseMove, x, y, cmd->CursorMove.duration));
            break;
        }

        case MCommandType_HMouseClick: {
            int x = eval_expr(cmd->HMouseClick.expr_x); // <- new: store expr as string
            int y = eval_expr(cmd->HMouseClick.expr_y);
            //printf("%d, %d\n", x, y);
            push_node(create_node(MEvent_MouseClick, x, y, cmd->HMouseClick.clickType));
            break;
        }

        default: break;
    }
}
static CGEventRef hotkey_callback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *userInfo) {
    if (type != kCGEventKeyDown) return event;

     CGKeyCode code = (CGKeyCode)CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
    //printf("Key pressed code: %u\n", code);  // log every∂key press
    MScript *script = (MScript*)userInfo;
    //print_script(script);
    for (size_t i = 0; i < script->hotkey_count; i++) {
        MHotkey *hk = &script->hotkeys[i];
        CGKeyCode hk_code = get_keycode(hk->key);
        //printf("key: %s, retrieved: %u, %u bool: %d\n", hk->key, code, get_keycode(hk->key), hk_code == code);
        if (hk_code == code) {
            push_frame();
            for (size_t j = 0; j < hk->cmd_count; j++) {
                execute_command(&hk->commands[j]);
            }
            pop_frame();
        }

    }

    return event;
}

#endif
