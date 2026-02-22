
#include <cfloat>
#include <algorithm>

#include "base/package_api.h"
#include "packages/core/outbuf.h"
#include "packages/core/sprintf.h"  // string_print_formatted

#ifdef F_LET_US_MAKE_A_CRASH
#include <unistd.h>  // for getpid

void f_let_us_make_a_crash(void) {
  size_t p = (size_t)getpid() - (size_t)getpid();
  *(size_t *)p = 0;
}
#endif


#ifdef F_XK_CMDRC4
#include <openssl/rc4.h>

static void hexDump(char const *desc, void const *addr, int len) {
  int i;
  unsigned char buff[17];
  unsigned char *pc = (unsigned char *)addr;

  // Output description if given.
  if (desc != NULL) printf ("%s:\n", desc);
  if (len == 0) {
    printf("  ZERO LENGTH\n");
    return;
  }
  if (len < 0) {
    printf("  NEGATIVE LENGTH: %i\n",len);
    return;
  }

  // Process every byte in the data.
  for (i = 0; i < len; i++) {
    // Multiple of 16 means new line (with line offset).
    if ((i % 16) == 0) {
        // Just don't print ASCII for the zeroth line.
        if (i != 0) printf ("  %s\n", buff);
        // Output the offset.
        printf ("  %04x ", i);
    }

    // Now the hex code for the specific character.
    printf (" %02x", pc[i]);

    // And store a printable ASCII character for later.
    if ((pc[i] < 0x20) || (pc[i] > 0x7e)) buff[i % 16] = '.';
    else buff[i % 16] = pc[i];
    buff[(i % 16) + 1] = '\0';
  }

  // Pad out last line if not exactly 16 characters.
  while ((i % 16) != 0) {
    printf ("   ");
    i++;
  }

  // And print the final ASCII bit.
  printf ("  %s\n", buff);
}

static char *xk_cmdrc4__unescape(const char *p, const char *q, char *o) {
  // \r\n\0 <R><N><0>
  while (p < q) {
    if (q-p >= 3 && p[0] == '<' && p[2] == '>') {
      switch (p[1]) {
        case 'R': *o++ = '\r'; p += 3; continue;
        case 'N': *o++ = '\n'; p += 3; continue;
        case '0': *o++ = '\0'; p += 3; continue;
      }
    }
    *o++ = *p++;
  }
  return o;
}

static char *xk_cmdrc4__filter(char const *p, char const *q, char *o) {
  char *e;
  e = xk_cmdrc4__unescape(p, q, o);
  RC4_KEY real_rc4key;
  // 初始密钥: 12abcdef89 (10字节ASCII)
  RC4_set_key(&real_rc4key, 10, (const unsigned char *)"12abcdef89");
  RC4(&real_rc4key, e-o, (const unsigned char *)o, (unsigned char *)o);
  o = e;
  return o;
}

void f_xk_cmdrc4() {
  const char *arg;
  int arg_len;
  char buf[128];
  char *e;

  arg = sp->u.string;
  arg_len = SVALUE_STRLEN(sp);
  if (arg_len >= sizeof(buf)) arg_len = sizeof(buf) - 1; // just ignore too long input for simplicity

  e = xk_cmdrc4__filter(arg, arg + arg_len, buf);
  *e = 0;

  pop_stack();
  push_malloced_string(string_copy(buf, "f_xk_cmdrc4"));
}
#endif



#ifdef F_STDOUT
void f_stdout(void) {
  int num_arg = st_num_arg;
  FILE *fd = stdout;

  if (num_arg == 1) {
    fputs(sp->u.string, fd);
  } else {
    char *ret = string_print_formatted((sp - num_arg + 1)->u.string, num_arg - 1, sp - num_arg + 2);
    if (ret) {
      fputs(ret, fd);
      FREE_MSTR(ret);
    }
  }

  pop_n_elems(num_arg);
}
#endif

#ifdef F_STDERR
void f_stderr(void) {
  int num_arg = st_num_arg;
  FILE *fd = stderr;

  if (num_arg == 1) {
    fputs(sp->u.string, fd);
  } else {
    char *ret = string_print_formatted((sp - num_arg + 1)->u.string, num_arg - 1, sp - num_arg + 2);
    if (ret) {
      fputs(ret, fd);
      FREE_MSTR(ret);
    }
  }

  pop_n_elems(num_arg);
}
#endif

