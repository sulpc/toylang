#ifndef _TL_CONFIG_H_
#define _TL_CONFIG_H_

// clang-format off
#define TL_PLATFORM_32                         1
#define TL_PLATFORM_64                         2
#define TL_PLATFORM                            TL_PLATFORM_32

#define TL_STRING_INTERALIZE_ENABLE            1
#define TL_STRING_POOL_SLOTS_DEFAULT           4            // 64
#define TL_STRING_HASH_SEED                    0x10242048   //
#define TL_MAP_CAPACITY_DEFALUT                4
#define TL_LIST_CAPACITY_DEFALUT               4

#define TL_GC_THRESHOLD_INIT                   512     // gc threshold after init
#define TL_GC_THRESHOLD_DEC                    32      // gc threshold decreased when trigger
#define TL_GC_THRESHOLD_MAX                    2048    //
#define TL_GC_THRESHOLD_MUL                    3 / 2   //
#define TL_GC_STEP_PROCESS_SIZE                512     //
#define TL_GC_STEP_MAX_SWEEP_NUM               24      //

#define TL_LEXER_MAX_TOKEN_LEN                 128

#define TL_DEBUG_OBJNAME_EN                    1
#define TL_DEBUG_MEMINFO_EN                    1       // meminfo
// clang-format on

#endif
