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

static void test_parse_expect_value()
{
  tiny_value v;

  v.type = TINY_FALSE;
  EXPECT_EQ_INT(TINY_PARSE_EXPECT_VALUE, tiny_parse(&v, ""));
  EXPECT_EQ_INT(TINY_NULL, tiny_get_type(&v));

  v.type = TINY_FALSE;
  EXPECT_EQ_INT(TINY_PARSE_EXPECT_VALUE, tiny_parse(&v, " "));
  EXPECT_EQ_INT(TINY_NULL, tiny_get_type(&v));
}

static void test_parse_invalid_value()
{
  tiny_value v;
  v.type = TINY_FALSE;
  EXPECT_EQ_INT(TINY_PARSE_INVALID_VALUE, tiny_parse(&v, "nul"));
  EXPECT_EQ_INT(TINY_NULL, tiny_get_type(&v));

  v.type = TINY_FALSE;
  EXPECT_EQ_INT(TINY_PARSE_INVALID_VALUE, tiny_parse(&v, "?"));
  EXPECT_EQ_INT(TINY_NULL, tiny_get_type(&v));
}

static void test_parse_root_not_singular()
{
  tiny_value v;
  v.type = TINY_FALSE;
  EXPECT_EQ_INT(TINY_PARSE_ROOT_NOT_SINGULAR, tiny_parse(&v, "null x"));
  EXPECT_EQ_INT(TINY_NULL, tiny_get_type(&v));
}

static void test_parse()
{
  test_parse_true();
  test_parse_false();
  test_parse_null();
  test_parse_expect_value();
  test_parse_invalid_value();
  test_parse_root_not_singular();
}

int main()
{
  test_parse();
  printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
  return main_ret;
}
