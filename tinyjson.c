#include "tinyjson.h"
#include <assert.h>  // assert()
#include <errno.h>   // errno, ERANGE
#include <math.h>    // HUGE_VAL
#include <stdlib.h>  // NULL, strtod()
#include <string.h>  // memcpy()

#ifndef TINY_PARSE_STACK_INIT_SIZE
#define TINY_PARSE_STACK_INIT_SIZE 256
#endif

#define EXPECT(c, ch)         \
  do                          \
  {                           \
    assert(*c->json == (ch)); \
    c->json++;                \
  } while (0)
#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9')
#define PUTC(c, ch)                                      \
  do                                                     \
  {                                                      \
    *(char *) tiny_context_push(c, sizeof(char)) = (ch); \
  } while (0)

typedef struct
{
  const char *json;
  char *stack;
  size_t size, top;  // size表示栈的容量
} tiny_context;

// 进栈size个字符
static void *tiny_context_push(tiny_context *c, size_t size)
{
  void *ret;
  assert(size > 0);
  if (c->top + size >= c->size)
  {
    if (c->size == 0)
    {
      c->size = TINY_PARSE_STACK_INIT_SIZE;
    }
    while (c->top + size >= c->size)  // 扩容
    {
      c->size += c->size >> 1; /* c->size * 1.5 */
    }
    c->stack = (char *) realloc(c->stack, c->size);
  }
  ret = c->stack + c->top;
  c->top += size;
  return ret;
}

// 出栈size个字符
static void *tiny_context_pop(tiny_context *c, size_t size)
{
  assert(c->top >= size);
  return c->stack + (c->top -= size);
}

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
  v->u.n = strtod(c->json, NULL);
  if (errno == ERANGE && (v->u.n == HUGE_VAL || v->u.n == -HUGE_VAL))
  {
    return TINY_PARSE_NUMBER_TOO_BIG;
  }
  v->type = TINY_NUMBER;
  c->json = p;
  return TINY_PARSE_OK;
}

static int tiny_parse_string(tiny_context *c, tiny_value *v)
{
  size_t head = c->top, len;
  const char *p;
  EXPECT(c, '\"');
  p = c->json;
  for (;;)
  {
    char ch = *p++;
    switch (ch)
    {
    case '\"':
      // 匹配""
      len = c->top - head;
      tiny_set_string(v, (const char *) tiny_context_pop(c, len), len);
      c->json = p;
      return TINY_PARSE_OK;
    case '\\':
      // 转义字符
      switch (*p++)
      {
      case '\"':
        PUTC(c, '\"');
        break;
      case '\\':
        PUTC(c, '\\');
        break;
      case '/':
        PUTC(c, '/');
        break;
      case 'b':
        PUTC(c, '\b');
        break;
      case 'f':
        PUTC(c, '\f');
        break;
      case 'n':
        PUTC(c, '\n');
        break;
      case 'r':
        PUTC(c, '\r');
        break;
      case 't':
        PUTC(c, '\t');
        break;
      default:
        c->top = head;
        return TINY_PARSE_INVALID_STRING_ESCAPE;
      }
      break;
    case '\0':
      // 不匹配""
      c->top = head;  // 重置栈顶
      return TINY_PARSE_MISS_QUOTATION_MARK;
    default:
      // char 带不带符号，是实现定义的。
      // 如果编译器定义 char 为带符号的话，(unsigned char)ch >= 0x80 的字符，都会变成负数，并产生 LEPT_PARSE_INVALID_STRING_CHAR 错误。
      // 我们现时还没有测试 ASCII 以外的字符，所以有没有转型至不带符号都不影响，但开始处理 Unicode 的时候就要考虑了
      if ((unsigned char) ch < 0x20)
      {
        c->top = head;
        return TINY_PARSE_INVALID_STRING_CHAR;
      }
      PUTC(c, ch);  // 把字符进栈
    }
  }
}

void tiny_free(tiny_value *v)
{
  assert(v != NULL);
  if (v->type == TINY_STRING)
  {
    free(v->u.s.s);
  }
  v->type = TINY_NULL;
}

const char *tiny_get_string(const tiny_value *v)
{
  assert(v != NULL && v->type == TINY_STRING);
  return v->u.s.s;
}

size_t tiny_get_string_length(const tiny_value *v)
{
  assert(v != NULL && v->type == TINY_STRING);
  return v->u.s.len;
}

void tiny_set_string(tiny_value *v, const char *s, size_t len)
{
  assert(v != NULL && (s != NULL || len == 0));
  tiny_free(v);
  v->u.s.s = (char *) malloc(len + 1);
  memcpy(v->u.s.s, s, len);
  v->u.s.s[len] = '\0';
  v->u.s.len = len;
  v->type = TINY_STRING;
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
  case '"':
    return tiny_parse_string(c, v);
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
  c.stack = NULL;
  c.size = c.top = 0;
  tiny_init(v);
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
  assert(c.top == 0);
  free(c.stack);
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
  return v->u.n;
}

int tiny_get_boolean(const tiny_value *v)
{
  assert(v != NULL && (v->type == TINY_TRUE || v->type == TINY_FALSE));
  return v->type == TINY_TRUE;
}

void tiny_set_boolean(tiny_value *v, int b)
{
  // valgrind --leak-check=full  ./leptjson_test
  // catch no free
  // tiny_free(v);
  v->type = b ? TINY_TRUE : TINY_FALSE;
}

void tiny_set_number(tiny_value *v, double n)
{
  tiny_free(v);
  v->u.n = n;
  v->type = TINY_NUMBER;
}
