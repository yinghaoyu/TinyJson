#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tinyjson.h"

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

#define EXPECT_EQ_BASE(equality, expect, actual, format)                                                     \
  do                                                                                                         \
  {                                                                                                          \
    test_count++;                                                                                            \
    if (equality)                                                                                            \
      test_pass++;                                                                                           \
    else                                                                                                     \
    {                                                                                                        \
      fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual); \
      main_ret = 1;                                                                                          \
    }                                                                                                        \
  } while (0)

#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")

// 浮点数用 == 比较有问题
#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%.17g")

#define EXPECT_EQ_STRING(expect, actual, alength) EXPECT_EQ_BASE(sizeof(expect) - 1 == (alength) && memcmp(expect, actual, alength) == 0, expect, actual, "%s")

#define EXPECT_TRUE(actual) EXPECT_EQ_BASE((actual) != 0, "true", "false", "%s")

#define EXPECT_FALSE(actual) EXPECT_EQ_BASE((actual) == 0, "false", "true", "%s")

// ANSI C（C89）并没有的 size_t 打印方法，在 C99 则加入了 "%zu"，但 VS2015 中才有，之前的 VC 版本使用非标准的 "%Iu"。因此，上面的代码使用条件编译去区分 VC
// 和其他编译器。虽然这部分不跨平台也不是 ANSI C 标准，但它只在测试程序中，不太影响程序库的跨平台性。
#if defined(_MSC_VER)
#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE((expect) == (actual), (size_t) expect, (size_t) actual, "%Iu")
#else
#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE((expect) == (actual), (size_t) expect, (size_t) actual, "%zu")
#endif

static void test_parse_null()
{
  tiny_value v;
  v.type = TINY_FALSE;
  EXPECT_EQ_INT(TINY_PARSE_OK, tiny_parse(&v, "null"));
  EXPECT_EQ_INT(TINY_NULL, tiny_get_type(&v));
}

static void test_parse_true()
{
  tiny_value v;
  v.type = TINY_FALSE;
  EXPECT_EQ_INT(TINY_PARSE_OK, tiny_parse(&v, "true"));
  EXPECT_EQ_INT(TINY_TRUE, tiny_get_type(&v));
}

static void test_parse_false()
{
  tiny_value v;
  v.type = TINY_TRUE;
  EXPECT_EQ_INT(TINY_PARSE_OK, tiny_parse(&v, "false"));
  EXPECT_EQ_INT(TINY_FALSE, tiny_get_type(&v));
}

#define TEST_NUMBER(expect, json)                       \
  do                                                    \
  {                                                     \
    tiny_value v;                                       \
    EXPECT_EQ_INT(TINY_PARSE_OK, tiny_parse(&v, json)); \
    EXPECT_EQ_INT(TINY_NUMBER, tiny_get_type(&v));      \
    EXPECT_EQ_DOUBLE(expect, tiny_get_number(&v));      \
  } while (0)

static void test_parse_number()
{
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

  TEST_NUMBER(1.0000000000000002, "1.0000000000000002");           /* the smallest number > 1 */
  TEST_NUMBER(4.9406564584124654e-324, "4.9406564584124654e-324"); /* minimum denorm
al */
  TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
  TEST_NUMBER(2.2250738585072009e-308, "2.2250738585072009e-308"); /* Max subnormal double */
  TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
  TEST_NUMBER(2.2250738585072014e-308, "2.2250738585072014e-308"); /* Min normal positive double */
  TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
  TEST_NUMBER(1.7976931348623157e+308, "1.7976931348623157e+308"); /* Max double */
  TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");
}

#define TEST_STRING(expect, json)                                              \
  do                                                                           \
  {                                                                            \
    tiny_value v;                                                              \
    tiny_init(&v);                                                             \
    EXPECT_EQ_INT(TINY_PARSE_OK, tiny_parse(&v, json));                        \
    EXPECT_EQ_INT(TINY_STRING, tiny_get_type(&v));                             \
    EXPECT_EQ_STRING(expect, tiny_get_string(&v), tiny_get_string_length(&v)); \
    tiny_free(&v);                                                             \
  } while (0)

