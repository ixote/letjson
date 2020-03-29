#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#include "letjson.h"

static int main_ret = 0; //测试函数结束状态
static int test_count = 0; //总测试数
static int test_pass = 0; //通过测试数

#define EXPECT_EQ_BASE(equality, expect, actual, format) \
    do  { \
        test_count++;\
        if (equality){\
            test_pass++;\
        } else {\
            fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual);\
            main_ret = 1;\
         }\
    } while(0)

#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect)==(actual), expect, actual, "%d")
#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect)==(actual), expect, actual, "%lf")
#define EXPECT_EQ_STRING(expect, actual, alength) \
    EXPECT_EQ_BASE(sizeof(expect) - 1 == alength && memcmp(expect, actual, alength) == 0, expect, actual, "%s")
#define EXPECT_TRUE(actual) EXPECT_EQ_BASE((actual) != 0, "true", "false", "%s")
#define EXPECT_FALSE(actual) EXPECT_EQ_BASE((actual) == 0, "false", "true", "%s")

#if defined(_MSC_VER)
#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE((expect) == (actual), (size_t)expect, (size_t)actual, "%Iu")
#else
#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE((expect) == (actual), (size_t)expect, (size_t)actual, "%zu")
#endif

#define TEST_ERROR(ERROR,EXPECT ,json)\
    do {\
        let_value v;\
        v.type = LET_FALSE;\
        EXPECT_EQ_INT(ERROR, let_parse(&v, json));\
        EXPECT_EQ_INT(EXPECT, let_get_type(&v));\
    } while(0)
#define TEST_NUMBER(expect, json) \
    do {\
        let_value v;\
        v.type = LET_FALSE;\
        EXPECT_EQ_INT(LET_PARSE_OK, let_parse(&v, json));\
        EXPECT_EQ_INT(LET_NUMBER, let_get_type(&v));\
        EXPECT_EQ_DOUBLE(expect, let_get_number(&v));\
    } while(0)

static void test_parse_null() {
    TEST_ERROR(LET_PARSE_OK,LET_NULL,"null");
}

static void test_parse_true() {
    TEST_ERROR(LET_PARSE_OK,LET_TRUE,"true");
}

static void test_parse_false() {
    TEST_ERROR(LET_PARSE_OK,LET_FALSE,"false");
}

//测试数字
static void test_parse_number() {
    TEST_NUMBER(0.0, "0");
    TEST_NUMBER(0.0, "-0");
    TEST_NUMBER(0.0, "-0.0");
    TEST_NUMBER(1.0, "1");
    TEST_NUMBER(-1.0, "-1");
    TEST_NUMBER(1.5, "1.5");
    TEST_NUMBER(-1.5, "-1.5");
    TEST_NUMBER(3.1416, "3.1416");
    TEST_NUMBER(1E10, "1E10");
    TEST_NUMBER(1e10, "1e10");
    TEST_NUMBER(1E+10, "1E+10");
    TEST_NUMBER(1E-10, "1E-10");
    TEST_NUMBER(-1E10, "-1E10");
    TEST_NUMBER(-1e10, "-1e10");
    TEST_NUMBER(-1E+10, "-1E+10");
    TEST_NUMBER(-1E-10, "-1E-10");
    TEST_NUMBER(1.234E+10, "1.234E+10");
    TEST_NUMBER(1.234E-10, "1.234E-10");
    TEST_NUMBER(0.0, "1e-10000"); /* must underflow */
    /* the smallest number > 1 */
    TEST_NUMBER(1.0000000000000002, "1.0000000000000002");
    /* minimum denormal */
    TEST_NUMBER( 4.9406564584124654e-324, "4.9406564584124654e-324");
    TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
    /* Max subnormal double */
    TEST_NUMBER( 2.2250738585072009e-308, "2.2250738585072009e-308");
    TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
    /* Min normal positive double */
    TEST_NUMBER( 2.2250738585072014e-308, "2.2250738585072014e-308");
    TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
    /* Max double */
    TEST_NUMBER( 1.7976931348623157e+308, "1.7976931348623157e+308");
    TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");
}

