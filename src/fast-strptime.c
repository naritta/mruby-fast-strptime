/*
** strptime.c - Strptime class
**
** Copyright (c) Tadayoshi Funaba, Koichi Sasada, Yui Naruse, Ritta Narita
*2011-2017
**
** See Copyright Notice in LICENSE
*/
#include <mruby.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <mruby/hash.h>
#include <mruby/string.h>
#include <mruby/range.h>
#include <mruby/array.h>
#include <mruby/class.h>
#include <mruby/string.h>
#include <mruby/variable.h>
#include <mruby/data.h>

#define NUM_PATTERN_P() num_pattern_p(&fmt[fi + 1])

#define fail()                                                                 \
  { return 0; }

static const char *month_names[] = {
    "January", "February", "March",     "April",   "May",      "June",
    "July",    "August",   "September", "October", "November", "December",
    "Jan",     "Feb",      "Mar",       "Apr",     "May",      "Jun",
    "Jul",     "Aug",      "Sep",       "Oct",     "Nov",      "Dec"};

#define sizeof_array(o) (sizeof o / sizeof o[0])

#define issign(c) ((c) == '-' || (c) == '+')

/* imply NUL-terminated string */
static size_t read_digits(const char *s, int *n, size_t width) {
  const char *s0 = s;
  const char *se = s + width;
  int r = 0;

  for (; s < se && isdigit(*s); s++) {
    r *= 10;
    r += (unsigned char)((*s) - '0');
  }
  *n = r;
  return (size_t)(s - s0);
}

#define READ_DIGITS(n, w)                                                      \
  {                                                                            \
    size_t l;                                                                  \
    l = read_digits(&str[si], &n, w);                                          \
    if (l == 0)                                                                \
      fail();                                                                  \
    si += l;                                                                   \
  }

static int valid_range_p(int v, int a, int b) { return !(v < a || v > b); }

mrb_value date_zone_to_diff(mrb_value);

#define READ_DIGITS_MAX(n) READ_DIGITS(n, LONG_MAX)

#define LIKELY(x) (__builtin_expect((x), 1))
#define UNLIKELY(x) (__builtin_expect((x), 0))

#define REG_PC (pc)
#define GET_PC() REG_PC
#define SET_PC(x) (REG_PC = (x))
#define GET_CURRENT_INSN() (*GET_PC())
#define GET_OPERAND(n) (GET_PC()[(n)])
#define ADD_PC(n) (SET_PC(REG_PC + (n)))

#define JUMP(dst) (REG_PC += (dst))

#define LABEL(x) INSN_LABEL_##x
#define ELABEL(x) INSN_ELABEL_##x
#define LABEL_PTR(x) &&LABEL(x)

#define INSN_ENTRY(insn) LABEL(insn) :

#define TC_DISPATCH(insn)                                                      \
  goto *(void const *)GET_CURRENT_INSN();                                      \
  ;
#define END_INSN(insn) TC_DISPATCH(insn);

#define INSN_DISPATCH() TC_DISPATCH(__START__) {

#define END_INSNS_DISPATCH()                                                   \
  mrb_bug(mrb, "strptime: unknown insn: %p", GET_CURRENT_INSN());              \
  } /* end of while loop */

#define NEXT_INSN() TC_DISPATCH(__NEXT_INSN__)

#define NDIV(x, y) (-(-((x) + 1) / (y)) - 1)
#define DIV(n, d) ((n) < 0 ? NDIV((n), (d)) : (n) / (d))

static int leap_year_p(int y) {
  return ((y % 4 == 0) && (y % 100 != 0)) || (y % 400 == 0);
}

static const int common_year_yday_offset[] = {
    -1,
    -1 + 31,
    -1 + 31 + 28,
    -1 + 31 + 28 + 31,
    -1 + 31 + 28 + 31 + 30,
    -1 + 31 + 28 + 31 + 30 + 31,
    -1 + 31 + 28 + 31 + 30 + 31 + 30,
    -1 + 31 + 28 + 31 + 30 + 31 + 30 + 31,
    -1 + 31 + 28 + 31 + 30 + 31 + 30 + 31 + 31,
    -1 + 31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30,
    -1 + 31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
    -1 + 31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30
    /* 1    2    3    4    5    6    7    8    9    10   11 */
};
static const int leap_year_yday_offset[] = {
    -1,
    -1 + 31,
    -1 + 31 + 29,
    -1 + 31 + 29 + 31,
    -1 + 31 + 29 + 31 + 30,
    -1 + 31 + 29 + 31 + 30 + 31,
    -1 + 31 + 29 + 31 + 30 + 31 + 30,
    -1 + 31 + 29 + 31 + 30 + 31 + 30 + 31,
    -1 + 31 + 29 + 31 + 30 + 31 + 30 + 31 + 31,
    -1 + 31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30,
    -1 + 31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
    -1 + 31 + 29 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30
    /* 1    2    3    4    5    6    7    8    9    10   11 */
};