#ifdef F_FUPTIME
void f_fuptime(void) {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC_RAW, &t);
  double s = (double)t.tv_sec + (double)t.tv_nsec / 1e9;
  static double fuptime_mark;
  if (fuptime_mark == 0) fuptime_mark = s;
  push_real(s - fuptime_mark);
}
#endif

#ifdef F_FTIME
void f_ftime(void) {
  struct timespec t;
  clock_gettime(CLOCK_REALTIME, &t);
  double s = (double)t.tv_sec + (double)t.tv_nsec / 1e9;
  push_real(s);
}
#endif


#ifdef F_SHUFFLE_ARRAY
void f_shuffle_array(void) {
  svalue_t *arg = sp - st_num_arg + 1;
  array_t *tmp = arg->u.arr;
  int num_arg = st_num_arg;

  check_for_destr(tmp);
  tmp = copy_array(tmp);
  std::random_shuffle(tmp->item, tmp->item + tmp->size);
  pop_n_elems(num_arg);
  push_refed_array(tmp);
}
#endif





#define JSON_MAX_SIZE (1024*600)
#define JSON_MAX_DEPTH 25
char *json_stringify_svalue(svalue_t *v, char *o, char *e, int strict_checking);
char const *json_parse_svalue(char const *p, size_t slen, svalue_t *sv);


char *json_stringify_svalue__depth(svalue_t *v, char *o, char *e, int depth, int strict_checking) {
  if (e - o < 1024) goto fail;
  switch (v->type) {
    case T_STRING: {
      char const *s = v->u.string;
      char c;
      *o++ = '\"';
      while ((c = *s++)) {
        if (e - o < 1024) goto fail;
        switch (c) {
          case '\"': *o++ = '\\'; *o++ = '\"'; break;
          //case '\'': *o++ = '\\'; *o++ = '\''; break;
          case '\\': *o++ = '\\'; *o++ = '\\'; break;
          //case  '/': *o++ = '\\'; *o++ = '/'; break;
          case '\b': *o++ = '\\'; *o++ = 'b'; break;
          case '\f': *o++ = '\\'; *o++ = 'f'; break;
          case '\n': *o++ = '\\'; *o++ = 'n'; break;
          case '\r': *o++ = '\\'; *o++ = 'r'; break;
          case '\t': *o++ = '\\'; *o++ = 't'; break;
          default:
            if ((unsigned char)c < 0x20 || c == 0x7f) {
              *o++ = '\\';
              *o++ = 'u';
              *o++ = '0';
              *o++ = '0';
              *o++ = "0123456789abcdef"[(unsigned char)c >> 4];
              *o++ = "0123456789abcdef"[c & 0xf];
            } else {
              *o++ = c;
            }
            break;
        }
      }
      *o++ = '\"';
      *o = 0;
      return o;
    }

    case T_ARRAY: {
      if (++depth > JSON_MAX_DEPTH) error("json_stringify(): Depth overflow.\n");
      size_t n = v->u.arr->size;
      svalue_t *p = v->u.arr->item;
      *o++ = '[';
      for (size_t i = 0; i < n; ++i) {
        if (i) *o++ = ',';
        o = json_stringify_svalue__depth(p+i, o, e, depth, strict_checking);
      }
      *o++ = ']';
      *o = 0;
      return o;
    }

    case T_NUMBER: {
      long long n = v->u.number;
      unsigned long long u = n;
      char buf[32];
      char *q = buf + sizeof(buf);
      int digits;
      if (n < 0) {
        u = -n;
        *o++ = '-';
      }
      do {
        *--q = u % 10 + '0';
        u /= 10;
      } while (u);
      digits = buf + sizeof(buf) - q;
      memcpy(o, q, digits);
      o += digits;
      *o = 0;
      return o;
    }

    case T_REAL: {
      double n = v->u.real;
      if (n != n) n = 0;
      else if (n > DBL_MAX) n = DBL_MAX;
      else if (n < -DBL_MAX) n = -DBL_MAX;
      // https://en.wikipedia.org/wiki/Double-precision_floating-point_format
      // lua��14��dao��18��ά��˵��15-17
      // �ַ���ת���֣�15��ԭ��ת����
      // ����ת�ַ�����17��ԭ��ת��ȥ
      // ����15��
      o += sprintf(o, "%.15g", n);
      *o = 0;
      return o;
    }

    case T_MAPPING: {
      if (++depth > JSON_MAX_DEPTH) error("json_stringify(): Depth overflow.\n");
      size_t j = v->u.map->table_size;
      mapping_node_t **a = v->u.map->table, *elt;
      size_t sum = 0;

      *o++ = '{';
      do {
        for (elt = a[j]; elt; elt = elt->next) {
          if (elt->values->type != T_STRING) {
            if (strict_checking) error("json_stringify(): Non-string key in mapping: %d.\n", elt->values->type);
            continue;
          }
          if (sum++) *o++ = ',';
          o = json_stringify_svalue__depth(elt->values, o, e, depth, strict_checking);
          *o++ = ':';
          o = json_stringify_svalue__depth(elt->values + 1, o, e, depth, strict_checking);
        }
      } while (j--);
      *o++ = '}';
      *o = 0;
      return o;
    }

    default:
      if (strict_checking) error("json_stringify(): Unsupported value type: 0x%x.\n", v->type);
      strcpy(o, "null");
      o += 4;
      return o;
  }
fail:
  error("json_stringify(): Overflow.\n");
  return NULL;
}

