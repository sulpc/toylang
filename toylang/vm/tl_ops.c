#include "tl_ops.h"
#include "tl_list.h"
#include "tl_map.h"
#include "tl_mem.h"
#include "tl_string.h"
#include "tl_utils.h"

static void tl_binop_compr(tl_token_t op, tl_value_t* a, tl_value_t* b, tl_value_t* ret)
{
    tl_vtype_t atype = a->vtype;
    tl_vtype_t btype = b->vtype;

    ret->vtype = TL_TYPE_BOOL;

    // EQ LT LE
    if (atype == TL_TYPE_TYPE && btype == TL_TYPE_TYPE) {
        if (op == TOKEN_EQ) {
            ret->vv.b = (a->vv.t == b->vv.t);
            return;
        }
    }
    if (atype == TL_TYPE_NULL && btype == TL_TYPE_NULL) {
        if (op == TOKEN_EQ) {
            ret->vv.b = true;
            return;
        }
    }
    if (atype == TL_TYPE_BOOL && btype == TL_TYPE_BOOL) {
        if (op == TOKEN_EQ) {
            ret->vv.b = a->vv.b <= b->vv.b;
            return;
        }
    }

    if (atype == TL_TYPE_INT && btype == TL_TYPE_INT) {
        if (op == TOKEN_EQ) {
            ret->vv.b = a->vv.i == b->vv.i;
            return;
        } else if (op == TOKEN_LT) {
            ret->vv.b = a->vv.i < b->vv.i;
            return;
        } else if (op == TOKEN_LE) {
            ret->vv.b = a->vv.i <= b->vv.i;
            return;
        }
    } else if ((atype == TL_TYPE_INT || atype == TL_TYPE_FLOAT) && (btype == TL_TYPE_INT || btype == TL_TYPE_FLOAT)) {
        float va = (atype == TL_TYPE_INT) ? a->vv.i : a->vv.n;
        float vb = (btype == TL_TYPE_INT) ? b->vv.i : b->vv.n;

        if (op == TOKEN_EQ) {
            ret->vv.b = va == vb;
            return;
        } else if (op == TOKEN_LT) {
            ret->vv.b = va < vb;
            return;
        } else if (op == TOKEN_LE) {
            ret->vv.b = va <= vb;
            return;
        }
    }

    if (atype == TL_TYPE_STRING && btype == TL_TYPE_STRING) {
        // tl_string_eq((tl_string_t*)a->vv.gco, (tl_string_t*)b->vv.gco)
        int r = strcmp(((tl_string_t*)a->vv.gco)->cstr, ((tl_string_t*)b->vv.gco)->cstr);
        if (op == TOKEN_EQ) {
            ret->vv.b = r == 0;
            return;
        } else if (op == TOKEN_LT) {
            ret->vv.b = r < 0;
            return;
        } else if (op == TOKEN_LE) {
            ret->vv.b = r <= 0;
            return;
        }
    }


    // TL_TYPE_LIST:
    // TL_TYPE_MAP:
    // TL_TYPE_FUNCTION:
    // TL_TYPE_CFUNCTION:

    tl_panic(TL_PANIC_TYPE, "binop<%s> of type <%d>vs<%d> not support!", tl_token_strings[op], atype, btype);
}
static void tl_binop_logic(tl_token_t op, tl_value_t* a, tl_value_t* b, tl_value_t* ret)
{
    tl_vtype_t atype = a->vtype;
    tl_vtype_t btype = b->vtype;

    ret->vtype = TL_TYPE_BOOL;

    switch (op) {
    case TOKEN_OR: {
        bool ba   = tl_op_conv2bool(a);
        bool bb   = tl_op_conv2bool(b);
        ret->vv.b = ba || bb;
        return;
    }
    case TOKEN_AND: {
        bool ba   = tl_op_conv2bool(a);
        bool bb   = tl_op_conv2bool(b);
        ret->vv.b = ba && bb;
        return;
    }
    case TOKEN_IS: {
        if (btype == TL_TYPE_TYPE) {
            ret->vv.b = (atype == b->vv.t);
            return;
        }
    }
    case TOKEN_IN:
        if (btype == TL_TYPE_MAP) {
            tl_value_t key;
            key.vtype = atype;
            key.vv    = a->vv;

            if (tl_map_get((tl_map_t*)b->vv.gco, &key)) {
                ret->vv.b = false;
            } else {
                ret->vv.b = true;
            }
            return;
        }
    default:
        break;
    }

    tl_panic(TL_PANIC_TYPE, "binop<%s> of type <%d>vs<%d> not support!", tl_token_strings[op], atype, btype);
}
static void tl_binop_arith(tl_token_t op, tl_value_t* a, tl_value_t* b, tl_value_t* ret)
{
    tl_vtype_t atype = a->vtype;
    tl_vtype_t btype = b->vtype;

    if (atype == TL_TYPE_INT && btype == TL_TYPE_INT) {
        ret->vtype = TL_TYPE_INT;
        switch (op) {
        case TOKEN_BOR:
            ret->vv.i = a->vv.i | b->vv.i;
            return;
        case TOKEN_BXOR:
            ret->vv.i = a->vv.i ^ b->vv.i;
            return;
        case TOKEN_BAND:
            ret->vv.i = a->vv.i & b->vv.i;
            return;
        case TOKEN_BSHL:
            ret->vv.i = a->vv.i << b->vv.i;
            return;
        case TOKEN_BSHR:
            ret->vv.i = a->vv.i >> b->vv.i;
            return;
        case TOKEN_ADD:
            ret->vv.i = a->vv.i + b->vv.i;
            return;
        case TOKEN_SUB:
            ret->vv.i = a->vv.i - b->vv.i;
            return;
        case TOKEN_MUL:
            ret->vv.i = a->vv.i * b->vv.i;
            return;
        case TOKEN_DIV:
            ret->vv.i = a->vv.i / b->vv.i;
            return;
        case TOKEN_MOD:
            ret->vv.i = a->vv.i % b->vv.i;
            return;
        case TOKEN_POW:
            ret->vv.i = 1;
            for (int i = 1; i <= b->vv.i; i++) {
                ret->vv.i *= a->vv.i;
            }
            return;
        default:
            break;
        }
    } else if ((atype == TL_TYPE_INT || atype == TL_TYPE_FLOAT) && (btype == TL_TYPE_INT || btype == TL_TYPE_FLOAT)) {
        float va = (atype == TL_TYPE_INT) ? a->vv.i : a->vv.n;
        float vb = (btype == TL_TYPE_INT) ? b->vv.i : b->vv.n;

        ret->vtype = TL_TYPE_FLOAT;

        switch (op) {
        case TOKEN_ADD:
            ret->vv.n = va + vb;
            return;
        case TOKEN_SUB:
            ret->vv.n = va - vb;
            return;
        case TOKEN_MUL:
            ret->vv.n = va * vb;
            return;
        case TOKEN_DIV:
            ret->vv.n = va / vb;
            return;
        case TOKEN_POW:
            ret->vv.n = powf(va, vb);
            return;
        default:
            break;
        }
    }

    if (atype == TL_TYPE_STRING && btype == TL_TYPE_STRING) {
        if (op == TOKEN_ADD) {
            ret->vtype       = TL_TYPE_STRING;
            tl_string_t* s1  = (tl_string_t*)a->vv.gco;
            tl_string_t* s2  = (tl_string_t*)b->vv.gco;
            char*        buf = tl_new(s1->len + s2->len + 1);
            memcpy(buf, s1->cstr, s1->len);
            memcpy(&buf[s1->len], s2->cstr, s2->len);
            buf[s1->len + s2->len] = '\0';

            ret->vv.gco = (tl_gcobj_t*)tl_string_new(buf);
            tl_free(buf, s1->len + s2->len + 1);
            return;
        }
    }

    tl_panic(TL_PANIC_TYPE, "binop<%s> of type <%d>vs<%d> not support!", tl_token_strings[op], atype, btype);
}

