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
#if 0
    TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
    TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
#endif
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
#if 0
    TEST_ERROR(tiny_PARSE_INVALID_STRING_ESCAPE, "\"\\v\"");
    TEST_ERROR(tiny_PARSE_INVALID_STRING_ESCAPE, "\"\\'\"");
    TEST_ERROR(tiny_PARSE_INVALID_STRING_ESCAPE, "\"\\0\"");
    TEST_ERROR(tiny_PARSE_INVALID_STRING_ESCAPE, "\"\\x12\"");
#endif
}

static void test_parse_invalid_string_char()
{
#if 0
    TEST_ERROR(tiny_PARSE_INVALID_STRING_CHAR, "\"\x01\"");
    TEST_ERROR(tiny_PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
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
  /* \TODO */
  /* Use EXPECT_TRUE() and EXPECT_FALSE() */
}

static void test_access_number()
{
  /* \TODO */
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
  test_parse_number_too_big();
  test_parse_expect_value();
  test_parse_invalid_value();
  test_parse_root_not_singular();
  test_parse_missing_quotation_mark();
  test_parse_invalid_string_escape();
  test_parse_invalid_string_char();

  test_access_null();
  test_access_boolean();
  test_access_number();
  test_access_string();
}

int main()
{
  test_parse();
  printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
  return main_ret;
}
