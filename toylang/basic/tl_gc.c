#define THIS_IS_IN_TL_GC_C

#include "tl_gc.h"
#include "tl_func.h"
#include "tl_list.h"
#include "tl_map.h"
#include "tl_string.h"

#define gc_debug 0
#define gc_info  tl_none


#define STATE_RESTART   0
#define STATE_PROPAGATE 1
#define STATE_ATOMIC    2
#define STATE_SWEEP     3
#define COLOR_GRAY      0                               // 0
#define COLOR_WHITE1    util_bitmask(0)                 // 1
#define COLOR_WHITE2    util_bitmask(1)                 // 2
#define COLOR_WHITE     (COLOR_WHITE1 | COLOR_WHITE2)   // 3
#define COLOR_BLACK     util_bitmask(2)                 // 4

#define isgray(gco)     ((gco)->marked == COLOR_GRAY)
#define iswhite(gco)    util_chkbits((gco)->marked, COLOR_WHITE)
#define isblack(gco)    ((gco)->marked == COLOR_BLACK)
#define white()         (tl_env.gc_currentwhite)
#define otherwhite()    (tl_env.gc_currentwhite ^ COLOR_WHITE)
#define white2gray(gco) (gco)->marked = COLOR_GRAY
#define black2gray(gco) (gco)->marked = COLOR_GRAY
#define gray2black(gco) (gco)->marked = COLOR_BLACK

#define markvalue(obj)                                                                                                 \
    if (tl_isgco(obj) && iswhite(tl_value2gco(obj))) {                                                                 \
        gc_markobj(tl_value2gco(obj));                                                                                 \
    }
#define link2gclist(gco, gcolist) (gco)->nextgray = gcolist, gcolist = (tl_gcobj_t*)(gco)

#define color2str(color) color_names[(color)]
const char*        color_names[]   = {"gray", "white1", "white2", "white", "black"};
static const char* gcstate_names[] = {"RESTART", "PROPAGATE", "ATOMIC", "SWEEP"};

static tl_size_t    gc_onestep(void);
static void         gc_restart(void);
static void         gc_propagate(void);
static void         gc_atomic(void);
static void         gc_sweep(void);
static tl_gcobj_t** gc_sweeplist(tl_gcobj_t** ptr, tl_size_t max);
static void         gc_markobj(tl_gcobj_t* gco);
static tl_size_t    gc_freeobj(tl_gcobj_t* gco);
static tl_size_t    gc_traverse_list(tl_list_t* list);
static tl_size_t    gc_traverse_map(tl_map_t* map);


void tl_gc_init(void)
{
    tl_env.gc_state        = STATE_RESTART;
    tl_env.gc_currentwhite = COLOR_WHITE1;
    tl_env.gc_all          = nullptr;
    tl_env.gc_gray         = nullptr;
    tl_env.gc_grayagain    = nullptr;
    tl_env.gc_sweepptr     = nullptr;
    tl_env.gc_threshold    = TL_GC_THRESHOLD_INIT;
    tl_env.gc_debt         = -TL_GC_THRESHOLD_INIT;
    tl_env.gc_debt_last    = tl_env.gc_debt;
    tl_env.gc_processed    = 0;
    tl_env.gc_inited       = true;
    gc_info("gc init, threshold=%d, debt=%d, total=%d", tl_env.gc_threshold, tl_env.gc_debt,
            tl_env.gc_threshold + tl_env.gc_debt);
}


void tl_gc_deinit(void)
{
    // force sweep all gco
    tl_gc_cleanall();
    tl_env.gc_inited = false;
    // tl_env.gc_state        = STATE_RESTART;
    // tl_env.gc_currentwhite = 0;
    // tl_env.gc_gray         = nullptr;
    // tl_env.gc_grayagain    = nullptr;
    // tl_env.gc_sweepptr     = nullptr;
    // tl_env.gc_threshold    = 0;
    // tl_env.gc_debt         = 0;
    // tl_env.gc_processed    = 0;
}


void tl_gc_cleanall(void)
{
    tl_size_t total_last = tl_env.gc_threshold + tl_env.gc_debt;
    tl_size_t total      = 0;

    gc_info("gc freeall, threshold=%d, debt=%d, total=%d", tl_env.gc_threshold, tl_env.gc_debt, total_last);

    while (true) {
        tl_gc_process();
        if (tl_env.gc_state == STATE_RESTART) {
            total = tl_env.gc_threshold + tl_env.gc_debt;
            if (total_last == total || total == 0) {
                gc_info("gc freeall done, threshold=%d, debt=%d, total=%d", tl_env.gc_threshold, tl_env.gc_debt, total);
                break;
            } else {
                gc_info("gc freeall pause, threshold=%d, debt=%d, total=%d, release=%d", tl_env.gc_threshold,
                        tl_env.gc_debt, total, total_last - total);
            }
            total_last = total;
        }
    }
}


