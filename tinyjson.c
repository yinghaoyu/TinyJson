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
  if (*p == '0')  // 只有单个0，不能有前导0，比如0123
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

// 读取4位16进制数
static const char *tiny_parse_hex4(const char *p, unsigned *u)
{
  int i;
  *u = 0;
  for (i = 0; i < 4; i++)
  {
    char ch = *p++;
    *u <<= 4;
    if (ch >= '0' && ch <= '9')
    {
      *u |= ch - '0';
    }
    else if (ch >= 'A' && ch <= 'F')
    {
      *u |= ch - ('A' - 10);
    }
    else if (ch >= 'a' && ch <= 'f')
    {
      *u |= ch - ('a' - 10);
    }
    else
    {
      return NULL;
    }
  }
  return p;
  // 这种方案会错误接受"\u 123"不合法的JSON，因为 strtol() 会跳过开始的空白
  // 要解决的话，还需要检测第一个字符是否 [0-9A-Fa-f]，或者 !isspace(*p)
  // char *end;
  //*u = (unsigned) strtol(p, &end, 16);
  // return end == p + 4 ? end : NULL;
}

static void tiny_encode_utf8(tiny_context *c, unsigned u)
{
  if (u <= 0x7F)
  {
    // 写进一个 char，为什么要做 x & 0xFF 这种操作呢？
    // 这是因为 u 是 unsigned 类型，一些编译器可能会警告这个转型可能会截断数据
    PUTC(c, u & 0xFF);
  }
  else if (u <= 0x7FF)
  {
    PUTC(c, 0xC0 | ((u >> 6) & 0xFF));
    PUTC(c, 0x80 | (u & 0x3F));
  }
  else if (u <= 0xFFFF)
  {
    PUTC(c, 0xE0 | ((u >> 12) & 0xFF));
    PUTC(c, 0x80 | ((u >> 6) & 0x3F));
    PUTC(c, 0x80 | (u & 0x3F));
  }
  else
  {
    assert(u <= 0x10FFFF);
    PUTC(c, 0xF0 | ((u >> 18) & 0xFF));
    PUTC(c, 0x80 | ((u >> 12) & 0x3F));
    PUTC(c, 0x80 | ((u >> 6) & 0x3F));
    PUTC(c, 0x80 | (u & 0x3F));
  }
}

#define STRING_ERROR(ret) \
  do                      \
  {                       \
    c->top = head;        \
    return ret;           \
  } while (0)

static int tiny_parse_string_raw(tiny_context *c, char **str, size_t *len)
{
  size_t head = c->top;
  unsigned u, u2;
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
      *len = c->top - head;
      *str = tiny_context_pop(c, *len);
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
      case 'u':
        if (!(p = tiny_parse_hex4(p, &u)))
        {
          STRING_ERROR(TINY_PARSE_INVALID_UNICODE_HEX);
        }
        // 如果第一个码点在0xD800 ~ 0xDBFF之间
        if (u >= 0xD800 && u <= 0xDBFF)
        {
          /* surrogate pair */
          // 应该伴随一个 U+DC00 ~ U+DFFF的低级代理项
          if (*p++ != '\\')
          {
            STRING_ERROR(TINY_PARSE_INVALID_UNICODE_SURROGATE);
          }
          if (*p++ != 'u')
          {
            STRING_ERROR(TINY_PARSE_INVALID_UNICODE_SURROGATE);
          }
          if (!(p = tiny_parse_hex4(p, &u2)))
          {
            STRING_ERROR(TINY_PARSE_INVALID_UNICODE_HEX);
          }
          if (u2 < 0xDC00 || u2 > 0xDFFF)
          {
            STRING_ERROR(TINY_PARSE_INVALID_UNICODE_SURROGATE);
          }
          // 计算真实的码点
          u = (((u - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;
        }
        tiny_encode_utf8(c, u);
        break;
      default:
        STRING_ERROR(TINY_PARSE_INVALID_STRING_ESCAPE);
      }
      break;
    case '\0':
      // 不匹配""
      STRING_ERROR(TINY_PARSE_MISS_QUOTATION_MARK);
    default:
      // char 带不带符号，是实现定义的。
      // 如果编译器定义 char 为带符号的话，(unsigned char)ch >= 0x80 的字符，都会变成负数，并产生 tiny_PARSE_INVALID_STRING_CHAR 错误。
      // 我们现时还没有测试 ASCII 以外的字符，所以有没有转型至不带符号都不影响，但开始处理 Unicode 的时候就要考虑了
      if ((unsigned char) ch < 0x20)
      {
        STRING_ERROR(TINY_PARSE_INVALID_STRING_CHAR);
      }
      PUTC(c, ch);  // 把字符进栈
    }
  }
}