#define TEST_STRING(expect, json)\
    do {\
        let_value v;\
        LET_INIT(&v);\
        EXPECT_EQ_INT(LET_PARSE_OK, let_parse(&v, json));\
        EXPECT_EQ_INT(LET_STRING, let_get_type(&v));\
        EXPECT_EQ_STRING(expect, let_get_string(&v), let_get_string_length(&v));\
        LET_SET_NULL(&v);\
    } while(0)

static void test_parse_string() {
    TEST_STRING("", "\"\"");
    TEST_STRING("Hello", "\"Hello\"");
// #if 0
    TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
    TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
// #endif
    TEST_STRING("Hello\0World", "\"Hello\\u0000World\"");
    TEST_STRING("\x24", "\"\\u0024\"");         /* Dollar sign U+0024 */
    TEST_STRING("\xC2\xA2", "\"\\u00A2\"");     /* Cents sign U+00A2 */
    TEST_STRING("\xE2\x82\xAC", "\"\\u20AC\""); /* Euro sign U+20AC */
    TEST_STRING("\xF0\x9D\x84\x9E", "\"\\uD834\\uDD1E\"");  /* G clef sign U+1D11E */
    TEST_STRING("\xF0\x9D\x84\x9E", "\"\\ud834\\udd1e\"");  /* G clef sign U+1D11E */
}


static void test_parse_array() {
    let_value v;
    LET_INIT(&v);
    EXPECT_EQ_INT(LET_PARSE_OK, let_parse(&v, "[ ]"));
    EXPECT_EQ_INT(LET_ARRAY, let_get_type(&v));
    EXPECT_EQ_SIZE_T(0, let_get_array_size(&v));
    LET_SET_NULL(&v);
// #if 0
    LET_INIT(&v);
    EXPECT_EQ_INT(LET_PARSE_OK, let_parse(&v, "[ null , false , true , 123 , \"abc\" ]"));
    EXPECT_EQ_INT(LET_ARRAY, let_get_type(&v));
    EXPECT_EQ_SIZE_T(5, let_get_array_size(&v));
    EXPECT_EQ_INT(LET_NULL,   let_get_type(let_get_array_element(&v, 0)));
    EXPECT_EQ_INT(LET_FALSE,  let_get_type(let_get_array_element(&v, 1)));
    EXPECT_EQ_INT(LET_TRUE,   let_get_type(let_get_array_element(&v, 2)));
    EXPECT_EQ_INT(LET_NUMBER, let_get_type(let_get_array_element(&v, 3)));
    EXPECT_EQ_INT(LET_STRING, let_get_type(let_get_array_element(&v, 4)));
    EXPECT_EQ_DOUBLE(123.0, let_get_number(let_get_array_element(&v, 3)));
    EXPECT_EQ_STRING("abc", let_get_string(let_get_array_element(&v, 4)), let_get_string_length(let_get_array_element(&v, 4)));
    LET_SET_NULL(&v);

    LET_INIT(&v);
    EXPECT_EQ_INT(LET_PARSE_OK, let_parse(&v, "[ [ ] , [ 0 ] , [ 0 , 1 ] , [ 0 , 1 , 2 ] ]"));
    EXPECT_EQ_INT(LET_ARRAY, let_get_type(&v));
    EXPECT_EQ_SIZE_T(4, let_get_array_size(&v));
    for (int i = 0; i < 4; i++) {
        let_value* a = let_get_array_element(&v, i);
        EXPECT_EQ_INT(LET_ARRAY, let_get_type(a));
        EXPECT_EQ_SIZE_T(i, let_get_array_size(a));
        for (int j = 0; j < i; j++) {
            let_value* e = let_get_array_element(a, j);
            EXPECT_EQ_INT(LET_NUMBER, let_get_type(e));
            EXPECT_EQ_DOUBLE((double)j, let_get_number(e));
        }
    }
    LET_SET_NULL(&v);
    // #endif
}