char *json_stringify_svalue(svalue_t *v, char *o, char *e, int strict_checking) {
  return json_stringify_svalue__depth(v, o, e, 0, strict_checking);
}



char const *json_parse__skip_whitespace(char const *p) {
  while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') ++p;
  return p;
}

int json_parse__is_delimiter(char c) {
  return c == 0 || c == ',' || c == ']' || c == ':' || c == '}' || c == ' ' || c == '\r' || c == '\n' || c == '\t';
}

int json_parse__is_digit(char c) {
  return '0' <= c && c <= '9';
}

char const *json_parse__number(char const *p, svalue_t *sv, char const **err) {
  char const *start = p;
  // http://www.json.org/
  // ���ܼӺſ�ͷ
  if (*p == '-') ++p;
  // 0��ͷ����ֻ�ܽӵ�
  if (*p == '0') {
    ++p;
    if (json_parse__is_digit(*p)) {
      *err = "got digit after leading zero";
      return NULL;
    }
  } else {
    if (!json_parse__is_digit(*p)) {
      *err = "leading digit expected";
      return NULL;
    }
    ++p;
    while (json_parse__is_digit(*p)) ++p;
  }
  int got_dot_or_exp = 0;
  // �������ǿ�ѡ�ģ������������и�����
  if (*p == '.') {
    got_dot_or_exp = 1;
    ++p;
    if (!json_parse__is_digit(*p)) {
      *err = "digit expected after dot";
      return NULL;
    }
    ++p;
    while (json_parse__is_digit(*p)) ++p;
  }
  if (*p == 'e' || *p == 'E') {
    got_dot_or_exp = 1;
    ++p;
    // + - �ǿ�ѡ��
    if (*p == '+' || *p == '-') ++p;
    // Ȼ������и�����
    if (!json_parse__is_digit(*p)) {
      *err = "digit expected for exponent";
      return NULL;
    }
    ++p;
    while (json_parse__is_digit(*p)) ++p;
  }

  // ����strtol����stdtod
  // http://en.cppreference.com/w/c/string/byte/strtol
  // http://en.cppreference.com/w/c/string/byte/strtof
  if (!got_dot_or_exp) {
    char *end;
    long long x = strtoll(start, &end, 10);
    if (end != p) {
      *err = "unexpected end of strtol";
      return NULL;
    }
    sv->type = T_NUMBER;
    sv->u.number = x;
    sv->subtype = 0;
  } else {
    char *end;
    double x = strtod(start, &end);
    if (end != p) {
      *err = "unexpected end of strtod";
      return NULL;
    }
    sv->type = T_REAL;
    sv->u.real = x;
  }

  return p;
}

static int json_parse__x(char const *p) {
  if ('0' <= *p && *p <= '9') return *p - '0';
  if ('a' <= *p && *p <= 'f') return *p - 'a' + 10;
  if ('A' <= *p && *p <= 'F') return *p - 'A' + 10;
  return -1;
}

static int json_parse__xx(char const *p) {
  int h, l;
  h = json_parse__x(p++);
  if (h == -1) return -1;
  l = json_parse__x(p++);
  if (l == -1) return -1;
  return (h << 4) + l;
}

