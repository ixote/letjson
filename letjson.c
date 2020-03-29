#include "letjson.h"
#include <assert.h>
#include <stdlib.h>
#include <errno.h>   /* errno, ERANGE */
#include <math.h>    /* HUGE_VAL */
#include <string.h>   /* memeset*/

#ifndef LET_PARSE_STACK_INIT_SIZE
#define LET_PARSE_STACK_INIT_SIZE 256
#endif

#ifndef LET_PARSE_STRINGIFY_INIT_SIZE
#define LET_PARSE_STRINGIFY_INIT_SIZE 256
#endif

// 字符头处理宏
#define EXPECT(c, ch) do{ assert(*c->json == (ch));  c->json++;} while(0)

#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
// #define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')

#define PUTC(c, ch) do { *(char*)let_context_push(c, sizeof(char)) = (ch); } while(0)
#define STRING_ERROR(ret) do { c->top = head; return ret; } while(0)

// json 消息内容，处理对象
typedef struct {
    const char *json;
    char* stack;
    size_t size, top;
}let_context;

static int let_parse_value(let_context* c, let_value* v);
// 处理前面的ws部分
static void let_parse_whitespace(let_context* c){
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'){
        p++;
    }
    c->json=p;
}
static int let_parse_expect(let_context* c, let_value* v, int TYPE, char *target){
        EXPECT(c, *target++);
        char chr;
        while ((chr = *target++)){
            if(chr != *c->json++){
                return LET_PARSE_INVALID_VALUE;
            }
        }
        v->type = TYPE;
        return LET_PARSE_OK;
}
// 判断是否属于null类
static int let_parse_null(let_context* c, let_value* v){
    return let_parse_expect(c, v, LET_NULL, "null");
}

static int let_parse_true(let_context* c, let_value* v) {
    return let_parse_expect(c, v, LET_TRUE, "true");
}

