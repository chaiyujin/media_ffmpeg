/* -----------------------------
* File   : log.h
* Author : Yuki Chai
* Created: 2016.11.28
* Project: Yuki
*/

#pragma once
#ifndef __YUKI_LOG_H__
#define __YUKI_LOG_H__

// #define CHECK_LEVEL_NONE // uncomment this line to disable runtime check

#include <stdio.h>
#include <stdarg.h>
#include <vector>

namespace Yuki {
    enum LOG_TYPE {
        LOG_LEVEL_NONE = 0, // 0
        LOG_LEVEL_ERROR, // 1
        LOG_LEVEL_WARNING, // 2
        LOG_LEVEL_NOTICE, // 3
        LOG_LEVEL_LOG, // 4
        LOG_LEVEL_DEBUG, // 5
        LOG_LEVEL_NEVER // 6
    };
	extern const char * _operations[];
#ifndef LOG_BUILD_LEVEL
#ifdef NDEBUG
#define LOG_BUILD_LEVEL LOG_LEVEL_LOG
#else
#define LOG_BUILD_LEVEL LOG_LEVEL_DEBUG
#endif
#endif

#define __LOG_FUNTION__(HEAD) \
    va_list args;\
    va_start(args, format);\
    for (uint32_t i = 0; i < instance->fp_list.size(); ++i) {\
        fprintf  (instance->fp_list[i], "%s", HEAD);\
        vfprintf (instance->fp_list[i], format, args);\
        fflush   (instance->fp_list[i]);\
    }\
    va_end(args);\

#define _LE 0
#define _LT 1
#define _GE 2
#define _GT 3
#define _EQ 4
#define _NE 5

#ifndef CHECK_LEVEL_NONE
#define CHECK(assertion) do {\
		bool flag = assertion;\
		if (!flag) {\
			LOG::error("[%s] (line: %d) : CHECK fail\n", __FUNCTION__, __LINE__);\
			system("pause");exit(1);\
		}\
	} while (0);

#define CHECK_OP(op, x, y) do {\
        bool flag = _compare(op, x, y);\
        if (!flag) {\
            LOG::error("[%s] (line: %d) : CHECK_OP '%s' \n", __FUNCTION__, __LINE__, _operations[op]);\
			system("pause");exit(1);\
        }\
    } while (0);
#else
#define CHECK(assertion)
#define CHECK_OP(op, x, y)
#endif

#define CHECK_LE(x, y) CHECK_OP(_LE, x, y)
#define CHECK_LT(x, y) CHECK_OP(_LT, x, y)
#define CHECK_GE(x, y) CHECK_OP(_GE, x, y)
#define CHECK_GT(x, y) CHECK_OP(_GT, x, y)
#define CHECK_EQ(x, y) CHECK_OP(_EQ, x, y)
#define CHECK_NE(x, y) CHECK_OP(_NE, x, y)

    template <class T, class U>
	inline bool _compare(short op, T x, U y) {
		switch (op) {
		case _LE: return (x <= y); break;
		case _LT: return (x < y); break;
		case _GE: return (x >= y); break;
		case _GT: return (x > y); break;
		case _EQ: return (x == y); break;
		case _NE: return (x != y); break;
		}
		return false;
	}
    
    class LOG {
    private:
        // members
        static LOG *instance;
        LOG_TYPE log_level_runtime;
        std::vector<FILE *> fp_list;

        // method
        LOG(LOG_TYPE t = LOG_BUILD_LEVEL) : log_level_runtime(t) {
            fp_list.push_back(stdout);
        }
        
        static LOG *create_instance() {
            return new LOG();
        }

    public:
        static void init(LOG_TYPE log_type) {
            delete instance;
            instance = new LOG(log_type);
        }
        static void add_log_file(FILE *fp) {
            if (!instance) instance = create_instance();
            instance->fp_list.push_back(fp);
        }
        static void destroy() {
            delete instance;
        }
        static void none(const char *format, ...) {}
        static void error(const char *format, ...) {
            if (!instance) instance = create_instance();
            if (LOG_LEVEL_ERROR <= instance->log_level_runtime) {
                __LOG_FUNTION__("ERROR: ")
            }
        }
        static void warning(const char *format, ...) {
            if (!instance) instance = create_instance();
            if (LOG_LEVEL_WARNING <= instance->log_level_runtime) {
                __LOG_FUNTION__("WARNING: ")
            }
        }
        static void notice(const char *format, ...) {
            if (!instance) instance = create_instance();
            if (LOG_LEVEL_NOTICE <= instance->log_level_runtime) {
                __LOG_FUNTION__("NOTICE: ")
            }
        }
        static void log(const char *format, ...) {
            if (!instance) instance = create_instance();
            if (LOG_LEVEL_LOG <= instance->log_level_runtime) {
                __LOG_FUNTION__("LOG: ")
            }
        }
        static void debug(const char *format, ...) {
            if (!instance) instance = create_instance();
            if (LOG_LEVEL_DEBUG <= instance->log_level_runtime) {
                __LOG_FUNTION__("DEBUG: ")
            }
        }
        static void never(const char *format, ...) {
            
        }
    };
}

#endif  // !__YUKI_LOG_H__