static void test_parse_object() {
    let_value v;
    size_t i;

    LET_INIT(&v);
    EXPECT_EQ_INT(LET_PARSE_OK, let_parse(&v, " { } "));
    EXPECT_EQ_INT(LET_OBJECT, let_get_type(&v));
    EXPECT_EQ_SIZE_T(0, let_get_object_size(&v));
    LET_SET_NULL(&v);

    LET_INIT(&v);
    EXPECT_EQ_INT(LET_PARSE_OK, let_parse(&v,
        " { "
        "\"n\" : null , "
        "\"f\" : false , "
        "\"t\" : true , "
        "\"i\" : 123 , "
        "\"s\" : \"abc\", "
        "\"a\" : [ 1, 2, 3 ],"
        "\"o\" : { \"1\" : 1, \"2\" : 2, \"3\" : 3 }"
        " } "
    ));
    EXPECT_EQ_INT(LET_OBJECT, let_get_type(&v));
    EXPECT_EQ_SIZE_T(7, let_get_object_size(&v));
    EXPECT_EQ_STRING("n", let_get_object_key(&v, 0), let_get_object_key_length(&v, 0));
    EXPECT_EQ_INT(LET_NULL,   let_get_type(let_get_object_value(&v, 0)));
    EXPECT_EQ_STRING("f", let_get_object_key(&v, 1), let_get_object_key_length(&v, 1));
    EXPECT_EQ_INT(LET_FALSE,  let_get_type(let_get_object_value(&v, 1)));
    EXPECT_EQ_STRING("t", let_get_object_key(&v, 2), let_get_object_key_length(&v, 2));
    EXPECT_EQ_INT(LET_TRUE,   let_get_type(let_get_object_value(&v, 2)));
    EXPECT_EQ_STRING("i", let_get_object_key(&v, 3), let_get_object_key_length(&v, 3));
    EXPECT_EQ_INT(LET_NUMBER, let_get_type(let_get_object_value(&v, 3)));
    EXPECT_EQ_DOUBLE(123.0, let_get_number(let_get_object_value(&v, 3)));
    EXPECT_EQ_STRING("s", let_get_object_key(&v, 4), let_get_object_key_length(&v, 4));
    EXPECT_EQ_INT(LET_STRING, let_get_type(let_get_object_value(&v, 4)));
    EXPECT_EQ_STRING("abc", let_get_string(let_get_object_value(&v, 4)), let_get_string_length(let_get_object_value(&v, 4)));
    EXPECT_EQ_STRING("a", let_get_object_key(&v, 5), let_get_object_key_length(&v, 5));
    EXPECT_EQ_INT(LET_ARRAY, let_get_type(let_get_object_value(&v, 5)));
    EXPECT_EQ_SIZE_T(3, let_get_array_size(let_get_object_value(&v, 5)));
    for (i = 0; i < 3; i++) {
        let_value* e = let_get_array_element(let_get_object_value(&v, 5), i);
        EXPECT_EQ_INT(LET_NUMBER, let_get_type(e));
        EXPECT_EQ_DOUBLE(i + 1.0, let_get_number(e));
    }
    EXPECT_EQ_STRING("o", let_get_object_key(&v, 6), let_get_object_key_length(&v, 6));
    {
        let_value* o = let_get_object_value(&v, 6);
        EXPECT_EQ_INT(LET_OBJECT, let_get_type(o));
        for (i = 0; i < 3; i++) {
            let_value* ov = let_get_object_value(o, i);
            EXPECT_TRUE('1' + i == let_get_object_key(o, i)[0]);
            EXPECT_EQ_SIZE_T(1, let_get_object_key_length(o, i));
            EXPECT_EQ_INT(LET_NUMBER, let_get_type(ov));
            EXPECT_EQ_DOUBLE(i + 1.0, let_get_number(ov));
        }
    }
    LET_SET_NULL(&v);
}
static void test_parse_expect_value() {
    TEST_ERROR(LET_PARSE_EXPECT_VALUE, LET_NULL,"");
    TEST_ERROR(LET_PARSE_EXPECT_VALUE, LET_NULL," ");
}