tl_gcobj_t* tl_gc_newobj(tl_vtype_t vtype, tl_size_t size)
{
    tl_gcobj_t* obj = (tl_gcobj_t*)tl_new(size);

    obj->marked   = white();
    obj->vtype    = vtype;
    obj->nextgco  = tl_env.gc_all;
    tl_env.gc_all = obj;

    gc_info("gc obj %s<%s> new@%p, objsize=%d", tl_typename(obj), TL_DEBUG_OBJNAME(obj), obj, size);
    return obj;
}


void tl_gc_process(void)
{
    gc_info("gc trigger, threshold=%d, debt=%d", tl_env.gc_threshold, tl_env.gc_debt);

    tl_size_t process = 0;
    do {
        process += gc_onestep();
        gc_info("gc process %d bytes", process);
    } while (process <= TL_GC_STEP_PROCESS_SIZE && tl_env.gc_state != STATE_RESTART);

    if (tl_env.gc_state == STATE_RESTART) {
        // gc completed
        // waits the total memory reach a larger value (relative to the memory at end of current gc)
        // before will triggered again
        tl_size_t inuse     = tl_env.gc_threshold + tl_env.gc_debt;
        tl_env.gc_threshold = util_min2(inuse * TL_GC_THRESHOLD_MUL, TL_GC_THRESHOLD_MAX);
        tl_env.gc_debt      = inuse - tl_env.gc_threshold;
        gc_info("gc done, inuse=%d, threshold=%d, debt=%d", inuse, tl_env.gc_threshold, tl_env.gc_debt);
    } else {
        // gc not completed
        // after alloc more memory (a certain amount) before gc triggered again
        // avoid gc triggered too often
        // tl_env.gc_threshold += TL_GC_STEP_PROCESS_SIZE / 2;
        // tl_env.gc_debt -= TL_GC_STEP_PROCESS_SIZE / 2;
        gc_info("gc pause, threshold=%d, debt=%d", tl_env.gc_threshold, tl_env.gc_debt);
    }
}


void tl_gc_trigger(void)
{
    if (tl_env.gc_debt == tl_env.gc_debt_last) {
        tl_env.gc_debt += TL_GC_THRESHOLD_DEC;
        tl_env.gc_threshold -= TL_GC_THRESHOLD_DEC;
    }

    if (tl_env.gc_debt > 0) {
        tl_gc_process();
    }

    tl_env.gc_debt_last = tl_env.gc_debt;
}


void tl_gc_listbarrier(struct tl_list_t* list, const tl_value_t* value)
{
    if (isblack(list) && tl_isgco(value) && iswhite(value->vv.gco)) {
        black2gray(list);
        link2gclist(list, tl_env.gc_grayagain);

        gc_info("list<%s> barrierback to grayagin", TL_DEBUG_OBJNAME(list));
        tl_gc_display(gc_debug);
    }
}


void tl_gc_mapbarrier(struct tl_map_t* map, const tl_value_t* key, const tl_value_t* val)
{
    if (isblack(map)) {
        if ((tl_isgco(key) && iswhite(key->vv.gco)) || (tl_isgco(val) && iswhite(val->vv.gco))) {
            black2gray(map);
            link2gclist(map, tl_env.gc_grayagain);
        }
    }
}


static tl_size_t gc_onestep(void)
{
    tl_env.gc_processed = 0;
    switch (tl_env.gc_state) {
    case STATE_RESTART:
        gc_info("gc STATE_RESTART");
        gc_restart();
        tl_env.gc_state = STATE_PROPAGATE;
        break;

    case STATE_PROPAGATE:
        gc_info("gc STATE_PROPAGATE");
        gc_propagate();
        if (tl_env.gc_gray == nullptr) {
            tl_env.gc_state = STATE_ATOMIC;
        }
        break;

    case STATE_ATOMIC:
        gc_info("gc STATE_ATOMIC");
        gc_atomic();
        tl_env.gc_state = STATE_SWEEP;
        break;

    case STATE_SWEEP:
        gc_info("gc STATE_SWEEP");
        gc_sweep();
        if (tl_env.gc_sweepptr == nullptr) {
            tl_env.gc_state = STATE_RESTART;
        }
        break;

    default:
        break;
    }
    return tl_env.gc_processed;
}


