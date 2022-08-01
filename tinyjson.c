#include "tinyjson.h"
#include <assert.h> /* assert() */
#include <stdlib.h> /* NULL */

#define EXPECT(c, ch)         \
  do                          \
  {                           \
    assert(*c->json == (ch)); \
    c->json++;                \
  } while (0)

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

static int tiny_parse_true(tiny_context *c, tiny_value *v)
{
  EXPECT(c, 't');
  if (c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e')
    return TINY_PARSE_INVALID_VALUE;
  c->json += 3;
  v->type = TINY_TRUE;
  return TINY_PARSE_OK;
}

static int tiny_parse_false(tiny_context *c, tiny_value *v)
{
  EXPECT(c, 'f');
  if (c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e')
    return TINY_PARSE_INVALID_VALUE;
  c->json += 4;
  v->type = TINY_FALSE;
  return TINY_PARSE_OK;
}

static int tiny_parse_null(tiny_context *c, tiny_value *v)
{
  EXPECT(c, 'n');
  if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
    return TINY_PARSE_INVALID_VALUE;
  c->json += 3;
  v->type = TINY_NULL;
  return TINY_PARSE_OK;
}

static int tiny_parse_number(tiny_context *c, tiny_value *v)
{
  char *end;
  /* \TODO validate number */
  // string to double
  // return value: number
  // end: first charatcor cannot convert
  v->n = strtod(c->json, &end);
  if (c->json == end)  // cannot convert
    return TINY_PARSE_INVALID_VALUE;
  c->json = end;
  v->type = TINY_NUMBER;
  return TINY_PARSE_OK;
}

static int tiny_parse_value(tiny_context *c, tiny_value *v)
{
  switch (*c->json)
  {
  case 't':
    return tiny_parse_true(c, v);
  case 'f':
    return tiny_parse_false(c, v);
  case 'n':
    return tiny_parse_null(c, v);
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