char const *json_parse__string(char const *p, svalue_t *sv) {
  char buf[JSON_MAX_SIZE]; // there is JSON_MAX_SIZE to limit input already, so this is enough
  char *o = buf;
  char c;
  char *newstr;
  while ((c = *p++)) {
    switch (c) {
      case '\"': {
        goto finish;
      }
      case '\\': {
        switch ((c = *p++)) {
          case '\"': *o++ = '\"'; break;
          case '\\': *o++ = '\\'; break;
          case '/': *o++ = '/'; break;
          case 'b': *o++ = '\b'; break;
          case 'f': *o++ = '\f'; break;
          case 'n': *o++ = '\n'; break;
          case 'r': *o++ = '\r'; break;
          case 't': *o++ = '\t'; break;
          case 'u': {
            int x;
            if (p[0] == '0' && p[1] == '0') {
              p += 2;
              x = json_parse__xx(p);
              if (x == 0) error("json_parse(): Found \\u0000.\n");
              if (x == -1) error("json_parse(): Invalid escape sequence in \\u00XX, hex of first X is 0x%.2x.\n", p[0]);
              *o++ = x;
              p += 2;
              break;
            }
            // fall through
          }
          default: error("json_parse(): Invalid escape sequence in string, hex is: 0x%.2x.\n", c);
        }
        break;
      }
      default: {
        if ((unsigned char)c < 0x20 || c == 0x7f) error("json_parse(): Control character in string.\n");
        *o++ = c;
        break;
      }
    }
  }

  error("json_parse(): Unfinished string.\n");
  return NULL;
finish:
  newstr = new_string(o-buf, "json_parse");
  memcpy(newstr, buf, o-buf);
  newstr[o-buf] = 0;
  sv->u.string = newstr;
  sv->type = T_STRING;
  sv->subtype = STRING_MALLOC;
  return p;
}

void json_parse__resize_array(svalue_t *v, size_t m) {
  size_t n = v->u.arr->size;
  array_t *oldarr = v->u.arr;
  array_t *newarr = allocate_array(m);
  size_t copy_n = m < n ? m : n;
  // 浅拷贝 items 到新数组（不改变引用计数）
  memcpy(newarr->item, oldarr->item, copy_n * sizeof(svalue_t));
  // 将已拷贝的 items 在旧数组中清零，防止 free_array 时 double-free
  for (size_t i = 0; i < copy_n; i++) {
    oldarr->item[i] = const0u;
  }
  // 安全释放旧数组（缩小时多余的 items 会被正确 free_svalue）
  free_array(oldarr);
  v->u.arr = newarr;
}

char const *json_parse_svalue__depth(char const *p, svalue_t *sv, int depth);

char const *json_parse__array(char const *p, svalue_t *v, int depth) {
  size_t m = 8;
  size_t n = 0;
  v->u.arr = allocate_array(m);
  v->type = T_ARRAY;
  for (;;) {
    p = json_parse__skip_whitespace(p);
    //printf("json_parse__array|%s|\n", p);
    if (*p == ']') break;
    if (n && *p++ != ',') error("json_parse(): Need comma in array.\n");
    if (n == m) json_parse__resize_array(v, (m = n*2));
    p = json_parse_svalue__depth(p, v->u.arr->item + n++, depth);
  }
  // ����Ľ����Ƚϵ�Ч����mudos������̫���й�
  if (n != m) json_parse__resize_array(v, n);
  return ++p;
}

char const *json_parse__mapping(char const *p, svalue_t *v, int depth) {
  size_t n = 0;
  v->u.map = allocate_mapping(4);
  v->type = T_MAPPING;
  for (;;) {
    svalue_t k[1];
    svalue_t *svp;
    p = json_parse__skip_whitespace(p);
    if (*p == '}') break;
    if (n && *p++ != ',') error("json_parse(): Need comma in object.\n");
    p = json_parse__skip_whitespace(p);
    if (*p != '\"') error("json_parse(): Object key must be string, hex is 0x%.2x.\n", *p);
    ++p;
    p = json_parse__string(p, k);
    // ���ˣ����ڵõ�һ���ַ��������ü���Ϊ1��������Cջ��
    // find_for_insert�����ڴ治��ʧ�ܵĻ������k�����ڴ�й©��
    // �����������ֻ������ʵ�֣������ڴ治��Ļ���mudosҲҪ����
    svp = find_for_insert(v->u.map, k, 1);
    // find_for_insert����ַ����������ü��������������Լ��ͷ�һ��
    free_svalue(k, "json_parse");
    p = json_parse__skip_whitespace(p);
    if (*p++ != ':') error("json_parse(): Missing colon.\n");
    p = json_parse_svalue__depth(p, svp, depth);
    ++n;
  }
  return ++p;
}