static void gc_restart(void)
{
    // mark gc start point
    tl_env.gc_gray = tl_env.gc_grayagain = nullptr;
    markvalue(&tl_env.global_list);   // to-delete
    markvalue(&tl_env.scopes)
}


static void gc_propagate(void)
{
    if (tl_env.gc_gray == nullptr)
        return;
    tl_gcobj_t* gco = tl_env.gc_gray;

    switch (gco->vtype) {
    case TL_TYPE_LIST: {
        // remove from gray list, mark to black, traverse it
        tl_list_t* list = (tl_list_t*)gco;
        tl_env.gc_gray  = list->nextgray;
        gray2black(list);
        gc_info("gc move <%s> from gc_gray, mark to black", TL_DEBUG_OBJNAME(gco));
        tl_gc_display(gc_debug);
        tl_env.gc_processed += gc_traverse_list(list);
    } break;

    case TL_TYPE_MAP: {
        tl_map_t* map  = (tl_map_t*)gco;
        tl_env.gc_gray = map->nextgray;
        gray2black(map);
        tl_env.gc_processed += gc_traverse_map(map);
    } break;

    default:
        // string won't be gray, never reach here
        break;
    }
}


static void gc_atomic(void)
{
    // propagate gc_gray all
    gc_info("gc propagate gc_gray atomicly");

    while (tl_env.gc_gray != nullptr) {
        gc_propagate();
    }

    gc_info("gc propagate gc_grayagain atomicly");
    // propagate grayagin all
    tl_env.gc_gray      = tl_env.gc_grayagain;
    tl_env.gc_grayagain = nullptr;
    while (tl_env.gc_gray != nullptr) {
        gc_propagate();
    }

    tl_env.gc_currentwhite = otherwhite();
    tl_env.gc_sweepptr     = &tl_env.gc_all;   // gc_all may be changed between two step
    gc_info("gc currentwhite changed to %s", color2str(tl_env.gc_currentwhite));
    // TODO: clear string cache
}


static void gc_sweep(void)
{
    // tl_size_t count = TL_GC_STEP_MAX_SWEEP_NUM;

    if (tl_env.gc_sweepptr != nullptr) {
        // sweep gclist
        tl_env.gc_sweepptr = gc_sweeplist(tl_env.gc_sweepptr, TL_GC_STEP_MAX_SWEEP_NUM);

        if (*(tl_env.gc_sweepptr) == nullptr) {
            // sweep really finish
            gc_info("gc sweep finish");
            tl_env.gc_sweepptr = nullptr;
        }
    }
}


static tl_gcobj_t** gc_sweeplist(tl_gcobj_t** pp, tl_size_t count)
{
    tl_byte_t ow = otherwhite();   // the really white in current gc process

    while (*pp != nullptr && count > 0) {
        tl_gcobj_t* gco    = *pp;
        tl_byte_t   marked = gco->marked;

        gc_info("gc obj<%s> is %s, ow is %s", TL_DEBUG_OBJNAME(gco), color2str(marked), color2str(ow));

        // not dead object, or extra string
        if (!(marked & ow) || (gco->vtype == TL_TYPE_STRING && ((tl_string_t*)gco)->extra > 0)) {
            // set to white for next gc process
            gco->marked = tl_env.gc_currentwhite;
            pp          = &(gco->nextgco);
            gc_info("gc mark <%s> to %s", TL_DEBUG_OBJNAME(gco), color2str(tl_env.gc_currentwhite));
        } else {                  // current white, dead object
            *pp = gco->nextgco;   // delete this node in gc_all list
            gc_info("gc obj<%s> removed from gc_all", TL_DEBUG_OBJNAME(gco));
            tl_gc_display(gc_debug);

            tl_env.gc_processed += gc_freeobj(gco);
        }
        count--;
    }

    return pp;
}


static void gc_markobj(tl_gcobj_t* gco)
{
    white2gray(gco);

    // for special types
    switch (gco->vtype) {
    case TL_TYPE_STRING: {
        // immediately mark to black
        gc_info("gc mark <%s> to black", TL_DEBUG_OBJNAME(gco));
        gray2black(gco);
        tl_string_t* str = (tl_string_t*)gco;
        tl_env.gc_processed += tl_string_size(str);
    } break;

    case TL_TYPE_FUNCTION: {
        // immediately mark to black
        gc_info("gc mark <%s> to black", TL_DEBUG_OBJNAME(gco));
        gray2black(gco);
        tl_env.gc_processed += sizeof(tl_func_t);
    } break;

    case TL_TYPE_LIST: {
        // add to gray list
        gc_info("gc mark <%s> to gray", TL_DEBUG_OBJNAME(gco));
        tl_list_t* list = (tl_list_t*)gco;
        link2gclist(list, tl_env.gc_gray);
        tl_gc_display(gc_debug);
    } break;

    case TL_TYPE_MAP: {
        // add to gray list
        gc_info("gc mark <%s> to gray", TL_DEBUG_OBJNAME(gco));
        tl_map_t* map = (tl_map_t*)gco;
        link2gclist(map, tl_env.gc_gray);
        tl_gc_display(gc_debug);
    } break;

    default:
        break;
    }
}