static void test_parse_invalid_value() {
    TEST_ERROR(LET_PARSE_INVALID_VALUE, LET_NULL,"nul");
    TEST_ERROR(LET_PARSE_INVALID_VALUE, LET_NULL,"?");
    TEST_ERROR(LET_PARSE_INVALID_VALUE, LET_NULL,"+0");
    TEST_ERROR(LET_PARSE_ROOT_NOT_SINGULAR, LET_NUMBER,"1A");
    TEST_ERROR(LET_PARSE_INVALID_VALUE, LET_NULL,".123"); /* at least one digit before '.' */
    TEST_ERROR(LET_PARSE_INVALID_VALUE, LET_NULL,"1.");   /* at least one digit after '.' */
    TEST_ERROR(LET_PARSE_INVALID_VALUE, LET_NULL,"INF");
    TEST_ERROR(LET_PARSE_INVALID_VALUE, LET_NULL,"inf");
    TEST_ERROR(LET_PARSE_INVALID_VALUE, LET_NULL,"NAN");
    TEST_ERROR(LET_PARSE_INVALID_VALUE, LET_NULL,"nan");
    // TEST_ERROR(LET_PARSE_INVALID_VALUE, LET_NULL,"1.121e1");
}


static void test_parse_root_not_singular() {
    TEST_ERROR(LET_PARSE_ROOT_NOT_SINGULAR,LET_NULL,"null x");
        /* invalid number */
    TEST_ERROR(LET_PARSE_ROOT_NOT_SINGULAR, LET_NULL, "0123"); /* after zero should be '.' or nothing */
    TEST_ERROR(LET_PARSE_ROOT_NOT_SINGULAR, LET_NULL, "0x0");
    TEST_ERROR(LET_PARSE_ROOT_NOT_SINGULAR, LET_NULL, "0x123");
}

static void test_parse_number_too_big() {
    TEST_ERROR(LET_PARSE_NUMBER_TOO_BIG, LET_NULL, "1e309");
    TEST_ERROR(LET_PARSE_NUMBER_TOO_BIG, LET_NULL, "-1e309");
}

static void test_parse_missing_quotation_mark() {
    TEST_ERROR(LET_PARSE_MISS_QUOTATION_MARK, LET_NULL, "\"");
    TEST_ERROR(LET_PARSE_MISS_QUOTATION_MARK, LET_NULL, "\"abc");
}

static void test_parse_invalid_string_escape() {
// #if 0
    TEST_ERROR(LET_PARSE_INVALID_STRING_ESCAPE, LET_NULL, "\"\\v\"");
    TEST_ERROR(LET_PARSE_INVALID_STRING_ESCAPE, LET_NULL, "\"\\'\"");
    TEST_ERROR(LET_PARSE_INVALID_STRING_ESCAPE, LET_NULL, "\"\\0\"");
    TEST_ERROR(LET_PARSE_INVALID_STRING_ESCAPE, LET_NULL, "\"\\x12\"");
// #endif
}

static void test_parse_invalid_string_char() {
// #if 0
    TEST_ERROR(LET_PARSE_INVALID_STRING_CHAR, LET_NULL, "\"\x01\"");
    TEST_ERROR(LET_PARSE_INVALID_STRING_CHAR, LET_NULL, "\"\x1F\"");
// #endif
}
static void test_parse_invalid_unicode_hex() {
    TEST_ERROR(LET_PARSE_INVALID_UNICODE_HEX, LET_NULL, "\"\\u\"");
    TEST_ERROR(LET_PARSE_INVALID_UNICODE_HEX, LET_NULL, "\"\\u0\"");
    TEST_ERROR(LET_PARSE_INVALID_UNICODE_HEX, LET_NULL, "\"\\u01\"");
    TEST_ERROR(LET_PARSE_INVALID_UNICODE_HEX, LET_NULL, "\"\\u012\"");
    TEST_ERROR(LET_PARSE_INVALID_UNICODE_HEX, LET_NULL, "\"\\u/000\"");
    TEST_ERROR(LET_PARSE_INVALID_UNICODE_HEX, LET_NULL, "\"\\uG000\"");
    TEST_ERROR(LET_PARSE_INVALID_UNICODE_HEX, LET_NULL, "\"\\u0/00\"");
    TEST_ERROR(LET_PARSE_INVALID_UNICODE_HEX, LET_NULL, "\"\\u0G00\"");
    TEST_ERROR(LET_PARSE_INVALID_UNICODE_HEX, LET_NULL, "\"\\u00/0\"");
    TEST_ERROR(LET_PARSE_INVALID_UNICODE_HEX, LET_NULL, "\"\\u00G0\"");
    TEST_ERROR(LET_PARSE_INVALID_UNICODE_HEX, LET_NULL, "\"\\u000/\"");
    TEST_ERROR(LET_PARSE_INVALID_UNICODE_HEX, LET_NULL, "\"\\u000G\"");
}
static void test_parse_invalid_unicode_surrogate() {
    TEST_ERROR(LET_PARSE_INVALID_UNICODE_SURROGATE, LET_NULL, "\"\\uD800\"");
    TEST_ERROR(LET_PARSE_INVALID_UNICODE_SURROGATE, LET_NULL, "\"\\uDBFF\"");
    TEST_ERROR(LET_PARSE_INVALID_UNICODE_SURROGATE, LET_NULL, "\"\\uD800\\\\\"");
    TEST_ERROR(LET_PARSE_INVALID_UNICODE_SURROGATE, LET_NULL, "\"\\uD800\\uDBFF\"");
    TEST_ERROR(LET_PARSE_INVALID_UNICODE_SURROGATE, LET_NULL, "\"\\uD800\\uE000\"");
}

