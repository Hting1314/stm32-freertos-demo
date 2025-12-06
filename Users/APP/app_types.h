#ifndef APP_TYPES_H
#define APP_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    CMD_NONE = 0,
    CMD_TOGGLE,
    // 后续可以继续扩展其他命令
} CmdType;

#ifdef __cplusplus
}
#endif

#endif /* APP_TYPES_H */
