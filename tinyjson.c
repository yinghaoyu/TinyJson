#include "tinyjson.h"
#include <assert.h>  // assert()
#include <errno.h>   // errno, ERANGE
#include <math.h>    // HUGE_VAL
#include <stdlib.h>  // NULL, strtod()

#define EXPECT(c, ch)         \
  do                          \
  {                           \
    assert(*c->json == (ch)); \
    c->json++;                \
  } while (0)
#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9')

typedef struct
{
  const char *json;
} tiny_context;

static void tiny_parse_whitespace(tiny_context *c)
{
  const char *p = c->json;
  while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
    p++;
  c->json = p;
}

static int tiny_parse_literal(tiny_context *c, tiny_value *v, const char *literal, tiny_type type)
{
  size_t i;
  EXPECT(c, literal[0]);
  // end with '\0', '\0' ASCII == 0
  for (i = 0; literal[i + 1]; i++)
  {
    if (c->json[i] != literal[i + 1])
    {
      return TINY_PARSE_INVALID_VALUE;
    }
  }
  c->json += i;
  v->type = type;
  return TINY_PARSE_OK;
}

static int tiny_parse_number(tiny_context *c, tiny_value *v)
{
  const char *p = c->json;
  if (*p == '-')  // 负数
  {
    p++;
  }
  if (*p == '0')  // 只有单个0
  {
    p++;
  }
  else
  {
    // 一个 1-9
    if (!ISDIGIT1TO9(*p))
    {
      return TINY_PARSE_INVALID_VALUE;
    }
    // 一个 1-9 再加上任意数量的 digit
    for (p++; ISDIGIT(*p); p++)
    {
    }
  }
  if (*p == '.')
  {
    p++;
    // 小数点后至少应有一个 digit
    if (!ISDIGIT(*p))
    {
      return TINY_PARSE_INVALID_VALUE;
    }
    for (p++; ISDIGIT(*p); p++)
    {
    }
  }
  if (*p == 'e' || *p == 'E')
  {
    // 有指数部分
    p++;
    if (*p == '+' || *p == '-')
    {
      p++;
    }
    if (!ISDIGIT(*p))
    {
      return TINY_PARSE_INVALID_VALUE;
    }
    for (p++; ISDIGIT(*p); p++)
    {
    }
  }
  errno = 0;
  v->n = strtod(c->json, NULL);
  if (errno == ERANGE && (v->n == HUGE_VAL || v->n == -HUGE_VAL))
  {
    return TINY_PARSE_NUMBER_TOO_BIG;
  }
  v->type = TINY_NUMBER;
  c->json = p;
  return TINY_PARSE_OK;
}

static int tiny_parse_value(tiny_context *c, tiny_value *v)
{
  switch (*c->json)
  {
  case 't':
    return tiny_parse_literal(c, v, "true", TINY_TRUE);
  case 'f':
    return tiny_parse_literal(c, v, "false", TINY_FALSE);
  case 'n':
    return tiny_parse_literal(c, v, "null", TINY_NULL);
  case '\0':
    return TINY_PARSE_EXPECT_VALUE;
  default:
    return tiny_parse_number(c, v);
  }
}

int tiny_parse(tiny_value *v, const char *json)
{
  int ret;
  tiny_context c;
  assert(v != NULL);
  c.json = json;
  v->type = TINY_NULL;
  tiny_parse_whitespace(&c);
  if ((ret = tiny_parse_value(&c, v)) == TINY_PARSE_OK)
  {
    tiny_parse_whitespace(&c);
    if (*c.json != '\0')
    {
      v->type = TINY_NULL;
      ret = TINY_PARSE_ROOT_NOT_SINGULAR;
    }
  }
  return ret;
}

tiny_type tiny_get_type(const tiny_value *v)
{
  assert(v != NULL);
  return v->type;
}

double tiny_get_number(const tiny_value *v)
{
  assert(v != NULL && v->type == TINY_NUMBER);
  return v->n;
}