time_t timegm_noleapsecond(struct tm *tm) {
  long tm_year = tm->tm_year;
  int tm_yday = tm->tm_mday;
  if (leap_year_p(tm_year + 1900))
    tm_yday += leap_year_yday_offset[tm->tm_mon];
  else
    tm_yday += common_year_yday_offset[tm->tm_mon];

  /*
   *  `Seconds Since the Epoch' in SUSv3:
   *  tm_sec + tm_min*60 + tm_hour*3600 + tm_yday*86400 +
   *  (tm_year-70)*31536000 + ((tm_year-69)/4)*86400 -
   *  ((tm_year-1)/100)*86400 + ((tm_year+299)/400)*86400
   */
  return tm->tm_sec + tm->tm_min * 60 + tm->tm_hour * 3600 +
         (time_t)(tm_yday + (tm_year - 70) * 365 + DIV(tm_year - 69, 4) -
                  DIV(tm_year - 1, 100) + DIV(tm_year + 299, 400)) *
             86400;
}

void mrb_timespec_now(mrb_state *mrb, struct timespec *ts) {
  if (clock_gettime(CLOCK_REALTIME, ts) == -1) {
    mrb_bug(mrb, "no clock_gettime");
  }
}

static struct mrb_data_type mrb_strptime_type = {"Strptime", mrb_free};