static tl_size_t gc_freeobj(tl_gcobj_t* gco)
{
    tl_size_t size = 0;
    switch (gco->vtype) {
    case TL_TYPE_STRING: {
        tl_string_t* str = (tl_string_t*)gco;
        if (str->extra == 0) {
            size = tl_string_size(str);
            tl_string_free(str);
        }
    } break;

    case TL_TYPE_FUNCTION: {
        size = sizeof(tl_func_t);
        tl_func_free((tl_func_t*)gco);
    } break;

    case TL_TYPE_LIST: {
        tl_list_t* list = (tl_list_t*)gco;
        size            = sizeof(tl_list_t) + sizeof(tl_value_t) * list->capacity;
        tl_list_free(list);
    } break;

    case TL_TYPE_MAP: {
        tl_map_t* map = (tl_map_t*)gco;
        size          = sizeof(tl_map_t) + sizeof(tl_node_t) * map->capacity;
        tl_map_free(map);
    } break;

    default:
        break;
    }
    return size;
}


static tl_size_t gc_traverse_list(tl_list_t* list)
{
    for (tl_size_t i = 0; i < list->size; i++) {
        gc_info("gc traverse list<%s>[%d]", TL_DEBUG_OBJNAME(list), i);
        markvalue(&list->data[i]);
    }
    return sizeof(tl_list_t) + list->size * sizeof(tl_value_t);
}

static tl_size_t gc_traverse_map(tl_map_t* map)
{
    for (tl_node_t* node = map->data; node < map->data + map->capacity; node++) {
        if (node->key.vtype == TL_TYPE_INVALID)
            continue;
        markvalue(&node->key);
        markvalue(&node->value);
    }
    return sizeof(tl_map_t) + map->capacity * sizeof(tl_node_t);
}

#define display_gco(gco)                                                                                               \
    tl_printf("%s<%s>", tl_typename(gco),                                                                              \
              gco->vtype == TL_TYPE_STRING ? ((tl_string_t*)gco)->cstr : TL_DEBUG_OBJNAME(gco))

static void display_gcall(tl_gcobj_t* gco)
{
    bool first = true;
    while (gco != nullptr) {
        first ? first = false : tl_printf(" -> ");
        display_gco(gco);
        gco = gco->nextgco;
    }
}

static void display_gcgray(tl_gcobj_t* gco)
{
    bool first = true;
    while (gco != nullptr) {
        first ? first = false : tl_printf(" -> ");
        display_gco(gco);
        gco = ((tl_container_t*)gco)->nextgray;
    }
}

void tl_gc_display(bool on)
{
    if (!on)
        return;

    tl_printf("GC INFO:\n");
    tl_printf("    state    : %s\n", gcstate_names[tl_env.gc_state]);
    tl_printf("    debt     : %d\n", tl_env.gc_debt);
    tl_printf("    threshold: %d\n", tl_env.gc_threshold);
    tl_printf("    total    : %d\n", tl_env.gc_threshold + tl_env.gc_debt);
    tl_printf("    all      : "), display_gcall(tl_env.gc_all), tl_printf("\n");
    tl_printf("    gray     : "), display_gcgray(tl_env.gc_gray), tl_printf("\n");
    tl_printf("    grayagain: "), display_gcgray(tl_env.gc_grayagain), tl_printf("\n");
    tl_printf("    sweep    : "), display_gcall(tl_env.gc_sweepptr ? *(tl_env.gc_sweepptr) : nullptr), tl_printf("\n");
}
int32_t tl_gc_memused(void)
{
    return tl_env.gc_threshold + tl_env.gc_debt;
}

// fix a gc object, it will never be gc
// void tl_gc_fixobj( tl_gcobj_t* obj) {
//     // remove obj from gc_all
//     // add obj to fixgc
//     obj->nextgco = tl_env.fixgc;
//     tl_env.fixgc   = obj;
//     white2gray(obj);
// }
