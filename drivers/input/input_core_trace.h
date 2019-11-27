#ifdef VENDOR_EDIT
#undef TRACE_SYSTEM
#define TRACE_SYSTEM input_core_trace

#if !defined(_TRACE_INPUT_CORE_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_INPUT_CORE_H

#include <linux/tracepoint.h>

TRACE_EVENT(input_core,
	TP_PROTO(const char* func_name, unsigned int type, unsigned int code, int value),

	TP_ARGS(func_name, type, code, value),

	TP_STRUCT__entry(
			__string(func_name, func_name)
			__field(u32, type)
			__field(u32, code)
			__field(s32, value)
	),

	TP_fast_assign(
			__assign_str(func_name, func_name);
            __entry->type = type;
			__entry->code = code;
			__entry->value = value;
	),

	TP_printk("%s, type = 0x%X, code = 0x%X, value = %d", __get_str(func_name), __entry->type, __entry->code, __entry->value)
);

#endif /* !defined(_TRACE_INPUT_CORE_H) || defined(TRACE_HEADER_MULTI_READ) */

/* This part must be outside protection */
#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#define TRACE_INCLUDE_FILE input_core_trace
#include <trace/define_trace.h>
#endif /* VENDOR_EDIT */