static int let_parse_false(let_context* c, let_value* v) {
    return let_parse_expect(c, v, LET_FALSE, "false");
}
//解析数字
static int let_parse_number(let_context* c, let_value* v) {
    char* end;
    /* \TODO validate number */
    // number = [ "-" ] int [ frac ] [ exp ]
    // int = "0" / digit1-9 *digit
    // frac = "." 1*digit
    // exp = ("e" / "E") ["-" / "+"] 1*digit
    //判断内部是否合法
    char *p = c->json;
    if(*p == '0'){
        if(ISDIGIT(*(p+1))||*(p+1)=='X'||*(p+1)=='x'){
            return LET_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    while(ISDIGIT(*(++p)));
    if(*p =='.' ){
        if (!ISDIGIT(*(++p))){
            return LET_PARSE_INVALID_VALUE;
        }
        while(ISDIGIT(*(++p)));
        if (*p == 'e' || *p == 'E') {
            p++;
            if (*p == '+' || *p == '-') {
                p++;
            }
            if (!ISDIGIT(*p)) return LET_PARSE_INVALID_VALUE;
            // while(ISDIGIT(*(++p)));
        }
    }
    // 判断负数
    int symbool =1;
    if(*c->json == '-'){
        c->json++;
        symbool =-1;
    }
    errno = 0;
    v->n= strtod(c->json, &end) * symbool;
    if (errno == ERANGE && (v->n == HUGE_VAL || v->n == -HUGE_VAL))
        return LET_PARSE_NUMBER_TOO_BIG;
    if (c->json == end ){
        return LET_PARSE_INVALID_VALUE;
    }
    c->json = end;
    v->type = LET_NUMBER;
    return LET_PARSE_OK;
}

//字符串解析
void LET_SET_NULL(let_value* v) {
    assert(v != NULL);
    switch (v->type) {
        case LET_STRING:
            free(v->s);
            break;
        case LET_ARRAY:
            for (int i = 0; i < v->size; i++)
                LET_SET_NULL(&v->e[i]);
            free(v->e);
            break;
        case LET_OBJECT:
            for (int i = 0; i < v->o.size; i++) {
                free(v->o.m[i].k);
                LET_SET_NULL(&v->o.m[i].v);
            }
            free(v->o.m);
            break;
        default: break;
    }
    v->type = LET_NULL;
}

static void* let_context_push(let_context* c, size_t size) {
    void* ret;
    assert(size > 0);
    if (c->top + size >= c->size) {
        if (c->size == 0)
            c->size = LET_PARSE_STACK_INIT_SIZE;
        while (c->top + size >= c->size)
            c->size += c->size >> 1;  /* c->size * 1.5 */
        c->stack = (char*)realloc(c->stack, c->size);
    }
    ret = c->stack + c->top;
    c->top += size;
    return ret;
}

static void* let_context_pop(let_context* c, size_t size) {
    assert(c->top >= size);
    return c->stack + (c->top -= size);
}
static const char* let_parse_hex4(const char* p, unsigned* u) {
    *u = 0;
    int i;
    for(i=0; i<4; i++){
        *u <<= 4;
        switch (*(p+i)){
            case '0' ... '9':
                *u |= *(p+i) - '0';
                break;
            case 'A' ... 'F':
                *u |= *(p+i) - ('A'-10);
                break;
            case 'a' ... 'f':
                *u |= *(p+i) - ('a'-10);
                break;
            default:
                return NULL;
        }
    }
    return p+4;
}

static void let_encode_utf8(let_context* c, unsigned u) {
       if (u <= 0x7F) 
        PUTC(c, u & 0xFF);
    else if (u <= 0x7FF) {
        PUTC(c, 0xC0 | ((u >> 6) & 0xFF));
        PUTC(c, 0x80 | ( u       & 0x3F));
    }
    else if (u <= 0xFFFF) {
        PUTC(c, 0xE0 | ((u >> 12) & 0xFF));
        PUTC(c, 0x80 | ((u >>  6) & 0x3F));
        PUTC(c, 0x80 | ( u        & 0x3F));
    }
    else {
        assert(u <= 0x10FFFF);
        PUTC(c, 0xF0 | ((u >> 18) & 0xFF));
        PUTC(c, 0x80 | ((u >> 12) & 0x3F));
        PUTC(c, 0x80 | ((u >>  6) & 0x3F));
        PUTC(c, 0x80 | ( u        & 0x3F));
    }
    /* \TODO */
}
static int let_parse_string_raw(let_context* c, char** str, size_t* len) {
    /* \todo */
    size_t head = c->top;
    unsigned u, u2;
    const char* p;
    EXPECT(c, '\"');
    p = c->json;
    while(1) {
        char ch = *p++;
        switch (ch) {
            case '\"':
                *len = c->top - head;
                *str =  let_context_pop(c, *len);
                c->json = p;
                return LET_PARSE_OK;
            case '\\':
                switch (*p++) {
                    case '\"': PUTC(c, '\"'); break;
                    case '\\': PUTC(c, '\\'); break;
                    case '/':  PUTC(c, '/' ); break;
                    case 'b':  PUTC(c, '\b'); break;
                    case 'f':  PUTC(c, '\f'); break;
                    case 'n':  PUTC(c, '\n'); break;
                    case 'r':  PUTC(c, '\r'); break;
                    case 't':  PUTC(c, '\t'); break;
                    case 'u':
                        if (!(p = let_parse_hex4(p, &u))){
                            STRING_ERROR(LET_PARSE_INVALID_UNICODE_HEX);
                        }
                        if (u >= 0xD800 && u <= 0xDBFF) {
                            if (*p++ != '\\')
                                STRING_ERROR(LET_PARSE_INVALID_UNICODE_SURROGATE);
                            if (*p++ != 'u')
                                STRING_ERROR(LET_PARSE_INVALID_UNICODE_SURROGATE);
                            if (!(p = let_parse_hex4(p, &u2)))
                                STRING_ERROR(LET_PARSE_INVALID_UNICODE_HEX);
                            if (u2 < 0xDC00 || u2 > 0xDFFF)
                                STRING_ERROR(LET_PARSE_INVALID_UNICODE_SURROGATE);
                            u = (((u - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;
                        }
                        let_encode_utf8(c, u);
                        break;
                    default:
                        STRING_ERROR(LET_PARSE_INVALID_STRING_ESCAPE);
                } 
                break;
            case '\0':
                STRING_ERROR(LET_PARSE_MISS_QUOTATION_MARK);
            default:
                if ((unsigned char)ch < 0x20) { 
                    STRING_ERROR(LET_PARSE_INVALID_STRING_CHAR);
                }
                PUTC(c, ch);
        }
    }
}

static int let_parse_string(let_context* c, let_value* v) {
    // string = quotation-mark *char quotation-mark
    // char = unescaped /
    // escape (
    //     %x22 /          ; "    quotation mark  U+0022
    //     %x5C /          ; \    reverse solidus U+005C
    //     %x2F /          ; /    solidus         U+002F
    //     %x62 /          ; b    backspace       U+0008
    //     %x66 /          ; f    form feed       U+000C
    //     %x6E /          ; n    line feed       U+000A
    //     %x72 /          ; r    carriage return U+000D
    //     %x74 /          ; t    tab             U+0009
    //     %x75 4HEXDIG )  ; uXXXX                U+XXXX
    // escape = %x5C          ; \
    // quotation-mark = %x22  ; "
    // unescaped = %x20-21 / %x23-5B / %x5D-10FFFF
    int ret;
    char* s;
    size_t len;
    if ((ret = let_parse_string_raw(c, &s, &len)) == LET_PARSE_OK)
        let_set_string(v, s, len);
    return ret;
}


//解析 array
static int let_parse_array(let_context* c, let_value* v) {
    size_t size = 0;
    int ret;
    EXPECT(c, '[');
    let_parse_whitespace(c);
    if (*c->json == ']') {
        c->json++;
        v->type = LET_ARRAY;
        v->size = 0;
        v->e = NULL;
        return LET_PARSE_OK;
    }
    while(1){
        let_value e;
        LET_INIT(&e);
        let_parse_whitespace(c);
        if ((ret = let_parse_value(c, &e)) != LET_PARSE_OK)
            return ret;
        memcpy((let_value*)let_context_push(c, sizeof(let_value)), &e, sizeof(let_value));//?
        size++;
        let_parse_whitespace(c);
        if (*c->json == ','){
            c->json++;
            let_parse_whitespace(c);
        }
        else if (*c->json == ']') {
            c->json++;
            v->type = LET_ARRAY;
            v->size = size;
            size *= sizeof(let_value);
            memcpy(v->e = (let_value*)malloc(size), let_context_pop(c, size), size);
            return LET_PARSE_OK;
            break;
        }
        else{
            ret = LET_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
            break;
        }
    }
    for (int i = 0; i < size; i++)
        LET_SET_NULL((let_value*)let_context_pop(c, sizeof(let_value)));
    return ret;
}
static int let_parse_object(let_context* c, let_value* v) {
    size_t size;
    let_member m;
    int ret;
    EXPECT(c, '{');
    let_parse_whitespace(c);
    if (*c->json == '}') {
        c->json++;
        v->type = LET_OBJECT;
        v->o.m = 0;
        v->o.size = 0;
        return LET_PARSE_OK;
    }
    m.k = NULL;
    size = 0;
    while(1) {
        char *str;
        LET_INIT(&m.v);
        if (*c->json != '"') {
            ret = LET_PARSE_MISS_KEY;
            break;
        }
        if ((ret = let_parse_string_raw(c, &str, &m.klen)) != LET_PARSE_OK)
            break;
        memcpy(m.k = (char*)malloc(m.klen + 1), str, m.klen);
        m.k[m.klen] = '\0';
        let_parse_whitespace(c);
        if (*c->json != ':') {
            ret = LET_PARSE_MISS_COLON;
            break;
        }
        c->json++;
        let_parse_whitespace(c);
        if ((ret = let_parse_value(c, &m.v)) != LET_PARSE_OK)
            break;
        memcpy(let_context_push(c, sizeof(let_member)), &m, sizeof(let_member));
        size++;
        m.k = NULL; /* ownership is transferred to member on stack */
        /* \todo parse ws [comma | right-curly-brace] ws */
        let_parse_whitespace(c);
        if (*c->json == ',') {
            c->json++;
            let_parse_whitespace(c);
        }
        else if (*c->json == '}') {
            size_t s = sizeof(let_member) * size;
            c->json++;
            v->type = LET_OBJECT;
            v->o.size = size;
            memcpy(v->o.m = (let_member*)malloc(s), let_context_pop(c, s), s);
            return LET_PARSE_OK;
        }
        else {
            ret = LET_PARSE_MISS_COMMA_OR_CURLY_BRACKET;
            break;
        }
    }
    free(m.k);
    for (int i = 0; i < size; i++) {
        let_member* m = (let_member*)let_context_pop(c, sizeof(let_member));
        free(m->k);
        LET_SET_NULL(&m->v);
    }
    v->type = LET_NULL;
    /* \todo Pop and free members on the stack */
    return ret;
}

// 对于每个node的解析函数
static int let_parse_value(let_context* c, let_value* v){
    switch(*c->json){
        case 'f': return let_parse_false(c, v); 
        case 't': return let_parse_true(c, v); 
        case 'n': return let_parse_null(c, v); 
        case  '-': return let_parse_number(c, v);
        case '0'...'9' : return let_parse_number(c, v);
        case '\"':  return let_parse_string(c, v);
        case '[': return let_parse_array(c, v);
        case '{':  return let_parse_object(c, v);
        case '\0': return LET_PARSE_EXPECT_VALUE; 
        default: return LET_PARSE_INVALID_VALUE; 
    }
}

// 对于一段json的解析函数
int let_parse(let_value* v, const char* json){
    let_context c;
    assert(v != NULL);
    c.json = json;
    c.stack = NULL;        /* <- */
    c.size = c.top = 0;    /* <- */
    LET_INIT(v);
    v->type = LET_NULL;
    let_parse_whitespace(&c);
    int ret = let_parse_value(&c ,v);
    if(ret == LET_PARSE_OK){    
        let_parse_whitespace(&c);
        if(*c.json != '\0' ){
            ret = LET_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    // assert(c.top == 0);    /* <- */
    free(c.stack);         /* <- */

    return ret;
}

// 获取node类型
let_type let_get_type(const let_value* v) {
    assert(v != NULL);
    return v->type;
}
int let_get_boolean(const let_value* v) {
    /* \TODO */
    assert(v != NULL &&( v->type == LET_TRUE || v->type == LET_FALSE));
    return v->n;
}

void let_set_boolean(let_value* v, int b) {
    /* \TODO */
    assert(v!=NULL);
    LET_INIT(v);
    v->n = b;
    v->type = b ? LET_TRUE : LET_FALSE;
}

double let_get_number(const let_value* v) {
    assert(v != NULL && v->type == LET_NUMBER);
    return v->n;
}

void let_set_number(let_value* v, double n) {
    /* \TODO */
    assert(v!=NULL);
    LET_INIT(v);
    v->n = n;
    v->type = LET_NUMBER;
}

const char* let_get_string(const let_value* v) {
    assert(v != NULL && v->type == LET_STRING);
    return v->s;
}

size_t let_get_string_length(const let_value* v) {
    assert(v != NULL && v->type == LET_STRING);
   
    return v->len;
}

void let_set_string(let_value* v, const char* s, size_t len) {
    assert(v != NULL && (s != NULL || len == 0));
    LET_INIT(v);
    v->s = (char*)malloc(len + 1);
    memcpy(v->s, s, len);
    v->s[len] = '\0';
    v->len = len;
    v->type = LET_STRING;
}


size_t let_get_array_size(const let_value* v) {
    assert(v != NULL && v->type == LET_ARRAY);
    return v->size;
}

#define PUTS(c, s, len)     memcpy(let_context_push(c, len), s, len)
#if 0
static void let_stringify_string(let_context* c, const char* s, size_t len) {
    size_t i;
    assert(s != NULL);
    PUTC(c, '"');
    for (i = 0; i < len; i++) {
        unsigned char ch = (unsigned char)s[i];
        switch (ch) {
            case '\"': PUTS(c, "\\\"", 2); break;
            case '\\': PUTS(c, "\\\\", 2); break;
            case '\b': PUTS(c, "\\b",  2); break;
            case '\f': PUTS(c, "\\f",  2); break;
            case '\n': PUTS(c, "\\n",  2); break;
            case '\r': PUTS(c, "\\r",  2); break;
            case '\t': PUTS(c, "\\t",  2); break;
            default:
                if (ch < 0x20) {
                    char buffer[7];
                    sprintf(buffer, "\\u%04X", ch);
                    PUTS(c, buffer, 6);
                }
                else
                    PUTC(c, s[i]);
        }
    }
    PUTC(c, '"');
}
#else
static void let_stringify_string(let_context* c, const char* s, size_t len) {
    static const char hex_digits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    size_t i, size;
    char* head, *p;
    assert(s != NULL);
    p = head = let_context_push(c, size = len * 6 + 2); /* "\u00xx..." */
    *p++ = '"';
    for (i = 0; i < len; i++) {
        unsigned char ch = (unsigned char)s[i];
        switch (ch) {
            case '\"': *p++ = '\\'; *p++ = '\"'; break;
            case '\\': *p++ = '\\'; *p++ = '\\'; break;
            case '\b': *p++ = '\\'; *p++ = 'b';  break;
            case '\f': *p++ = '\\'; *p++ = 'f';  break;
            case '\n': *p++ = '\\'; *p++ = 'n';  break;
            case '\r': *p++ = '\\'; *p++ = 'r';  break;
            case '\t': *p++ = '\\'; *p++ = 't';  break;
            default:
                if (ch < 0x20) {
                    *p++ = '\\'; *p++ = 'u'; *p++ = '0'; *p++ = '0';
                    *p++ = hex_digits[ch >> 4];
                    *p++ = hex_digits[ch & 15];
                }
                else
                    *p++ = s[i];
        }
    }
    *p++ = '"';
    c->top -= size - (p - head);
}
#endif

static int let_stringify_value(let_context* c, const let_value* v) {
    size_t i;
    switch (v->type) {
        case LET_NULL:   PUTS(c, "null",  4); break;
        case LET_FALSE:  PUTS(c, "false", 5); break;
        case LET_TRUE:   PUTS(c, "true",  4); break;
        case LET_NUMBER:
            c->top -= 32 - sprintf(let_context_push(c, 32), "%.17g", v->n);
            break;
        case LET_STRING: let_stringify_string(c, v->s, v->len); break;
        case LET_ARRAY:
            PUTC(c, '[');
            for (i = 0; i < v->size; i++) {
                if (i > 0)
                    PUTC(c, ',');
                let_stringify_value(c, &v->e[i]);
            }
            PUTC(c, ']');
            break;
        case LET_OBJECT:
            PUTC(c, '{');
            for (i = 0; i < v->o.size; i++) {
                if (i > 0)
                    PUTC(c, ',');
                let_stringify_string(c, v->o.m[i].k, v->o.m[i].klen);
                PUTC(c, ':');
                let_stringify_value(c, &v->o.m[i].v);
            }
            PUTC(c, '}');            break;
        default: assert(0 && "invalid type");
        /* ... */
    }
    return LET_STRINGIFY_OK;
}

int let_stringify(const let_value* v, char** json, size_t* length) {
    let_context c;
    int ret;
    assert(v != NULL);
    assert(json != NULL);
    c.stack = (char*)malloc(c.size = LET_PARSE_STRINGIFY_INIT_SIZE);
    c.top = 0;
    if ((ret = let_stringify_value(&c, v)) != LET_STRINGIFY_OK) {
        free(c.stack);
        *json = NULL;
        return ret;
    }
    if (length)
        *length = c.top;
    PUTC(&c, '\0');
    *json = c.stack;
    return LET_STRINGIFY_OK;
}

#define LET_KEY_NOT_EXIST ((size_t)-1)

size_t let_get_array_capacity(const let_value* v) {
    assert(v != NULL && v->type == LET_ARRAY);
    return v->capacity;
}

void let_reserve_array(let_value* v, size_t capacity) {
    assert(v != NULL && v->type == LET_ARRAY);
    if (v->capacity < capacity) {
        v->capacity = capacity;
        v->e = (let_value*)realloc(v->e, capacity * sizeof(let_value));
    }
}

void let_shrink_array(let_value* v) {
    assert(v != NULL && v->type == LET_ARRAY);
    if (v->capacity > v->size) {
        v->capacity = v->size;
        v->e = (let_value*)realloc(v->e, v->capacity * sizeof(let_value));
    }
}

void let_clear_array(let_value* v) {
    assert(v != NULL && v->type == LET_ARRAY);
    let_erase_array_element(v, 0, v->size);
}

let_value* let_get_array_element(const let_value* v, size_t index) {
    assert(v != NULL && v->type == LET_ARRAY);
    assert(index < v->size);
    return &v->e[index];
}

let_value* let_pushback_array_element(let_value* v) {
    assert(v != NULL && v->type == LET_ARRAY);
    if (v->size == v->capacity)
        let_reserve_array(v, v->capacity == 0 ? 1 : v->capacity * 2);
    LET_INIT(&v->e[v->size]);
    return &v->e[v->size++];
}

void let_popback_array_element(let_value* v) {
    assert(v != NULL && v->type == LET_ARRAY && v->size > 0);
    LET_SET_NULL(&v->e[--v->size]);
}

let_value* let_insert_array_element(let_value* v, size_t index) {
    assert(v != NULL && v->type == LET_ARRAY && index <= v->size);
    /* \todo */
    return NULL;
}

void let_erase_array_element(let_value* v, size_t index, size_t count) {
    assert(v != NULL && v->type == LET_ARRAY && index + count <= v->size);
    /* \todo */
}

void let_set_object(let_value* v, size_t capacity) {
    assert(v != NULL);
    LET_SET_NULL(v);
    v->type = LET_OBJECT;
    v->o.size = 0;
    v->o.capacity = capacity;
    v->o.m = capacity > 0 ? (let_member*)malloc(capacity * sizeof(let_member)) : NULL;
}

size_t let_get_object_size(const let_value* v) {
    assert(v != NULL && v->type == LET_OBJECT);
    return v->o.size;
}

size_t let_get_object_capacity(const let_value* v) {
    assert(v != NULL && v->type == LET_OBJECT);
    /* \todo */
    return 0;
}

void let_reserve_object(let_value* v, size_t capacity) {
    assert(v != NULL && v->type == LET_OBJECT);
    /* \todo */
}

void let_shrink_object(let_value* v) {
    assert(v != NULL && v->type == LET_OBJECT);
    /* \todo */
}

void let_clear_object(let_value* v) {
    assert(v != NULL && v->type == LET_OBJECT);
    /* \todo */
}

const char* let_get_object_key(const let_value* v, size_t index) {
    assert(v != NULL && v->type == LET_OBJECT);
    assert(index < v->o.size);
    return v->o.m[index].k;
}

size_t let_get_object_key_length(const let_value* v, size_t index) {
    assert(v != NULL && v->type == LET_OBJECT);
    assert(index < v->o.size);
    return v->o.m[index].klen;
}

let_value* let_get_object_value(const let_value* v, size_t index) {
    assert(v != NULL && v->type == LET_OBJECT);
    assert(index < v->o.size);
    return &v->o.m[index].v;
}

size_t let_find_object_index(const let_value* v, const char* key, size_t klen) {
    size_t i;
    assert(v != NULL && v->type == LET_OBJECT && key != NULL);
    for (i = 0; i < v->o.size; i++)
        if (v->o.m[i].klen == klen && memcmp(v->o.m[i].k, key, klen) == 0)
            return i;
    return LET_KEY_NOT_EXIST;
}

let_value* let_find_object_value(let_value* v, const char* key, size_t klen) {
    size_t index = let_find_object_index(v, key, klen);
    return index != LET_KEY_NOT_EXIST ? &v->o.m[index].v : NULL;
}

let_value* let_set_object_value(let_value* v, const char* key, size_t klen) {
    assert(v != NULL && v->type == LET_OBJECT && key != NULL);
    /* \todo */
    return NULL;
}

void let_remove_object_value(let_value* v, size_t index) {
    assert(v != NULL && v->type == LET_OBJECT && index < v->o.size);
    /* \todo */
}