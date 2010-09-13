/* tctest.c */
/* (C) Kynesim Ltd 2010 */

/** @file
 * 
 * Unit tests for timecalc. Exits with zero status iff all tests
 *  have passed.
 *
 * @author Richard Watts <rrw@kynesim.co.uk>
 * @date   2010-08-30
 */

#include <stdint.h>
#include "timecalc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASSERT_INTERVALS_EQUAL(x,y,s)		\
  if (timecalc_interval_cmp((x),(y))) { faili((x),(y),"Intervals not equal",\
					     (s),__FILE__,__LINE__, __func__); }
  

#define ASSERT_INTEGERS_EQUAL(x,y,s) \
  if ((x) != (y)) { failint((x),(y),"Numbers not equal",\
			    (s), __FILE__, __LINE__, __func__); }


#define ASSERT_STRINGS_EQUAL(x,y,s) \
  if (strcmp((x),(y))) { failstring((x),(y),"Strings not equal",\
				    (s), __FILE__, __LINE__, __func__); }


static void failint(int x, int y,
		    const char *leg1, const char *leg2,
		    const char *file, const int line,
		    const char *func);

static void failstring(const char *x, const char *y,
		    const char *leg1, const char *leg2,
		    const char *file, const int line,
		    const char *func);

static void faili(const timecalc_interval_t *a, 
		  const timecalc_interval_t *b,
		  const char *leg1,
		  const char *leg2,
		  const char *file,
		  const int line,
		  const char *func);

static void test_gtai(void);
static void test_interval(void);


int main(int argn, char *args[])
{
  if (argn != 2)
    {
      fprintf(stderr, "Syntax: tctest [seed]\n");
      exit(1);
    }
  int seed = atoi(args[1]);
  printf("> Using seed = %d \n", seed);

  printf("-- test_interval() \n");
  test_interval();

  printf(" -- test_gtai()\n");
  test_gtai();

  return 0;
}


static void test_interval(void)
{
  const static timecalc_interval_t a = { 6, -100 }, b = { 4010, 1000004000 };
  const static timecalc_interval_t diff = { -4005, -4100 };
  const static timecalc_interval_t sum = { 4017, 3900 };
  timecalc_interval_t c;
  int rv;
  
  // Check that sum and difference work the way we think they do.
  timecalc_interval_add(&c, &a, &b);
  ASSERT_INTERVALS_EQUAL(&sum, &c, "add() doesn't work how we expect");
  
  timecalc_interval_subtract(&c, &a, &b);
  ASSERT_INTERVALS_EQUAL(&diff, &c, "subtract() doesn't work how we expect");

  // Check comparisons.
  rv = timecalc_interval_cmp(&a, &b);
  ASSERT_INTEGERS_EQUAL(-1, rv, "timecalc_interval_cmp(a,b) was not -1");

  rv = timecalc_interval_cmp(&b, &a);
  ASSERT_INTEGERS_EQUAL(1, rv, "timecalc_interval_cmp(b,a) was not 1");

  rv = timecalc_interval_cmp(&a, &a);
  ASSERT_INTEGERS_EQUAL(0, rv, "a does not compare equal to a");

  rv = timecalc_interval_cmp(&b, &b);
  ASSERT_INTEGERS_EQUAL(0, rv, "b does not compare equal to b");

  static const char *a_rep = "6 s -100 ns";
  char buf[128];

  rv = timecalc_interval_sprintf(buf, 128,
				 &a);
  ASSERT_INTEGERS_EQUAL(11, rv, "String rep of a of wrong length");
  ASSERT_STRINGS_EQUAL(buf, a_rep, "String rep of a not equal to prototype");
  
  rv = timecalc_interval_sgn(&a);
  ASSERT_INTEGERS_EQUAL(1,rv, "sgn(a) is not correct");

  const static timecalc_interval_t nve = { -20, 200 };
  rv = timecalc_interval_sgn(&nve);
  ASSERT_INTEGERS_EQUAL(-1, rv, "sgn(nve) is not correct");

  const static timecalc_interval_t zer = { 0, 0 };
  rv = timecalc_interval_sgn(&zer);
  ASSERT_INTEGERS_EQUAL(0, rv,  "sgn(zer) is not correct");
}


static void test_gtai(void)
{
  



}

static void faili(const timecalc_interval_t *a,
		  const timecalc_interval_t *b,
		  const char *leg1,
		  const char *leg2,
		  const char *file,
		  const int line,
		  const char *func)
{
  char bx[128], bx2[128];
  timecalc_interval_sprintf(bx, 128, a);
  timecalc_interval_sprintf(bx2, 128, b);

  fprintf(stderr, "%s:%d (%s) Failed: %s %s (op1=%s, op2=%s)\n",
	  file, line, func, leg1, leg2, bx, bx2);
  exit(2);
}

static void failint(int x, int y,
		    const char *leg1, const char *leg2,
		    const char *file, const int line,
		    const char *func)
{
  fprintf(stderr, "%s:%d (%s) Failed: %s %s (op1=%d, op2=%d)\n",
	  file, line, func,leg1,leg2, x, y);
  exit(3);
}


static void failstring(const char *x, const char *y,
		    const char *leg1, const char *leg2,
		    const char *file, const int line,
		    const char *func)
{
  fprintf(stderr, "%s:%d (%s) Failed: %s %s (op1=%s, op2=%s)\n",
	  file, line, func,leg1,leg2, x, y);
  exit(4);
}



// End file.