static void test_parse_string()
{
  TEST_STRING("", "\"\"");
  TEST_STRING("Hello", "\"Hello\"");
#if 1
  TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
  TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
#endif
  TEST_STRING("Hello\0World", "\"Hello\\u0000World\"");
  TEST_STRING("\x24", "\"\\u0024\"");                    /* Dollar sign U+0024 */
  TEST_STRING("\xC2\xA2", "\"\\u00A2\"");                /* Cents sign U+00A2 */
  TEST_STRING("\xE2\x82\xAC", "\"\\u20AC\"");            /* Euro sign U+20AC */
  TEST_STRING("\xF0\x9D\x84\x9E", "\"\\uD834\\uDD1E\""); /* G clef sign U+1D11E */
  TEST_STRING("\xF0\x9D\x84\x9E", "\"\\ud834\\udd1e\""); /* G clef sign U+1D11E */
}

static void test_parse_array()
{
  size_t i, j;
  tiny_value v;

  tiny_init(&v);
  EXPECT_EQ_INT(TINY_PARSE_OK, tiny_parse(&v, "[ ]"));
  EXPECT_EQ_INT(TINY_ARRAY, tiny_get_type(&v));
  EXPECT_EQ_SIZE_T(0, tiny_get_array_size(&v));
  tiny_free(&v);

  tiny_init(&v);
  EXPECT_EQ_INT(TINY_PARSE_OK, tiny_parse(&v, "[ null , false , true , 123 , \"abc\" ]"));
  EXPECT_EQ_INT(TINY_ARRAY, tiny_get_type(&v));
  EXPECT_EQ_SIZE_T(5, tiny_get_array_size(&v));
  EXPECT_EQ_INT(TINY_NULL, tiny_get_type(tiny_get_array_element(&v, 0)));
  EXPECT_EQ_INT(TINY_FALSE, tiny_get_type(tiny_get_array_element(&v, 1)));
  EXPECT_EQ_INT(TINY_TRUE, tiny_get_type(tiny_get_array_element(&v, 2)));
  EXPECT_EQ_INT(TINY_NUMBER, tiny_get_type(tiny_get_array_element(&v, 3)));
  EXPECT_EQ_INT(TINY_STRING, tiny_get_type(tiny_get_array_element(&v, 4)));
  EXPECT_EQ_DOUBLE(123.0, tiny_get_number(tiny_get_array_element(&v, 3)));
  EXPECT_EQ_STRING("abc", tiny_get_string(tiny_get_array_element(&v, 4)), tiny_get_string_length(tiny_get_array_element(&v, 4)));
  tiny_free(&v);

  tiny_init(&v);
  EXPECT_EQ_INT(TINY_PARSE_OK, tiny_parse(&v, "[ [ ] , [ 0 ] , [ 0 , 1 ] , [ 0 , 1 , 2 ] ]"));
  EXPECT_EQ_INT(TINY_ARRAY, tiny_get_type(&v));
  EXPECT_EQ_SIZE_T(4, tiny_get_array_size(&v));
  for (i = 0; i < 4; i++)
  {
    tiny_value *a = tiny_get_array_element(&v, i);
    EXPECT_EQ_INT(TINY_ARRAY, tiny_get_type(a));
    EXPECT_EQ_SIZE_T(i, tiny_get_array_size(a));
    for (j = 0; j < i; j++)
    {
      tiny_value *e = tiny_get_array_element(a, j);
      EXPECT_EQ_INT(TINY_NUMBER, tiny_get_type(e));
      EXPECT_EQ_DOUBLE((double) j, tiny_get_number(e));
    }
  }
  tiny_free(&v);
}

