#ifndef LETJSON_H__
#define LETJSON_H__
#include <stddef.h> /* size_t */
#include <stdio.h>
#ifdef _WINDOWS
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#define LET_INIT(v) do { (v)->type = LET_NULL; } while(0)

#define LET_SET_NULL(v) let_free(v)

// 解析符号集
typedef enum {
    LET_NULL,
    LET_FALSE,
    LET_TRUE,
    LET_NUMBER,
    LET_STRING,
    LET_ARRAY,
    LET_OBJECT,
} let_type;

// 处理状态吗
enum {
    LET_PARSE_OK = 0,
    LET_PARSE_EXPECT_VALUE,
    LET_PARSE_INVALID_VALUE,
    LET_PARSE_ROOT_NOT_SINGULAR,
    LET_PARSE_NUMBER_TOO_BIG,
    LET_PARSE_MISS_QUOTATION_MARK,
    LET_PARSE_INVALID_STRING_ESCAPE,
    LET_PARSE_INVALID_STRING_CHAR,
    LET_PARSE_INVALID_UNICODE_HEX,
    LET_PARSE_INVALID_UNICODE_SURROGATE,
    LET_PARSE_MISS_COMMA_OR_SQUARE_BRACKET,
    LET_PARSE_MISS_KEY,
    LET_PARSE_MISS_COLON,
    LET_PARSE_MISS_COMMA_OR_CURLY_BRACKET,
    LET_STRINGIFY_OK
};
typedef struct let_member let_member;

// 基本解析树
typedef struct _let_value{
    union {

        struct { let_member* m; size_t size, capacity; }o;
        struct { struct _let_value* e; size_t size, capacity;}; /* array */
        struct { char* s; size_t len; };  /* string */
        double n;                          /* number */
    };
    let_type type;
} let_value;

struct let_member {
    char* k; size_t klen;   /* member key string, key string length */
    let_value v;           /* member value */
};
//解析函数
int let_parse(let_value *v,const char* json);

//类型分析函数
let_type let_get_type(const let_value *v);



void let_free(let_value* v);

int let_get_boolean(const let_value* v);
void let_set_boolean(let_value* v, int b);

double let_get_number(const let_value* v);
void let_set_number(let_value* v, double n);

const char* let_get_string(const let_value* v);
size_t let_get_string_length(const let_value* v);
void let_set_string(let_value* v, const char* s, size_t len);



void let_set_array(let_value* v, size_t capacity);
size_t let_get_array_size(const let_value* v);
size_t let_get_array_capacity(const let_value* v);
void let_reserve_array(let_value* v, size_t capacity);
void let_shrink_array(let_value* v);
void let_clear_array(let_value* v);
let_value* let_get_array_element(const let_value* v, size_t index);
let_value* let_pushback_array_element(let_value* v);
void let_popback_array_element(let_value* v);
let_value* let_insert_array_element(let_value* v, size_t index);
void let_erase_array_element(let_value* v, size_t index, size_t count);



size_t let_find_object_index(const let_value* v, const char* key, size_t klen);

void let_set_object(let_value* v, size_t capacity);
size_t let_get_object_size(const let_value* v);
size_t let_get_object_capacity(const let_value* v);
void let_reserve_object(let_value* v, size_t capacity);
void let_shrink_object(let_value* v);
void let_clear_object(let_value* v);
const char* let_get_object_key(const let_value* v, size_t index);
size_t let_get_object_key_length(const let_value* v, size_t index);
let_value* let_get_object_value(const let_value* v, size_t index);
size_t let_find_object_index(const let_value* v, const char* key, size_t klen);
let_value* let_find_object_value(let_value* v, const char* key, size_t klen);
let_value* let_set_object_value(let_value* v, const char* key, size_t klen);
void let_remove_object_value(let_value* v, size_t index);
// jsonencode
int let_stringify(const let_value* v, char** json, size_t* length);
#endif