static int strptime_exec0(mrb_state *mrb, void **pc, const char *fmt,
                          const char *str, size_t slen, struct timespec *tsp,
                          int *gmtoffp) {
  size_t si = 0;
  int year = INT_MAX, mon = -1, mday = -1, hour = -1, min = -1, sec = -1,
      nsec = 0, gmtoff = INT_MAX;
  if (UNLIKELY(tsp == NULL)) {
    static const void *const insns_address_table[] = {
        LABEL_PTR(A),   LABEL_PTR(B), LABEL_PTR(C),   LABEL_PTR(D),
        LABEL_PTR(E),   LABEL_PTR(F), LABEL_PTR(G),   LABEL_PTR(H),
        LABEL_PTR(I),   NULL,         NULL,           LABEL_PTR(L),
        LABEL_PTR(M),   LABEL_PTR(N), LABEL_PTR(O),   LABEL_PTR(P),
        LABEL_PTR(Q),   LABEL_PTR(R), LABEL_PTR(S),   LABEL_PTR(T),
        LABEL_PTR(U),   LABEL_PTR(V), LABEL_PTR(W),   LABEL_PTR(X),
        LABEL_PTR(Y),   LABEL_PTR(Z), LABEL_PTR(_25), LABEL_PTR(_2b),
        LABEL_PTR(_3a), NULL,         LABEL_PTR(_5f), LABEL_PTR(_60),
        LABEL_PTR(a),   LABEL_PTR(B), LABEL_PTR(c),   LABEL_PTR(d),
        LABEL_PTR(d),   NULL,         LABEL_PTR(g),   LABEL_PTR(B),
        NULL,           LABEL_PTR(j), NULL,           LABEL_PTR(l),
        LABEL_PTR(m),   LABEL_PTR(n), NULL,           LABEL_PTR(p),
        NULL,           LABEL_PTR(r), LABEL_PTR(s),   LABEL_PTR(t),
        LABEL_PTR(u),   LABEL_PTR(v), LABEL_PTR(w),   LABEL_PTR(x),
        LABEL_PTR(y),   LABEL_PTR(z),
    };
    *pc = (void *)insns_address_table;
    return 0;
  }

  INSN_DISPATCH();
  INSN_ENTRY(A) {
    ADD_PC(1);
    END_INSN(A)
  }
  INSN_ENTRY(B) {
    int i;
    for (i = 0; i < (int)sizeof_array(month_names); i++) {
      size_t l = strlen(month_names[i]);
      if (strncasecmp(month_names[i], &str[si], l) == 0) {
        si += l;
        mon = (i % 12) + 1;
        ADD_PC(1);
        END_INSN(B)
      }
    }
    fail();
  }
  INSN_ENTRY(C) {
    ADD_PC(1);
    END_INSN(C)
  }
  INSN_ENTRY(D) {
    ADD_PC(1);
    END_INSN(D)
  }
  INSN_ENTRY(E) {
    ADD_PC(1);
    END_INSN(E)
  }
  INSN_ENTRY(F) {
    ADD_PC(1);
    END_INSN(F)
  }
  INSN_ENTRY(G) {
    ADD_PC(1);
    END_INSN(G)
  }
  INSN_ENTRY(H) {
    READ_DIGITS(hour, 2);
    if (!valid_range_p(hour, 0, 23))
      fail();
    ADD_PC(1);
    END_INSN(H)
  }
  INSN_ENTRY(I) {
    ADD_PC(1);
    END_INSN(I)
  }
  INSN_ENTRY(L) {
    ADD_PC(1);
    END_INSN(L)
  }
  INSN_ENTRY(M) {
    READ_DIGITS(min, 2);
    if (!valid_range_p(min, 0, 59))
      fail();
    ADD_PC(1);
    END_INSN(M)
  }
  INSN_ENTRY(N) {
    size_t l;
    l = read_digits(&str[si], &nsec, 9);
    if (!l)
      fail();
    si += l;
    for (; l < 9; l++) {
      nsec *= 10;
    }
    ADD_PC(1);
    END_INSN(N)
  }
  INSN_ENTRY(O) {
    ADD_PC(1);
    END_INSN(O)
  }
  INSN_ENTRY(P) {
    ADD_PC(1);
    END_INSN(P)
  }
  INSN_ENTRY(Q) {
    ADD_PC(1);
    END_INSN(Q)
  }
  INSN_ENTRY(R) {
    ADD_PC(1);
    END_INSN(R)
  }
  INSN_ENTRY(S) {
    READ_DIGITS(sec, 2);
    if (!valid_range_p(sec, 0, 60))
      fail();
    ADD_PC(1);
    END_INSN(S)
  }
  INSN_ENTRY(T) {
    ADD_PC(1);
    END_INSN(T)
  }
  INSN_ENTRY(U) {
    ADD_PC(1);
    END_INSN(U)
  }
  INSN_ENTRY(V) {
    ADD_PC(1);
    END_INSN(V)
  }
  INSN_ENTRY(W) {
    ADD_PC(1);
    END_INSN(W)
  }
  INSN_ENTRY(X) {
    ADD_PC(1);
    END_INSN(X)
  }
  INSN_ENTRY(Y) {
    char c = str[si];
    if (issign(c))
      si++;
    READ_DIGITS(year, 4);
    if (c == '-')
      year *= -1;
    ADD_PC(1);
    END_INSN(Y)
  }
  INSN_ENTRY(Z) {
    ADD_PC(1);
    END_INSN(Z)
  }
  INSN_ENTRY(a) {
    ADD_PC(1);
    END_INSN(a)
  }
  INSN_ENTRY(c) {
    ADD_PC(1);
    END_INSN(c)
  }
  INSN_ENTRY(d) {
    READ_DIGITS(mday, 2);
    if (!valid_range_p(mday, 1, 31))
      fail();
    ADD_PC(1);
    END_INSN(d)
  }
  INSN_ENTRY(g) {
    ADD_PC(1);
    END_INSN(g)
  }
  INSN_ENTRY(j) {
    ADD_PC(1);
    END_INSN(j)
  }
  INSN_ENTRY(l) {
    ADD_PC(1);
    END_INSN(l)
  }
  INSN_ENTRY(m) {
    READ_DIGITS(mon, 2);
    if (!valid_range_p(mon, 1, 12))
      fail();
    ADD_PC(1);
    END_INSN(m)
  }
  INSN_ENTRY(n) {
    for (; si < slen && isspace(str[si]); si++) {
    }
    ADD_PC(1);
    END_INSN(n)
  }
  INSN_ENTRY(p) {
    ADD_PC(1);
    END_INSN(p)
  }
  INSN_ENTRY(r) {
    ADD_PC(1);
    END_INSN(r)
  }
  INSN_ENTRY(s) {
    ADD_PC(1);
    END_INSN(s)
  }
  INSN_ENTRY(t) {
    ADD_PC(1);
    END_INSN(t)
  }
  INSN_ENTRY(u) {
    ADD_PC(1);
    END_INSN(u)
  }
  INSN_ENTRY(v) {
    ADD_PC(1);
    END_INSN(v)
  }
  INSN_ENTRY(w) {
    ADD_PC(1);
    END_INSN(w)
  }
  INSN_ENTRY(x) {
    ADD_PC(1);
    END_INSN(x)
  }
  INSN_ENTRY(y) {
    READ_DIGITS(year, 2);
    year += year < 69 ? 2000 : 1900;
    ADD_PC(1);
    END_INSN(y)
  }
  INSN_ENTRY(z) {
    const char *p0 = str + si;
    int r;
    size_t len;
    if (*p0 == 'z' || *p0 == 'Z') {
      gmtoff = 0;
      ADD_PC(1);
      END_INSN(z)
    }
    if (issign(*p0))
      si++;
    READ_DIGITS(r, 2);
    gmtoff = r * 60;
    if (str[si] == ':')
      si++;
    len = read_digits(&str[si], &r, 2);
    if (len) {
      si += len;
      gmtoff += r;
    }
    gmtoff *= 60;
    if (*p0 == '-')
      gmtoff = -gmtoff;
    ADD_PC(1);
    END_INSN(z)
  }
  INSN_ENTRY(_25) {
    ADD_PC(1);
    END_INSN(_25)
  }
  INSN_ENTRY(_2b) {
    ADD_PC(1);
    END_INSN(_2b)
  }
  INSN_ENTRY(_3a) {
    ADD_PC(1);
    END_INSN(_3a)
  }
  INSN_ENTRY(_60) {
    size_t v = (size_t)GET_OPERAND(1);
    size_t fi = v & 0xFFFF;
    size_t cnt = v >> 16;
    /* optimize to short string instead of memcmp(3) */
    const char *p = str + si;
    const char *q = fmt + fi;
    const char *qe = q + cnt;
    for (; q < qe; p++, q++) {
      if (*p != *q)
        return 1;
    }
    pc += 2;
    si += cnt;
    END_INSN(_60)
  }
  INSN_ENTRY(_5f) {
    struct timespec ts;
    struct tm tm;
    time_t t;
    int gmt = gmtoff >= INT_MAX - 1 ? INT_MAX - gmtoff : 2;

    /* overwrite time */
    if (year != INT_MAX) {
      tm.tm_year = year - 1900;
      if (mon == -1)
        mon = 1;
    setmonth:
      tm.tm_mon = mon - 1;
      if (mday == -1)
        mday = 1;
    setmday:
      tm.tm_mday = mday;
      if (hour == -1)
        hour = 0;
    sethour:
      tm.tm_hour = hour;
      if (min == -1)
        min = 0;
    setmin:
      tm.tm_min = min;
      if (sec == -1)
        sec = 0;
      tm.tm_sec = sec;
    } else {
      mrb_timespec_now(mrb, &ts);
      // if (gmt) {
        t = ts.tv_sec;
        if (gmt == 2)
          t += gmtoff;
        gmtime_r(&t, &tm);
      // } else {
        // other time zone is not supported in mruby
        // long off;
        //localtime_r(&t, &tm);
        // gmtoff = (int)off;
      // }
      if (mon != -1)
        goto setmonth;
      if (mday != -1)
        goto setmday;
      if (hour != -1)
        goto sethour;
      if (min != -1)
        goto setmin;
      if (sec != -1)
        tm.tm_sec = sec;
    }

    //	if (gmt) {
    t = timegm_noleapsecond(&tm);
    if (gmt == 2)
      t -= gmtoff;
    //	}
    //	else {
    // other time zone is not supported in mruby
    //	    r = find_time_t(&tm, gmt, &t);
    //	    if (r) fail();
    //	}
    tsp->tv_sec = t;
    tsp->tv_nsec = nsec;
    *gmtoffp = gmtoff;
    return 0;
    END_INSN(_5f)
  }
  END_INSNS_DISPATCH();

  /* unreachable */
  mrb_bug(mrb, "strptime_exec0: unreachable");
  //    UNREACHABLE;
}