static int tiny_parse_string(tiny_context *c, tiny_value *v)
{
  int ret;
  char *s;
  size_t len;
  if ((ret = tiny_parse_string_raw(c, &s, &len)) == TINY_PARSE_OK)
    tiny_set_string(v, s, len);
  return ret;
}

static int tiny_parse_value(tiny_context *c, tiny_value *v);

static int tiny_parse_array(tiny_context *c, tiny_value *v)
{
  size_t size = 0;
  int i, ret;
  EXPECT(c, '[');
  // 解析空白字符
  tiny_parse_whitespace(c);
  if (*c->json == ']')
  {
    // 数组内没有元素
    c->json++;
    v->type = TINY_ARRAY;
    v->u.a.size = 0;
    v->u.a.e = NULL;
    return TINY_PARSE_OK;
  }
  for (;;)
  {
    tiny_value e;
    tiny_init(&e);
    // buggy
    // 因为 tiny_parse_value() 及之下的函数都需要调用 tiny_context_push()
    // 而 tiny_context_push() 在发现栈满了的时候会用 realloc() 扩容。
    // 这时候，我们上层的 e 就会失效

    // tiny_value *e = tiny_context_push(c, sizeof(tiny_value));  // 可能会失效
    // tiny_init(e);
    // size++;
    // if ((ret = tiny_parse_value(c, e)) != tiny_PARSE_OK)  // 可能会realloc()
    //  return ret;

    if ((ret = tiny_parse_value(c, &e)) != TINY_PARSE_OK)
    {
      break;
    }
    // 解析空白字符
    tiny_parse_whitespace(c);
    memcpy(tiny_context_push(c, sizeof(tiny_value)), &e, sizeof(tiny_value));
    size++;
    if (*c->json == ',')
    {
      c->json++;
      // 解析空白字符
      tiny_parse_whitespace(c);
    }
    else if (*c->json == ']')
    {
      // 数组结束
      c->json++;
      v->type = TINY_ARRAY;
      v->u.a.size = size;
      // size 表示的是元素的数量
      size *= sizeof(tiny_value);
      memcpy(v->u.a.e = (tiny_value *) malloc(size), tiny_context_pop(c, size), size);
      return TINY_PARSE_OK;
    }
    else
    {
      // 不匹配 ']'
      ret = TINY_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
      break;
    }
  }
  /* Pop and free values on the stack */
  for (i = 0; i < size; i++)
  {
    tiny_free((tiny_value *) tiny_context_pop(c, sizeof(tiny_value)));
  }
  return ret;
}