bool tl_op_conv2bool(tl_value_t* v)
{
    tl_vtype_t vtype = v->vtype;

    switch (vtype) {
    case TL_TYPE_NULL:
        return false;
    case TL_TYPE_BOOL:
        return v->vv.b;
    case TL_TYPE_INT:
        return v->vv.i != 0;
    case TL_TYPE_FLOAT:
        return v->vv.n != 0;
    default:
        break;
    }

    tl_panic(TL_PANIC_TYPE, "conv2bool for type <%d> not support!", v->vtype);
    return false;
}

void tl_op_binop(tl_token_t opt, tl_value_t* a, tl_value_t* b, tl_value_t* ret)
{
    if (opt >= TOKEN_BINOP_LOGIC_START && opt <= TOKEN_BINOP_LOGIC_END) {
        tl_binop_logic(opt, a, b, ret);
    } else if (opt >= TOKEN_COMPARE_START && opt <= TOKEN_COMPARE_END) {
        bool reverse = false;
        if (opt == TOKEN_NE) {
            reverse = true;
            opt     = TOKEN_EQ;
        } else if (opt == TOKEN_GE) {
            reverse = true;
            opt     = TOKEN_LT;
        } else if (opt == TOKEN_GT) {
            reverse = true;
            opt     = TOKEN_LE;
        }
        tl_binop_compr(opt, a, b, ret);
        if (reverse) {
            ret->vv.b = !ret->vv.b;
        }
    } else if (opt >= TOKEN_ARITH_START && opt <= TOKEN_ARITH_END) {
        tl_binop_arith(opt, a, b, ret);
    } else {
        tl_panic(TL_PANIC_TYPE, "unsupport binop <%d>!", opt);
    }
}