char const *json_parse_svalue__depth(char const *p, svalue_t *sv, int depth) {
  char c;
  if (++depth > JSON_MAX_DEPTH) error("json_parse(): Depth overflow.\n");
  p = json_parse__skip_whitespace(p);
  switch ((c = *p++)) {
    case '\"': return json_parse__string(p, sv);
    case '[': return json_parse__array(p, sv, depth);
    case '{': return json_parse__mapping(p, sv, depth);
    case '-':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9': {
      char const *err = NULL;
      p = json_parse__number(p-1, sv, &err);
      if (NULL == p) error("json_parse(): Malformed number: %s.\n", err);
//      if (!parse_numeric((char **)&p, c, sv)) error("json_parse(): Invalid number.\n");
//      //printf("after parse_numeric|%s|\n", p);
//      --p;
//      //printf("after revise parse_numeric|%s|\n", p);
      return p;
    }
    case 't':
      if (p[0] == 'r' && p[1] == 'u' && p[2] == 'e' && json_parse__is_delimiter(p[3])) {
        p += 3;
        *sv = const1;
        return p;
      }
      goto fail;
    case 'f':
      if (p[0] == 'a' && p[1] == 'l' && p[2] == 's' && p[3] == 'e' && json_parse__is_delimiter(p[4])) {
        p += 4;
        *sv = const0;
        return p;
      }
      goto fail;
    case 'n':
      if (p[0] == 'u' && p[1] == 'l' && p[2] == 'l' && json_parse__is_delimiter(p[3])) {
        p += 3;
        *sv = const0;
        return p;
      }
      goto fail;
  }
fail:
  error("json_parse(): Invalid syntax.\n");
  return NULL;
}

char const *json_parse_svalue(char const *p, size_t slen, svalue_t *sv) {
  if (slen > JSON_MAX_SIZE) error("json_parse(): Too long input: %d.\n", slen);
  return json_parse_svalue__depth(p, sv, 0);
}






#ifdef F_JSON_STRINGIFY
// json_stringify
void f_json_stringify(void) {
  char buf[JSON_MAX_SIZE];
  char *e = json_stringify_svalue(sp-1, buf, buf + sizeof(buf), (int)sp->u.number);
  (void)e;
  pop_stack();
  pop_stack();
  push_malloced_string(string_copy(buf, "f_json_stringify"));
}
#endif


#ifdef F_JSON_PARSE
// json_parse
void f_json_parse(void) {
  char const *p = sp->u.string;
  size_t slen = SVALUE_STRLEN(sp);
  push_svalue(&const0);
  json_parse_svalue(p, slen, sp);
  svalue_t tmp = sp[0];
  sp[0] = sp[-1];
  sp[-1] = tmp;
  pop_stack();
}
#endif






// PACKAGE_XK 定义自己的 PostgreSQL 连接结构，独立于 PACKAGE_DB
#include "libpq-fe.h"

typedef union dbconn_u {
  struct tmp_postgres {
    PGconn *conn;
    PGresult *res;
    int getting;
  } postgres;
} dbconn_t;

#define DB_FLAG_EMPTY   0x1

typedef struct _db {
  int flags;
  dbconn_t c;
} db_t;

static int dbConnAlloc, dbConnUsed;
static db_t *dbConnList;

// create_db_conn: 创建新的数据库连接槽位
static int create_db_conn() {
  int i;

  /* allocate more slots if we need them */
  if (dbConnAlloc == dbConnUsed) {
    i = dbConnAlloc;
    dbConnAlloc += 10;
    if (!dbConnList) {
      dbConnList = (db_t *)DCALLOC(dbConnAlloc, sizeof(db_t), TAG_XK, "create_db_conn");
    } else {
      dbConnList = RESIZE(dbConnList, dbConnAlloc, db_t, TAG_XK, "create_db_conn");
    }
    while (i < dbConnAlloc) {
      dbConnList[i++].flags = DB_FLAG_EMPTY;
    }
  }

  for (i = 0;  i < dbConnAlloc;  i++) {
    if (dbConnList[i].flags & DB_FLAG_EMPTY) {
      dbConnList[i].flags = 0;
      dbConnUsed++;
      return i + 1;
    }
  }

  fatal("dbConnAlloc != dbConnUsed, but no empty slots");
}
// find_db_conn: 查找数据库连接
static db_t *find_db_conn(int handle) {
  if (handle < 1 || handle > dbConnAlloc || (dbConnList[handle - 1].flags & DB_FLAG_EMPTY)) {
    return nullptr;
  }
  return &(dbConnList[handle - 1]);
}

