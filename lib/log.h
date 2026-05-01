#ifndef AUTOMATOR_LOG_H_
#define AUTOMATOR_LOG_H_

#include <stdio.h>

/*
 * Простая логирующая абстракция для libautomator.
 *
 * Уровни (от самого подробного к самому критичному):
 *   AUTOMATOR_LOG_DEBUG, AUTOMATOR_LOG_INFO, AUTOMATOR_LOG_WARN, AUTOMATOR_LOG_ERROR
 *
 * Уровень по умолчанию задаётся макросом AUTOMATOR_LOG_LEVEL во время
 * сборки (в Release-сборках имеет смысл задавать AUTOMATOR_LOG_LEVEL=2,
 * чтобы скрыть DEBUG/INFO).
 *
 * Пример: -DAUTOMATOR_LOG_LEVEL=2
 */

#define AUTOMATOR_LOG_DEBUG 0
#define AUTOMATOR_LOG_INFO  1
#define AUTOMATOR_LOG_WARN  2
#define AUTOMATOR_LOG_ERROR 3

#ifndef AUTOMATOR_LOG_LEVEL
#  ifdef NDEBUG
#    define AUTOMATOR_LOG_LEVEL AUTOMATOR_LOG_WARN
#  else
#    define AUTOMATOR_LOG_LEVEL AUTOMATOR_LOG_INFO
#  endif
#endif

#define LOG_DEBUG(fmt, ...) \
    do { if (AUTOMATOR_LOG_LEVEL <= AUTOMATOR_LOG_DEBUG) \
        fprintf(stderr, "[DEBUG] " fmt "\n", ##__VA_ARGS__); } while (0)

#define LOG_INFO(fmt, ...) \
    do { if (AUTOMATOR_LOG_LEVEL <= AUTOMATOR_LOG_INFO) \
        fprintf(stderr, "[INFO] " fmt "\n", ##__VA_ARGS__); } while (0)

#define LOG_WARN(fmt, ...) \
    do { if (AUTOMATOR_LOG_LEVEL <= AUTOMATOR_LOG_WARN) \
        fprintf(stderr, "[WARN] " fmt "\n", ##__VA_ARGS__); } while (0)

#define LOG_ERROR(fmt, ...) \
    do { if (AUTOMATOR_LOG_LEVEL <= AUTOMATOR_LOG_ERROR) \
        fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__); } while (0)

#endif /* AUTOMATOR_LOG_H_ */