static void test_parse_miss_comma_or_square_bracket() {
// #if 0
    TEST_ERROR(LET_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, LET_NULL, "[1");
    TEST_ERROR(LET_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, LET_NULL, "[1}");
    TEST_ERROR(LET_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, LET_NULL, "[1 2");
    TEST_ERROR(LET_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, LET_NULL, "[[]");
// #endif
}

#define TEST_PARSE_ERROR(error, json)\
    do {\
        let_value v;\
        LET_INIT(&v);\
        v.type = LET_FALSE;\
        EXPECT_EQ_INT(error, let_parse(&v, json));\
        EXPECT_EQ_INT(LET_NULL, let_get_type(&v));\
        LET_SET_NULL(&v);\
    } while(0)

static void test_parse_miss_key() {
    TEST_PARSE_ERROR(LET_PARSE_MISS_KEY, "{:1,");
    TEST_PARSE_ERROR(LET_PARSE_MISS_KEY, "{1:1,");
    TEST_PARSE_ERROR(LET_PARSE_MISS_KEY, "{true:1,");
    TEST_PARSE_ERROR(LET_PARSE_MISS_KEY, "{false:1,");
    TEST_PARSE_ERROR(LET_PARSE_MISS_KEY, "{null:1,");
    TEST_PARSE_ERROR(LET_PARSE_MISS_KEY, "{[]:1,");
    TEST_PARSE_ERROR(LET_PARSE_MISS_KEY, "{{}:1,");
    TEST_PARSE_ERROR(LET_PARSE_MISS_KEY, "{\"a\":1,");
}

static void test_parse_miss_colon() {
    TEST_PARSE_ERROR(LET_PARSE_MISS_COLON,  "{\"a\"}");
    TEST_PARSE_ERROR(LET_PARSE_MISS_COLON,  "{\"a\",\"b\"}");
}