static int tiny_parse_object(tiny_context *c, tiny_value *v)
{
  size_t i, size;
  tiny_member m;
  int ret;
  EXPECT(c, '{');
  tiny_parse_whitespace(c);
  if (*c->json == '}')
  {
    c->json++;
    v->type = TINY_OBJECT;
    v->u.o.m = 0;
    v->u.o.size = 0;
    return TINY_PARSE_OK;
  }
  m.k = NULL;
  size = 0;
  for (;;)
  {
    char *str;
    tiny_init(&m.v);
    /* parse key */
    if (*c->json != '"')
    {
      ret = TINY_PARSE_MISS_KEY;
      break;
    }
    if ((ret = tiny_parse_string_raw(c, &str, &m.klen)) != TINY_PARSE_OK)
    {
      break;
    }
    memcpy(m.k = (char *) malloc(m.klen + 1), str, m.klen);
    m.k[m.klen] = '\0';
    /* parse ws colon ws */
    tiny_parse_whitespace(c);
    if (*c->json != ':')
    {
      ret = TINY_PARSE_MISS_COLON;
      break;
    }
    c->json++;
    tiny_parse_whitespace(c);
    /* parse value */
    if ((ret = tiny_parse_value(c, &m.v)) != TINY_PARSE_OK)
    {
      break;
    }
    memcpy(tiny_context_push(c, sizeof(tiny_member)), &m, sizeof(tiny_member));
    size++;
    m.k = NULL; /* ownership is transferred to member on stack */
                /* parse ws [comma | right-curly-brace] ws */
    tiny_parse_whitespace(c);
    if (*c->json == ',')
    {
      c->json++;
      tiny_parse_whitespace(c);
    }
    else if (*c->json == '}')
    {
      size_t s = sizeof(tiny_member) * size;
      c->json++;
      v->type = TINY_OBJECT;
      v->u.o.size = size;
      memcpy(v->u.o.m = (tiny_member *) malloc(s), tiny_context_pop(c, s), s);
      return TINY_PARSE_OK;
    }
    else
    {
      ret = TINY_PARSE_MISS_COMMA_OR_CURLY_BRACKET;
      break;
    }
  }
  /* Pop and free members on the stack */
  free(m.k);
  for (i = 0; i < size; i++)
  {
    tiny_member *m = (tiny_member *) tiny_context_pop(c, sizeof(tiny_member));
    free(m->k);
    tiny_free(&m->v);
  }
  v->type = TINY_NULL;
  return ret;
}

void tiny_free(tiny_value *v)
{
  size_t i;
  assert(v != NULL);
  switch (v->type)
  {
  case TINY_STRING:
    free(v->u.s.s);
    break;
  case TINY_ARRAY:
    for (i = 0; i < v->u.a.size; i++)
    {
      tiny_free(&v->u.a.e[i]);
    }
    free(v->u.a.e);
    break;
  case TINY_OBJECT:
    for (i = 0; i < v->u.o.size; i++)
    {
      free(v->u.o.m[i].k);
      tiny_free(&v->u.o.m[i].v);
    }
    free(v->u.o.m);
    break;
  default:
    break;
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
  case '"':
    return tiny_parse_string(c, v);
  case '[':
    return tiny_parse_array(c, v);
  case '{':
    return tiny_parse_object(c, v);
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
  // valgrind --leak-check=full  ./tinyjson_test
  // catch no free
  tiny_free(v);
  v->type = b ? TINY_TRUE : TINY_FALSE;
}

void tiny_set_number(tiny_value *v, double n)
{
  tiny_free(v);
  v->u.n = n;
  v->type = TINY_NUMBER;
}

size_t tiny_get_array_size(const tiny_value *v)
{
  assert(v != NULL && v->type == TINY_ARRAY);
  return v->u.a.size;
}

tiny_value *tiny_get_array_element(const tiny_value *v, size_t index)
{
  assert(v != NULL && v->type == TINY_ARRAY);
  assert(index < v->u.a.size);
  return &v->u.a.e[index];
}

size_t tiny_get_object_size(const tiny_value *v)
{
  assert(v != NULL && v->type == TINY_OBJECT);
  return v->u.o.size;
}

const char *tiny_get_object_key(const tiny_value *v, size_t index)
{
  assert(v != NULL && v->type == TINY_OBJECT);
  assert(index < v->u.o.size);
  return v->u.o.m[index].k;
}

size_t tiny_get_object_key_length(const tiny_value *v, size_t index)
{
  assert(v != NULL && v->type == TINY_OBJECT);
  assert(index < v->u.o.size);
  return v->u.o.m[index].klen;
}

tiny_value *tiny_get_object_value(const tiny_value *v, size_t index)
{
  assert(v != NULL && v->type == TINY_OBJECT);
  assert(index < v->u.o.size);
  return &v->u.o.m[index].v;
}