void tl_op_uniop(tl_token_t opt, tl_value_t* v, tl_value_t* ret)
{
    tl_vtype_t vtype = v->vtype;

    // + - ! ~ #
    switch (opt) {
    // +
    case TOKEN_ADD: {
        if (vtype == TL_TYPE_INT || vtype == TL_TYPE_FLOAT) {
            ret->vtype = vtype;
            ret->vv    = v->vv;
            return;
        }
    } break;
    // -
    case TOKEN_SUB: {
        if (vtype == TL_TYPE_INT) {
            ret->vtype = vtype;
            ret->vv.i  = -v->vv.i;
            return;
        } else if (vtype == TL_TYPE_FLOAT) {
            ret->vtype = vtype;
            ret->vv.n  = -v->vv.n;
            return;
        }
    } break;
    // !
    case TOKEN_NOT: {
        ret->vtype = TL_TYPE_BOOL;
        ret->vv.b  = !tl_op_conv2bool(v);
        return;
    }   // break;
    // ~
    case TOKEN_BNOT: {
        if (vtype == TL_TYPE_INT) {
            ret->vtype = vtype;
            ret->vv.i  = ~v->vv.i;
            return;
        }
    } break;
    // #
    case TOKEN_LEN: {
        if (vtype == TL_TYPE_STRING) {
            ret->vtype = TL_TYPE_INT;
            ret->vv.i  = ((tl_string_t*)v->vv.gco)->len;
            return;
        } else if (vtype == TL_TYPE_LIST) {
            ret->vtype = TL_TYPE_INT;
            ret->vv.i  = ((tl_list_t*)v->vv.gco)->size;
            return;
        } else if (vtype == TL_TYPE_MAP) {
            ret->vtype = TL_TYPE_INT;
            ret->vv.i  = ((tl_map_t*)v->vv.gco)->count;
            return;
        }
    } break;
    default:
        break;
    }

    tl_panic(TL_PANIC_TYPE, "uniop<%s> of type <%d> not support!", tl_token_strings[opt], vtype);
}