void **strptime_compile(mrb_state *mrb, const char *fmt, size_t flen) {
  size_t fi = 0;
  char c;
  void **isns0 = malloc(sizeof(void *) * (flen + 2));
  void **isns = isns0;
  void **insns_address_table;
  void *tmp;
  strptime_exec0(mrb, (void **)&insns_address_table, NULL, NULL, 0, NULL, NULL);

  while (fi < flen) {
    switch (fmt[fi]) {
    case '%':
      fi++;
      c = fmt[fi];
      switch (c) {
      case 'B':
      case 'H':
      case 'M':
      case 'N':
      case 'S':
      case 'Y':
      case 'b':
      case 'd':
      case 'e':
      case 'h':
      case 'm':
      case 'n':
      case 'y':
      case 'z':
        tmp = insns_address_table[c - 'A'];
        if (tmp) {
          *isns++ = tmp;
          fi++;
          continue;
        }
      default:
        mrb_raise(mrb, E_RUNTIME_ERROR, "invalid format");
        break;
      }
    case ' ':
    case '\t':
    case '\n':
    case '\v':
    case '\f':
    case '\r':
      *isns++ = insns_address_table['n' - 'A'];
      fi++;
      break;
    default: {
      const char *p0 = fmt + fi, *p = p0, *pe = fmt + flen;
      size_t v = fi;
      while (p < pe && *p != '%' && !ISSPACE(*p))
        p++;
      v += (p - p0) << 16;
      fi += p - p0;
      *isns++ = insns_address_table['`' - 'A'];
      *isns++ = (void *)v;
    } break;
    }
  }
  *isns++ = insns_address_table['_' - 'A'];
  realloc(isns0, sizeof(void *) * (isns - isns0));
  return isns0;
}