#define TEST_ERROR(error, json)                  \
  do                                             \
  {                                              \
    tiny_value v;                                \
    v.type = TINY_FALSE;                         \
    EXPECT_EQ_INT(error, tiny_parse(&v, json));  \
    EXPECT_EQ_INT(TINY_NULL, tiny_get_type(&v)); \
  } while (0)

static void test_parse_expect_value()
{
  TEST_ERROR(TINY_PARSE_EXPECT_VALUE, "");
  TEST_ERROR(TINY_PARSE_EXPECT_VALUE, " ");
}

static void test_parse_invalid_value()
{
  TEST_ERROR(TINY_PARSE_INVALID_VALUE, "nul");
  TEST_ERROR(TINY_PARSE_INVALID_VALUE, "?");
#if 1
  TEST_ERROR(TINY_PARSE_INVALID_VALUE, "+0");
  TEST_ERROR(TINY_PARSE_INVALID_VALUE, "+1");
  TEST_ERROR(TINY_PARSE_INVALID_VALUE, ".123"); /* at least one digit before '.' */
  TEST_ERROR(TINY_PARSE_INVALID_VALUE, "1.");   /* at least one digit after '.' */
  TEST_ERROR(TINY_PARSE_INVALID_VALUE, "INF");
  TEST_ERROR(TINY_PARSE_INVALID_VALUE, "inf");
  TEST_ERROR(TINY_PARSE_INVALID_VALUE, "NAN");
  TEST_ERROR(TINY_PARSE_INVALID_VALUE, "nan");
#endif

  /* invalid value in array */
#if 1
  TEST_ERROR(TINY_PARSE_INVALID_VALUE, "[1,]");
  TEST_ERROR(TINY_PARSE_INVALID_VALUE, "[\"a\", nul]");
#endif
}

static void test_parse_root_not_singular()
{
  TEST_ERROR(TINY_PARSE_ROOT_NOT_SINGULAR, "null x");
#if 1
  /* invalid number */
  TEST_ERROR(TINY_PARSE_ROOT_NOT_SINGULAR, "0123"); /* after zero should be '.' , 'E' , 'e' or nothing */
  TEST_ERROR(TINY_PARSE_ROOT_NOT_SINGULAR, "0x0");
  TEST_ERROR(TINY_PARSE_ROOT_NOT_SINGULAR, "0x123");
#endif
}

static void test_parse_number_too_big()
{
#if 1
  TEST_ERROR(TINY_PARSE_NUMBER_TOO_BIG, "1e309");
  TEST_ERROR(TINY_PARSE_NUMBER_TOO_BIG, "-1e309");
#endif
}

static void test_parse_missing_quotation_mark()
{
  TEST_ERROR(TINY_PARSE_MISS_QUOTATION_MARK, "\"");
  TEST_ERROR(TINY_PARSE_MISS_QUOTATION_MARK, "\"abc");
}

static void test_parse_invalid_string_escape()
{
#if 1
  TEST_ERROR(TINY_PARSE_INVALID_STRING_ESCAPE, "\"\\v\"");
  TEST_ERROR(TINY_PARSE_INVALID_STRING_ESCAPE, "\"\\'\"");
  TEST_ERROR(TINY_PARSE_INVALID_STRING_ESCAPE, "\"\\0\"");
  TEST_ERROR(TINY_PARSE_INVALID_STRING_ESCAPE, "\"\\x12\"");
#endif
}