void tl_op_compoundassign(tl_token_t opt, tl_value_t* a, tl_value_t* b)
{
    if (a->vtype == TL_TYPE_REF)
        a = a->vv.r;

    tl_vtype_t atype = a->vtype;
    tl_vtype_t btype = b->vtype;

    if (atype == TL_TYPE_INT && btype == TL_TYPE_INT) {
        int ret;

        switch (opt) {
        case TOKEN_SELFADD:   // +=
            a->vv.i += b->vv.i;
            return;
        case TOKEN_SELFSUB:   // -=
            a->vv.i -= b->vv.i;
            return;
        case TOKEN_SELFMUL:   // *=
            a->vv.i *= b->vv.i;
            return;
        case TOKEN_SELFDIV:   // /=
            a->vv.i /= b->vv.i;
            return;
        case TOKEN_SELFPOW:   // **=
            ret = 1;
            for (int i = 1; i < b->vv.i; i++) {
                ret *= a->vv.i;
            }
            a->vv.i = ret;
            return;
        case TOKEN_SELFMOD:   // %=
            a->vv.i %= b->vv.i;
            return;
        case TOKEN_SELFBSHL:   // <<=
            a->vv.i <<= b->vv.i;
            return;
        case TOKEN_SELFBSHR:   // >>=
            a->vv.i >>= b->vv.i;
            return;
        case TOKEN_SELFBAND:   // &=
            a->vv.i &= b->vv.i;
            return;
        case TOKEN_SELFBXOR:   // ^=
            a->vv.i ^= b->vv.i;
            return;
        case TOKEN_SELFBOR:   // |=
            a->vv.i |= b->vv.i;
            return;
        default:
            break;
        }
    } else if ((atype == TL_TYPE_INT || atype == TL_TYPE_FLOAT) && (btype == TL_TYPE_INT || btype == TL_TYPE_FLOAT)) {
        float va = (atype == TL_TYPE_INT) ? a->vv.i : a->vv.n;
        float vb = (btype == TL_TYPE_INT) ? b->vv.i : b->vv.n;

        a->vtype = TL_TYPE_FLOAT;

        switch (opt) {
        case TOKEN_SELFADD:   // +=
            a->vv.n = va + vb;
            return;
        case TOKEN_SELFSUB:   // -=
            a->vv.n = va - vb;
            return;
        case TOKEN_SELFMUL:   // *=
            a->vv.n = va * vb;
            return;
        case TOKEN_SELFDIV:   // /=
            a->vv.n = va / vb;
            return;
        case TOKEN_SELFPOW:   // **=
            a->vv.n = powf(va, vb);
            return;
        default:
            break;
        }
    }

    if (atype == TL_TYPE_STRING && btype == TL_TYPE_STRING) {
        if (opt == TOKEN_SELFADD) {
            tl_string_t* s1  = (tl_string_t*)a->vv.gco;
            tl_string_t* s2  = (tl_string_t*)b->vv.gco;
            char*        buf = tl_new(s1->len + s2->len + 1);
            memcpy(buf, s1->cstr, s1->len);
            memcpy(&buf[s1->len], s2->cstr, s2->len);
            buf[s1->len + s2->len] = '\0';

            a->vv.gco = (tl_gcobj_t*)tl_string_new(buf);
            tl_free(buf, s1->len + s2->len + 1);
            return;
        }
    }

    tl_panic(TL_PANIC_TYPE, "compoundassign<%s> of type <%d>vs<%d> not support!", tl_token_strings[opt], atype, btype);
}

bool tl_op_eq(tl_value_t* a, tl_value_t* b)
{
    tl_vtype_t atype = a->vtype;
    tl_vtype_t btype = b->vtype;

    switch (atype) {
    case TL_TYPE_INVALID:
        return false;
    case TL_TYPE_TYPE:
        return (btype == TL_TYPE_TYPE) ? (a->vv.t == b->vv.t) : false;
    case TL_TYPE_NULL:
        return (btype == TL_TYPE_NULL) ? true : false;
    case TL_TYPE_BOOL:
        return (btype == TL_TYPE_BOOL) ? (a->vv.b == b->vv.b) : false;
    case TL_TYPE_INT:
        if (btype == TL_TYPE_INT)
            return a->vv.i == b->vv.i;
        else if (btype == TL_TYPE_FLOAT)
            return a->vv.i == b->vv.n;
        else
            return false;
    case TL_TYPE_FLOAT:
        if (btype == TL_TYPE_INT)
            return a->vv.n == b->vv.i;
        else if (btype == TL_TYPE_FLOAT)
            return a->vv.n == b->vv.n;
        else
            return false;
    case TL_TYPE_STRING:
        return (btype == TL_TYPE_STRING)   //
                   ? tl_string_eq((tl_string_t*)a->vv.gco, (tl_string_t*)b->vv.gco)
                   : false;
    case TL_TYPE_LIST:
        // TODO
    case TL_TYPE_MAP:
        // TODO
    case TL_TYPE_FUNCTION:
        // TODO
    case TL_TYPE_CFUNCTION:
        // TODO
    default:
        // unreachable
        return false;
    }
}