static void test_parse_miss_comma_or_curly_bracket() {
    TEST_PARSE_ERROR(LET_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1");
    TEST_PARSE_ERROR(LET_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1]");
    TEST_PARSE_ERROR(LET_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1 \"b\"");
    TEST_PARSE_ERROR(LET_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":{}");
}

static void test_parse() {
    test_parse_null();
    test_parse_true();
    test_parse_false();
    test_parse_number();
    test_parse_string();
    test_parse_array();
    test_parse_object();
    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
    test_parse_number_too_big();
    test_parse_missing_quotation_mark();
    test_parse_invalid_string_escape();
    test_parse_invalid_string_char();
    test_parse_invalid_unicode_hex();
    test_parse_invalid_unicode_surrogate();
    test_parse_miss_comma_or_square_bracket();
    test_parse_miss_key();
    test_parse_miss_colon();
    test_parse_miss_comma_or_curly_bracket();
}
static void test_access_null() {
    let_value v;
    LET_INIT(&v);
    let_set_string(&v, "a", 1);
    LET_SET_NULL(&v);
    EXPECT_EQ_INT(LET_NULL, let_get_type(&v));
    LET_SET_NULL(&v);
}
static void test_access_boolean() {
    /* \TODO */
    /* Use EXPECT_TRUE() and EXPECT_FALSE() */
    let_value v;
    LET_INIT(&v);
    let_set_string(&v, "a", 1);
    let_set_boolean(&v, 1);
    EXPECT_TRUE(let_get_boolean(&v));
    let_set_boolean(&v, 0);
    EXPECT_FALSE(let_get_boolean(&v));
    LET_SET_NULL(&v);
}

static void test_access_number() {
    /* \TODO */
    let_value v;
    LET_INIT(&v);
    let_set_string(&v, "a", 1);
    let_set_number(&v, 1234.5);
    EXPECT_EQ_DOUBLE(1234.5, let_get_number(&v));
    LET_SET_NULL(&v);
}

static void test_access_string() {
    let_value v;
    LET_INIT(&v);
    let_set_string(&v, "", 0);
    EXPECT_EQ_STRING("", let_get_string(&v), let_get_string_length(&v));
    let_set_string(&v, "Hello", 5);
    EXPECT_EQ_STRING("Hello", let_get_string(&v), let_get_string_length(&v));
    LET_SET_NULL(&v);
}

static void test_access(){
    test_access_null();
    test_access_boolean();
    test_access_number();
    test_access_string();
}
#define TEST_ROUNDTRIP(json)\
    do {\
        let_value v;\
        char* json2;\
        size_t length;\
        LET_INIT(&v);\
        EXPECT_EQ_INT(LET_PARSE_OK, let_parse(&v, json));\
        EXPECT_EQ_INT(LET_STRINGIFY_OK, let_stringify(&v, &json2, &length));\
        EXPECT_EQ_STRING(json, json2, length);\
        LET_SET_NULL(&v);\
        free(json2);\
    } while(0)

static void test_stringify_number() {
    TEST_ROUNDTRIP("0");
    TEST_ROUNDTRIP("-0");
    TEST_ROUNDTRIP("1");
    TEST_ROUNDTRIP("-1");
    TEST_ROUNDTRIP("1.5");
    TEST_ROUNDTRIP("-1.5");
    TEST_ROUNDTRIP("3.25");
    TEST_ROUNDTRIP("1e+20");
    TEST_ROUNDTRIP("1.234e+20");
    TEST_ROUNDTRIP("1.234e-20");

    TEST_ROUNDTRIP("1.0000000000000002"); /* the smallest number > 1 */
    TEST_ROUNDTRIP("4.9406564584124654e-324"); /* minimum denormal */
    TEST_ROUNDTRIP("-4.9406564584124654e-324");
    TEST_ROUNDTRIP("2.2250738585072009e-308");  /* Max subnormal double */
    TEST_ROUNDTRIP("-2.2250738585072009e-308");
    TEST_ROUNDTRIP("2.2250738585072014e-308");  /* Min normal positive double */
    TEST_ROUNDTRIP("-2.2250738585072014e-308");
    TEST_ROUNDTRIP("1.7976931348623157e+308");  /* Max double */
    TEST_ROUNDTRIP("-1.7976931348623157e+308");
}

static void test_stringify_string() {
    TEST_ROUNDTRIP("\"\"");
    TEST_ROUNDTRIP("\"Hello\"");
    TEST_ROUNDTRIP("\"Hello\\nWorld\"");
    TEST_ROUNDTRIP("\"\\\" \\\\ / \\b \\f \\n \\r \\t\"");
    TEST_ROUNDTRIP("\"Hello\\u0000World\"");
}

static void test_stringify_array() {
    TEST_ROUNDTRIP("[]");
    TEST_ROUNDTRIP("[null,false,true,123,\"abc\",[1,2,3]]");
}

static void test_stringify_object() {
    TEST_ROUNDTRIP("{}");
    TEST_ROUNDTRIP("{\"n\":null,\"f\":false,\"t\":true,\"i\":123,\"s\":\"abc\",\"a\":[1,2,3],\"o\":{\"1\":1,\"2\":2,\"3\":3}}");
}

static void test_stringify() {
    TEST_ROUNDTRIP("null");
    TEST_ROUNDTRIP("false");
    TEST_ROUNDTRIP("true");
    test_stringify_number();
    test_stringify_string();
    test_stringify_array();
    test_stringify_object();
}

int main(int agc, const char** agv)
{
    //内存泄露测试
    #ifdef _WINDOWS
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    #endif
    test_parse();
    test_access();
    test_stringify();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass*100.0 / test_count);
    return main_ret;
}