// free_db_conn: 释放数据库连接槽位
static void free_db_conn(db_t *db) {
  DEBUG_CHECK(db->flags & DB_FLAG_EMPTY, "Freeing DB connection that is already freed\n");
  DEBUG_CHECK(!dbConnUsed, "Freeing DB connection when dbConnUsed == 0\n");
  dbConnUsed--;
  db->flags |= DB_FLAG_EMPTY;
}




static void Postgres_cleanup(dbconn_t * c) {
  if (c->postgres.res) PQclear(c->postgres.res);
  c->postgres.res = 0;
}

static int Postgres_close (dbconn_t *c) {
  if (c->postgres.res) PQclear(c->postgres.res);
  c->postgres.res = NULL;
  if (c->postgres.conn) PQfinish(c->postgres.conn);
  c->postgres.conn = NULL;
  return 1;
}

static int Postgres_execute_params(dbconn_t *c, const char *s, char const **params, int n) {
  c->postgres.res = n ? PQexecParams(c->postgres.conn, s, n, NULL, params, NULL, NULL, 0) : PQexec(c->postgres.conn, s);
  if (PQresultStatus(c->postgres.res) == PGRES_TUPLES_OK) return PQntuples(c->postgres.res);
  if (PQresultStatus(c->postgres.res) == PGRES_COMMAND_OK) return 0;
  return -1;
}

static int Postgres_send_query(dbconn_t *c, const char *s, char const **params, int n) {
  int ret = n ? PQsendQueryParams(c->postgres.conn, s, n, NULL, params, NULL, NULL, 0) : PQsendQuery(c->postgres.conn, s);
  if (ret == 1) c->postgres.getting = 1;
  return ret ? 1 : -1;
}

static void Postgres_cleanup_getting(dbconn_t *c) {
  while (PQgetResult(c->postgres.conn) != NULL);
  c->postgres.getting = 0;
}

static int Postgres_get_result(dbconn_t *c) {
  Postgres_cleanup(c);
  c->postgres.res = PQgetResult(c->postgres.conn);
  if (NULL == c->postgres.res) {
    c->postgres.getting = 0;
    return 0;
  }
  // �ƺ����Ŀ�������ʾ�����ģ�status��PGRES_TUPLES_OK
  ExecStatusType status = PQresultStatus(c->postgres.res);
  if (status != PGRES_SINGLE_TUPLE && status != PGRES_TUPLES_OK) {
    printf("Unexpected PQgetResult's status: %s\n", PQresStatus(PQresultStatus(c->postgres.res)));
    return -1;
  }
  int ret = PQntuples(c->postgres.res);
  if (ret == 1) return 1;
  Postgres_cleanup_getting(c);
  return 0;
}

void xk_cleanup() {
  int i;

  for (i = 0;  i < dbConnAlloc;  i++) {
    if (!(dbConnList[i].flags & DB_FLAG_EMPTY)) {
      Postgres_cleanup(&dbConnList[i].c);
      Postgres_close(&dbConnList[i].c);

      dbConnList[i].flags = DB_FLAG_EMPTY;
      dbConnUsed--;
    }
  }
}







#ifdef F_PG_CLOSE
void f_pg_close(void) {
  int ret = 0;
  db_t *db;

  db = find_db_conn(sp->u.number);
  if (!db) {
    error("Attempt to close an invalid database handle\n");
  }

  Postgres_cleanup(&db->c);
  Postgres_close(&db->c);

  /* Remove the entry from the linked list */
  free_db_conn(db);

  sp->u.number = ret;
}
#endif


#ifdef F_PG_OK
void f_pg_ok(void) {
  int ret = 0;
  db_t *db;

  db = find_db_conn(sp->u.number);
  if (!db) {
    error("Attempt to check an invalid database handle\n");
  }

  auto c = &db->c;
  ret = c && PQstatus(c->postgres.conn) == CONNECTION_OK;

  sp->u.number = ret;
}
#endif


