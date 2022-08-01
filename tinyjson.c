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

static int tiny_parse_null(tiny_context *c, tiny_value *v)
{
  EXPECT(c, 'n');
  if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
    return TINY_PARSE_INVALID_VALUE;
  c->json += 3;
  v->type = TINY_NULL;
  return TINY_PARSE_OK;
}

static int tiny_parse_value(tiny_context *c, tiny_value *v)
{
  switch (*c->json)
  {
  case 'n':
    return tiny_parse_null(c, v);
  case '\0':
    return TINY_PARSE_EXPECT_VALUE;
  default:
    return TINY_PARSE_INVALID_VALUE;
  }
}

int tiny_parse(tiny_value *v, const char *json)
{
  tiny_context c;
  assert(v != NULL);
  c.json = json;
  v->type = TINY_NULL;
  tiny_parse_whitespace(&c);
  return tiny_parse_value(&c, v);
}

tiny_type tiny_get_type(const tiny_value *v)
{
  assert(v != NULL);
  return v->type;
}
