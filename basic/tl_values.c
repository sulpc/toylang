#include "tl_values.h"

void tl_printvalue(tl_value_t* v)
{
    if (v == nullptr) {
        tl_printf("<nullptr>");
        return;
    }

    tl_vtype_t vtype = v->vtype;

    switch (vtype) {
    case TL_TYPE_INVALID:
        break;

    case TL_TYPE_TYPE:
        tl_printf("<%s>", tl_type_names[v->vv.t]);
        break;

    case TL_TYPE_NULL:
        tl_printf("null");
        break;

    case TL_TYPE_BOOL:
        tl_printf("%s", v->vv.b ? "true" : "false");
        break;

    case TL_TYPE_INT:
        tl_printf("%d", v->vv.i);
        break;

    case TL_TYPE_FLOAT:
        tl_printf("%f", v->vv.n);
        break;

    case TL_TYPE_STRING: {
        tl_string_t* s = (tl_string_t*)v->vv.gco;
        tl_printf("%s", s->cstr);
    } break;

    case TL_TYPE_LIST: {
        tl_list_t* l = (tl_list_t*)v->vv.gco;
        tl_printf("[");
        for (tl_size_t i = 0; i < l->size; i++) {
            if (i != 0)
                tl_printf(", ");
            tl_printvalue(&l->data[i]);
        }
        tl_printf("]");
    } break;

    case TL_TYPE_MAP: {
        tl_map_t* m     = (tl_map_t*)v->vv.gco;
        bool      first = true;
        tl_printf("{");
        for (tl_node_t* kv = m->data; kv != m->data + m->capacity; kv++) {
            if (kv->key.vtype != TL_TYPE_INVALID) {
                first ? first = false : tl_printf(", ");
                tl_printvalue(&kv->key);
                tl_printf(": ");
                tl_printvalue(&kv->value);
            }
        }
        tl_printf("}");
    } break;

    case TL_TYPE_FUNCTION:
        tl_printf("<.func.>");
        break;

    case TL_TYPE_CFUNCTION:
        tl_printf("<.cfunc.>");
        break;

    case TL_TYPE_REF:
        tl_printf("<.ref.>");
        break;

    default:
        tl_printf("<unknown>");
        break;
    }
}