struct strptime_object {
  void **isns;
  const char *fmt;
};

static mrb_value strptime_initialize(mrb_state *mrb, mrb_value self) {
  mrb_value mfmt;
  mrb_get_args(mrb, "o", &mfmt);
  const char *fmt = RSTRING_PTR(mfmt);
  mrb_gc_register(mrb, mfmt);

  struct strptime_object *tobj;
  void **isns;

  isns = strptime_compile(mrb, fmt, strlen(fmt));
  tobj =
      (struct strptime_object *)mrb_malloc(mrb, sizeof(struct strptime_object));

  tobj->isns = isns;
  tobj->fmt = fmt;

  DATA_TYPE(self) = &mrb_strptime_type;
  DATA_PTR(self) = tobj;

  return self;
}

static mrb_value strptime_exec(mrb_state *mrb, mrb_value self) {
  mrb_value mstr;
  mrb_get_args(mrb, "o", &mstr);
  const char *str = mrb_str_to_cstr(mrb, mstr);

  struct strptime_object *tobj;
  tobj = DATA_PTR(self);

  int r, gmtoff = INT_MAX;
  struct timespec ts;

  r = strptime_exec0(mrb, tobj->isns, tobj->fmt, str, strlen(str), &ts,
                     &gmtoff);

  if (r) mrb_raise(mrb, E_ARGUMENT_ERROR, "string doesn't match");

  struct RClass *time_class;
  time_class = mrb_class_get(mrb, "Time");
  mrb_value time_obj = mrb_obj_value(time_class);

  mrb_value time =
      mrb_funcall(mrb, time_obj, "at", 1, mrb_fixnum_value(ts.tv_sec));
  return time;
}

static mrb_value strptime_execi(mrb_state *mrb, mrb_value self) {
  mrb_value mstr;
  mrb_get_args(mrb, "o", &mstr);
  const char *str = mrb_str_to_cstr(mrb, mstr);

  struct strptime_object *tobj;
  tobj = DATA_PTR(self);

  int r, gmtoff = INT_MAX;
  struct timespec ts;

  r = strptime_exec0(mrb, tobj->isns, tobj->fmt, str, strlen(str), &ts,
                     &gmtoff);

  if (r) mrb_raise(mrb, E_ARGUMENT_ERROR, "string doesn't match");

  return mrb_fixnum_value(ts.tv_sec);
}

static mrb_value
mrb_strptime_init_copy(mrb_state *mrb, mrb_value copy)
{
  mrb_value src;

  mrb_get_args(mrb, "o", &src);
  if (mrb_obj_equal(mrb, copy, src)) return copy;
  if (!mrb_obj_is_instance_of(mrb, src, mrb_obj_class(mrb, copy))) {
    mrb_raise(mrb, E_TYPE_ERROR, "wrong argument class");
  }
  if (!DATA_PTR(copy)) {
    DATA_PTR(copy) = (struct strptime_object *)mrb_malloc(mrb, sizeof(struct strptime_object));
    DATA_TYPE(copy) = &mrb_strptime_type;
  }
  *(struct strptime_object *)DATA_PTR(copy) = *(struct strptime_object *)DATA_PTR(src);
  return copy;
}

void mrb_mruby_fast_strptime_gem_init(mrb_state *mrb) {
  struct RClass *strptime_class =
      mrb_define_class(mrb, "Strptime", mrb->object_class);
  mrb_define_method(mrb, strptime_class, "initialize", strptime_initialize,
                    MRB_ARGS_REQ(1));
  mrb_define_method(mrb, strptime_class, "initialize_copy", mrb_strptime_init_copy,
                    MRB_ARGS_NONE());
  mrb_define_method(mrb, strptime_class, "exec", strptime_exec,
                    MRB_ARGS_REQ(1));
  mrb_define_method(mrb, strptime_class, "execi", strptime_execi,
                    MRB_ARGS_REQ(1));
}

void mrb_mruby_fast_strptime_gem_final(mrb_state *mrb) { /* finalizer */
}