void tl_op_next(tl_value_t* c, tl_int_t* idx, tl_value_t* k, tl_value_t* v)
{
    if (c->vtype == TL_TYPE_LIST) {
        tl_list_t* l = (tl_list_t*)c->vv.gco;
        if (*idx < 0) {
            tl_panic(TL_PANIC_INTERNAL, "next invalid idx <%d>!", *idx);
        } else if (*idx >= l->size) {
            *idx = 0;
            return;
        } else {
            tl_value_t* vtmp = tl_list_get(l, *idx);
            if (k) {
                k->vtype = TL_TYPE_INT;
                k->vv.i  = *idx;
            }
            v->vtype = vtmp->vtype;
            v->vv    = vtmp->vv;

            *idx += 1;
            return;
        }

    } else if (c->vtype == TL_TYPE_MAP) {
        tl_map_t* m = (tl_map_t*)c->vv.gco;

        if (*idx < 0) {
            tl_panic(TL_PANIC_INTERNAL, "next invalid idx <%d>!", *idx);
        } else if (*idx >= m->capacity) {
            *idx = 0;
        } else {
            while (*idx < m->capacity) {
                if (m->data[*idx].key.vtype != TL_TYPE_INVALID) {
                    k->vtype = m->data[*idx].key.vtype;
                    k->vv    = m->data[*idx].key.vv;
                    v->vtype = m->data[*idx].value.vtype;
                    v->vv    = m->data[*idx].value.vv;
                    break;
                }
                *idx += 1;
            }

            if (*idx == m->capacity) {
                *idx = 0;
            } else {
                *idx += 1;
            }
        }
        return;
    } else if (c->vtype == TL_TYPE_STRING) {
        tl_string_t* ts   = (tl_string_t*)c->vv.gco;
        char         s[2] = {0};
        if (*idx < 0) {
            tl_panic(TL_PANIC_INTERNAL, "next invalid idx <%d>!", *idx);
        } else if (*idx >= ts->len) {
            *idx = 0;
        } else {
            s[0] = ts->cstr[*idx];
            if (k) {
                k->vtype = TL_TYPE_INT;
                k->vv.i  = *idx;
            }
            v->vtype  = TL_TYPE_STRING;
            v->vv.gco = (tl_gcobj_t*)tl_string_new(s);
            *idx += 1;
        }
        return;
    }

    tl_panic(TL_PANIC_TYPE, "next opt not supportted for type<%d>!", c->vtype);
}

void tl_op_setmember(tl_value_t* c, tl_value_t* k, tl_value_t* v)
{
    if (c->vtype == TL_TYPE_LIST) {
        if (k->vtype == TL_TYPE_INT) {
            if (tl_list_set((tl_list_t*)c->vv.gco, k->vv.i, v) == nullptr) {
                tl_printf("tl_list_set invalid");
            }
            return;
        }
    } else if (c->vtype == TL_TYPE_MAP) {
        if (tl_map_set((tl_map_t*)c->vv.gco, k, v) == nullptr) {
            tl_printf("tl_map_set invalid");
        }
        return;
    }

    tl_panic(TL_PANIC_TYPE, "setmember not supportted for k<%d> v<%d>!", c->vtype, k->vtype);
}

void tl_op_getmember(tl_value_t* c, tl_value_t* k, tl_value_t* v)
{
    if (c->vtype == TL_TYPE_LIST) {
        if (k->vtype == TL_TYPE_INT) {
            tl_value_t* t = tl_list_get((tl_list_t*)c->vv.gco, k->vv.i);
            if (t != nullptr) {
                v->vtype = TL_TYPE_REF;
                v->vv.r  = t;
            } else {
                v->vtype = TL_TYPE_NULL;
                v->vv.i  = 0;
            }
            return;
        }
    } else if (c->vtype == TL_TYPE_MAP) {
        tl_value_t* t = tl_map_get((tl_map_t*)c->vv.gco, k);
        if (t != nullptr) {
            v->vtype = TL_TYPE_REF;
            v->vv.r  = t;
        } else {
            v->vtype = TL_TYPE_NULL;
            v->vv.i  = 0;
        }
        return;
    } else if (c->vtype == TL_TYPE_STRING) {
        if (k->vtype == TL_TYPE_INT) {
            tl_string_t* ts   = (tl_string_t*)c->vv.gco;
            int          idx  = 0;
            char         s[2] = {0};

            idx = k->vv.i;
            if (idx < 0) {
                idx += ts->len;
            }

            if (idx < 0 || idx >= ts->len) {
                v->vtype = TL_TYPE_NULL;
                v->vv.i  = 0;
            } else {
                s[0]      = ts->cstr[idx];
                v->vtype  = TL_TYPE_STRING;
                v->vv.gco = (tl_gcobj_t*)tl_string_new(s);
            }

            return;
        }
    }

    tl_panic(TL_PANIC_TYPE, "getmember not supportted for k<%d> v<%d>!", c->vtype, k->vtype);
}
