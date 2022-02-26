/*++

Module Name:

    Trace.h

Abstract:

    Header file for the debug tracing related function defintions and macros.

Environment:

    Kernel mode

--*/



//
// Define the tracing flags.
//
// Tracing GUID - 71cdcbfc-b105-47d5-8fb9-c242a168b823
//

#define WPP_CONTROL_GUIDS                                              \
    WPP_DEFINE_CONTROL_GUID(                                           \
        EUSBTraceGuid, (71cdcbfc,b105,47d5,8fb9,c242a168b823), \
                                                                            \
        WPP_DEFINE_BIT(MYDRIVER_ALL_INFO)                              \
        WPP_DEFINE_BIT(TRACE_DRIVER)                                   \
        WPP_DEFINE_BIT(TRACE_HC_DEVICE)                                \
        WPP_DEFINE_BIT(TRACE_USB_DEVICE)                               \
        WPP_DEFINE_BIT(TRACE_QUEUE)                                    \
        )                             

#define WPP_FLAG_LEVEL_LOGGER(flag, level)                                  \
    WPP_LEVEL_LOGGER(flag)

#define WPP_FLAG_LEVEL_ENABLED(flag, level)                                 \
    (WPP_LEVEL_ENABLED(flag) &&                                             \
     WPP_CONTROL(WPP_BIT_ ## flag).Level >= level)

#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) \
           WPP_LEVEL_LOGGER(flags)
               
#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) \
           (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)

//           
// WPP orders static parameters before dynamic parameters. To support the Trace function
// defined below which sets FLAGS=MYDRIVER_ALL_INFO, a custom macro must be defined to
// reorder the arguments to what the .tpl configuration file expects.
//
#define WPP_RECORDER_FLAGS_LEVEL_ARGS(flags, lvl) WPP_RECORDER_LEVEL_FLAGS_ARGS(lvl, flags)
#define WPP_RECORDER_FLAGS_LEVEL_FILTER(flags, lvl) WPP_RECORDER_LEVEL_FLAGS_FILTER(lvl, flags)


// shortcuts
#define TL_FATAL        TRACE_LEVEL_FATAL
#define TL_ERROR        TRACE_LEVEL_ERROR
#define TL_WARNING      TRACE_LEVEL_WARNING
#define TL_INFO         TRACE_LEVEL_INFORMATION
#define TL_VERBOSE      TRACE_LEVEL_VERBOSE

//
// This comment block is scanned by the trace preprocessor to define our
// Trace function.
//
// begin_wpp config
// FUNC Trace{FLAGS=MYDRIVER_ALL_INFO}(LEVEL, MSG, ...);
// FUNC TraceEvents(LEVEL, FLAGS, MSG, ...);
// LogFatal{LEVEL=TL_FATAL}(FLAGS,MSG,...);
// LogError{LEVEL=TL_ERROR}(FLAGS,MSG,...);
// LogWarning{LEVEL=TL_WARNING}(FLAGS,MSG,...);
// LogInfo{LEVEL=TL_INFO}(FLAGS,MSG,...);
// LogVerbose{LEVEL=TL_INFO}(FLAGS,MSG,...);
// 
// end_wpp
//