#ifdef F_PG_CONNECT
void f_pg_connect(void) {
  char *errormsg = 0;
  db_t *db;
  int handle, ret = 0;

  handle = create_db_conn();
  if (!handle) {
    pop_stack();
    push_number(0);
    return;
  }
  db = find_db_conn(handle);

  db->c.postgres.conn = NULL;
  db->c.postgres.res = NULL;
  db->c.postgres.getting = 0;

  db->c.postgres.conn = PQconnectdb(sp->u.string);
  ret = db->c.postgres.conn && PQstatus(db->c.postgres.conn) == CONNECTION_OK;

  pop_stack();
  if (!ret) {
    free_db_conn(db);
    error("pg_connect(): Connectoin failed.\n");
  } else {
    push_number(handle);
  }
}
#endif



#ifdef F_PG_CALL
#define XKOS_PG_CALL_MAX_PARAMS 32

static void Postgres_fetch_row(dbconn_t *c, int i, svalue_t *row, int start_pos, int num_fields) {
  for (int j = 0; j < num_fields; ++j) {
    svalue_t *field = row + start_pos + j;
    // https://github.com/postgres/postgres/blob/master/src/include/catalog/pg_type.h
    Oid oid = PQftype(c->postgres.res, j);
    char const *str = PQgetvalue(c->postgres.res, i, j);
    //printf("|%d,%d|%d|%s|\n", i, j, oid, str);
    if (PQgetisnull(c->postgres.res, i, j)) {
      *field = const0u;
      continue;
    }
#define BOOLOID      16
#define INT8OID      20
#define INT2OID      21
#define INT4OID      23
#define JSONOID 114
#define FLOAT4OID 700
#define FLOAT8OID 701
#define CASHOID 790
#define JSONBOID 3802
    switch (oid) {
      case JSONBOID: case JSONOID:
        // ������Ҫ�׳��쳣���������row���ڴ�й©�ˣ�Ӧ�ð����dbģ��ĳ�pgsqlר�õ�
        json_parse_svalue(str, strlen(str), field);
        break;
      case INT2OID: case INT4OID: case INT8OID:
      case FLOAT4OID: case FLOAT8OID:
      case CASHOID:
        json_parse_svalue(str, strlen(str), field);
        break;
      case BOOLOID:
        *field = (str[0] == 't') ? const1 : const0;
        break;
      default:
        field->type = T_STRING;
        field->subtype = STRING_MALLOC;
        field->u.string = string_copy(str, "pg_call");
    }
  }
}

static void Postgres_push_row(dbconn_t *c, int i) {
  int num_fields = PQnfields(c->postgres.res);

  if (num_fields == 0) {
    push_number(0);
    return;
  }

  if (num_fields > 1) {
    array_t *row = allocate_empty_array(num_fields);
    push_refed_array(row);
    Postgres_fetch_row(c, i, row->item, 0, num_fields);
  } else {
    push_number(0);
    Postgres_fetch_row(c, i, sp, 0, num_fields);
  }
}

static void Postgres_push_rows(dbconn_t *c, int num_rows) {
  array_t *rows = &the_null_array;
  int num_fields = PQnfields(c->postgres.res);

  if (num_fields == 0) {
    push_number(num_rows);
    return;
  }

  if (num_rows) rows = allocate_empty_array(num_rows);
  push_refed_array(rows);
  if (num_rows == 0) return;

  for (int i = 0; i < num_rows; ++i) {
    array_t *row = NULL;
    if (num_fields > 1) {
      row = allocate_empty_array(num_fields);
      rows->item[i].type = T_ARRAY;
      rows->item[i].u.arr = row;
      Postgres_fetch_row(c, i, row->item, 0, num_fields);
    } else {
      Postgres_fetch_row(c, i, rows->item, i, num_fields);
    }
  }
}