static void test_parse_invalid_string_char()
{
#if 1
  TEST_ERROR(TINY_PARSE_INVALID_STRING_CHAR, "\"\x01\"");
  TEST_ERROR(TINY_PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
#endif
}

static void test_parse_invalid_unicode_hex()
{
  TEST_ERROR(TINY_PARSE_INVALID_UNICODE_HEX, "\"\\u\"");
  TEST_ERROR(TINY_PARSE_INVALID_UNICODE_HEX, "\"\\u0\"");
  TEST_ERROR(TINY_PARSE_INVALID_UNICODE_HEX, "\"\\u01\"");
  TEST_ERROR(TINY_PARSE_INVALID_UNICODE_HEX, "\"\\u012\"");
  TEST_ERROR(TINY_PARSE_INVALID_UNICODE_HEX, "\"\\u/000\"");
  TEST_ERROR(TINY_PARSE_INVALID_UNICODE_HEX, "\"\\uG000\"");
  TEST_ERROR(TINY_PARSE_INVALID_UNICODE_HEX, "\"\\u0/00\"");
  TEST_ERROR(TINY_PARSE_INVALID_UNICODE_HEX, "\"\\u0G00\"");
  TEST_ERROR(TINY_PARSE_INVALID_UNICODE_HEX, "\"\\u00/0\"");
  TEST_ERROR(TINY_PARSE_INVALID_UNICODE_HEX, "\"\\u00G0\"");
  TEST_ERROR(TINY_PARSE_INVALID_UNICODE_HEX, "\"\\u000/\"");
  TEST_ERROR(TINY_PARSE_INVALID_UNICODE_HEX, "\"\\u000G\"");
}

static void test_parse_invalid_unicode_surrogate()
{
  TEST_ERROR(TINY_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\"");
  TEST_ERROR(TINY_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uDBFF\"");
  TEST_ERROR(TINY_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\\\\"");
  TEST_ERROR(TINY_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uDBFF\"");
  TEST_ERROR(TINY_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uE000\"");
}

static void test_parse_miss_comma_or_square_bracket()
{
#if 1
  TEST_ERROR(TINY_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1");
  TEST_ERROR(TINY_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1}");
  TEST_ERROR(TINY_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1 2");
  TEST_ERROR(TINY_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[[]");
#endif
}

static void test_access_null()
{
  tiny_value v;
  tiny_init(&v);
  tiny_set_string(&v, "a", 1);
  tiny_set_null(&v);
  EXPECT_EQ_INT(TINY_NULL, tiny_get_type(&v));
  tiny_free(&v);
}

static void test_access_boolean()
{
  tiny_value v;
  tiny_init(&v);
  tiny_set_string(&v, "a", 1);
  tiny_set_boolean(&v, 1);
  EXPECT_TRUE(tiny_get_boolean(&v));
  tiny_set_boolean(&v, 0);
  EXPECT_FALSE(tiny_get_boolean(&v));
  tiny_free(&v);
}

static void test_access_number()
{
  tiny_value v;
  tiny_init(&v);
  tiny_set_string(&v, "a", 1);
  tiny_set_number(&v, 1234.5);
  EXPECT_EQ_DOUBLE(1234.5, tiny_get_number(&v));
  tiny_free(&v);
}

static void test_access_string()
{
  tiny_value v;
  tiny_init(&v);
  tiny_set_string(&v, "", 0);
  EXPECT_EQ_STRING("", tiny_get_string(&v), tiny_get_string_length(&v));
  tiny_set_string(&v, "Hello", 5);
  EXPECT_EQ_STRING("Hello", tiny_get_string(&v), tiny_get_string_length(&v));
  tiny_free(&v);
}
static void test_parse()
{
  test_parse_true();
  test_parse_false();
  test_parse_null();
  test_parse_number();
  test_parse_string();
  test_parse_array();
  test_parse_number_too_big();
  test_parse_expect_value();
  test_parse_invalid_value();
  test_parse_root_not_singular();
  test_parse_missing_quotation_mark();
  test_parse_invalid_string_escape();
  test_parse_invalid_string_char();
  test_parse_invalid_unicode_hex();
  test_parse_invalid_unicode_surrogate();
  test_parse_miss_comma_or_square_bracket();
}

static void test_access()
{
  test_access_null();
  test_access_boolean();
  test_access_number();
  test_access_string();
}

int main()
{
  test_parse();
  test_access();
  printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
  return main_ret;
}