static void do_pg_call(int also_fetch) {
  svalue_t *arg = sp - st_num_arg + 1;
  int num_arg = st_num_arg;
  int ret = 0;
  db_t *db;
  size_t i = 0;
  svalue_t *p = arg + 2;
  size_t n = num_arg - 2;
  char buf[JSON_MAX_SIZE];
  char *o = buf;
  char *e = buf + sizeof(buf);
  const char *params[XKOS_PG_CALL_MAX_PARAMS];

  if (n > XKOS_PG_CALL_MAX_PARAMS) error("pg_call(): Too many params.\n");

  db = find_db_conn(arg->u.number);
  if (!db) {
    error("Attempt to exec on an invalid database handle\n");
  }
  if ((&db->c)->postgres.getting) error("You should use pg_get after pg_send until get 0!\n");

  for (; i < n; ++i) {
    svalue_t *sv = p + i;
    if (e - o < 1024) error("pg_call(): Buffer overflow.\n");
    params[i] = o;
    switch (sv->type) {
      case T_NUMBER: o += sprintf(o, "%ld", sv->u.number) + 1; break;
      case T_REAL: o += sprintf(o, "%.15g", sv->u.real) + 1; break;
      case T_STRING: params[i] = sv->u.string; break;
      case T_MAPPING:
      case T_ARRAY: o = json_stringify_svalue(sv, o, e, 0) + 1; break;
      default: error("pg_call(): Unknown param type.\n");
    }
  }

  Postgres_cleanup(&db->c);

  // PQexecParams��֧�ֶ�����䣬����û����ʱ����PQexec
  ret = (also_fetch ? Postgres_execute_params : Postgres_send_query)(&(db->c), (arg+1)->u.string, params, n);

  if (!also_fetch && ret == 1) {
    PQsetSingleRowMode((&(db->c))->postgres.conn);
  }

  if (ret == -1) {
    char errbuf[4096];
    snprintf(errbuf, sizeof(errbuf), "%s", PQerrorMessage((&(db->c))->postgres.conn));
    Postgres_cleanup(&(db->c));
    error("pg_call(): %s\n%s", (arg+1)->u.string, errbuf);
  }

  // ��ʱ�������Ĳ�����û���ˣ��������
  while (num_arg--) pop_stack();

  if (also_fetch) {
    Postgres_push_rows(&db->c, ret);
    Postgres_cleanup(&db->c);
  }
}

void f_pg_call(void) {
  return do_pg_call(1);
}
void f_pg_send(void) {
  return do_pg_call(0);
}
void f_pg_get(void) {
  db_t *db;

  db = find_db_conn(sp->u.number);
  if (!db) {
    error("Attempt to exec on an invalid database handle\n");
  }
  int ret = Postgres_get_result(&db->c);

  if (ret == -1) {
    char errbuf[4096];
    snprintf(errbuf, sizeof(errbuf), "%s", PQerrorMessage((&(db->c))->postgres.conn));
    Postgres_cleanup_getting(&(db->c));
    error("pg_get(): %s", errbuf);
  }

  // ����Լ���ǳ���tricky�����ֻ��һ�У���ô�о��õ��Ӷα�ʾ��ǡ����0�Ļ����ͻ��û�н�������Ϣ�غ���
  pop_stack();
  if (ret) Postgres_push_row(&db->c, 0);
  else push_number(0);
  Postgres_cleanup(&db->c);
}
#endif




#ifdef F_PG_LOOK
void f_pg_look(void) {
  int i;
  outbuffer_t out;

  outbuf_zero(&out);

  for (i = 0;  i < dbConnAlloc;  i++) {
    if (dbConnList[i].flags & DB_FLAG_EMPTY) continue;
    // TODO: stats
    outbuf_addv(&out, "Handle: %d %s\n", i + 1, PQstatus((&dbConnList[i].c)->postgres.conn) == CONNECTION_OK ? "OK" : "BAD");
  }

  outbuf_push(&out);
}
#endif




#include "jemalloc/jemalloc.h"



void f_malloc_stats_print(void) {
  outbuffer_t out;
  outbuf_zero(&out);
  malloc_stats_print((void (*)(void *, const char *))outbuf_add, &out, sp->u.string);
  pop_stack();
  outbuf_push(&out);
}


/*
mallctl ���ʺϷ�װ����Ҫת��C���͵�����̫�鷳
���� malloc_stats_print ��ͳ����Ϣ������
string mallctl(string, string default: 0);
void f_mallctl(void) {
  const char *name = sp[-1].u.string;
  char oldp[1024];
  size_t oldlenp = sizeof(oldp) - 1;
  if (sp->type == T_STRING) {
    const char *arg = sp->u.string;
    int arg_len = SVALUE_STRLEN(sp);
    mallctl(name, oldp, &oldlenp, arg, arg_len);
  } else {
    mallctl(name, oldp, &oldlenp, NULL, 0);
  }
  pop_stack();
  pop_stack();
  push_malloced_string(string_copy(oldp, "f_mallctl"));
}
*/


#ifdef F_JEMALLOC_PURGE
void f_jemalloc_purge(void) {
  mallctl("arena.4096.purge", NULL, NULL, NULL, 0);
}
#endif


