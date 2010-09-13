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
static void test_calendar(void);
//static void test_utc(void);
//static void test_utcplus(void);


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
  
  printf("-- test_calendar() \n");
  test_calendar();

  printf(" -- test_gtai()\n");
  test_gtai();

  //  printf(" -- test_utc() \n");
  // test_utc();

  //  printf(" -- test_utcplus() \n");
  //test_utcplus();

  return 0;
}

static void test_calendar(void)
{
  const static timecalc_calendar_t t1 = 
    { 1990, 0, 1, 0, 0, 0, 0, TIMECALC_SYSTEM_GREGORIAN_TAI };

  const static timecalc_calendar_t t2 = 
    { 1991, 0, 1, 0, 0, 0, 0, TIMECALC_SYSTEM_GREGORIAN_TAI };
  const static timecalc_calendar_t t3 = 
    { 1990, 0, 1, 0, 0, 0, -3, TIMECALC_SYSTEM_GREGORIAN_TAI };
  const static char *rep = "1990-01-01 00:00:00.000000000 TAI";
  char buf[128];
  int rv;

  rv = timecalc_calendar_cmp(&t1, &t2);
  ASSERT_INTEGERS_EQUAL(-1, rv, "cmp says t1 is not < t2");
  
  rv = timecalc_calendar_cmp(&t2, &t1);
  ASSERT_INTEGERS_EQUAL(1, rv, "cmp says t2 is not < t1");

  rv = timecalc_calendar_cmp(&t3, &t1);
  ASSERT_INTEGERS_EQUAL(-1, rv, "cmp says t3 is not < t1");
  
  rv = timecalc_calendar_cmp(&t1, &t1);
  ASSERT_INTEGERS_EQUAL(0, rv, "cmp says t1 != t1");
  
  rv = timecalc_calendar_sprintf(buf, 128, &t1);
  ASSERT_INTEGERS_EQUAL(rv, 33, "Wrong length for sprintf(t1)");
  ASSERT_STRINGS_EQUAL(buf, rep, "Wrong representation for t1");
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
  timecalc_zone_t *gtai;
  static const char *check_gtai_desc = "TAI";
  const char *gtai_desc;
  static timecalc_calendar_t gtai_epoch = 
    { 1958, 0, 1, 0, 0, 0, 0, TIMECALC_SYSTEM_GREGORIAN_TAI };
  timecalc_calendar_t a, b;
  int rv;
  char buf[128];

  
  gtai_desc = timecalc_describe_system(TIMECALC_SYSTEM_GREGORIAN_TAI);
  ASSERT_STRINGS_EQUAL(gtai_desc, check_gtai_desc, "GTAI descriptions don't match");

  rv = timecalc_zone_new(TIMECALC_SYSTEM_GREGORIAN_TAI,
			 &gtai, 0, NULL);
  ASSERT_INTEGERS_EQUAL(0, rv, "Cannot create gtai zone");
  rv = gtai->epoch(gtai, &a);

  rv = timecalc_calendar_cmp(&a, &gtai_epoch);
  ASSERT_INTEGERS_EQUAL(0, rv, "Epoch does not compare equal to prototype");

  // Check the epoch
  rv = gtai->epoch(gtai, &a);
  ASSERT_INTEGERS_EQUAL(0, rv, "Cannot get gtai epoch");
  rv = timecalc_calendar_cmp(&a, &gtai_epoch);
  ASSERT_INTEGERS_EQUAL(rv, 0, "gtai epoch isn't what we expected");

  // Add a year
  {
    timecalc_interval_t ti;
    static const char *check = "1959-01-01 00:00:00.000000000 TAI";
    static timecalc_calendar_t check_tm = 
      { 1959, 0, 1, 0, 0, 0, 0, TIMECALC_SYSTEM_GREGORIAN_TAI };


    ti.ns = 0;
    ti.s = (1 * 365 * 86400);
    rv = timecalc_zone_add(gtai, &b, &a, &ti);
    ASSERT_INTEGERS_EQUAL(0, rv, "add() failed");
    rv = timecalc_calendar_sprintf(buf, 128, &b);
    ASSERT_STRINGS_EQUAL(buf, check,
			 "Adding 1 year of seconds to epoch");
    //printf("b = %s\n", buf);
    // timecalc_calendar_sprintf(buf, 128, &check_tm);
    //printf("check = %s\n", buf);
    rv = timecalc_calendar_cmp(&b, &check_tm);
    ASSERT_INTEGERS_EQUAL(0, rv, "Adding a year of seconds doesn't lead to year + 1");
  }


  // Now, 1960 was a leap year. If we add 3 * 365 * 86400, we should get december 31st.
  {
    timecalc_interval_t ti;
    static const char *check1 = "1960-12-31 00:00:00.000000000 TAI";
    
    ti.ns = 0;
    ti.s = (3 * 365 * 86400);
    rv = timecalc_zone_add(gtai, 
			   &b,
			   &a,
			   &ti);
    ASSERT_INTEGERS_EQUAL(0, rv, "add() failed");
    rv = timecalc_calendar_sprintf(buf, 128, 
				   &b);
    ASSERT_STRINGS_EQUAL(buf, check1,
			 "Adding 2 years of seconds to epoch failed.");
  }

  // 2000 was a leap year (divisible by 400)
  // 1900 was not (divisible by 100)
  {
    timecalc_interval_t ti;
    static timecalc_calendar_t a = 
      { 2000, 1, 28, 0, 0, 0, 0, TIMECALC_SYSTEM_GREGORIAN_TAI };

    // 1996 was
    static timecalc_calendar_t b = 
      { 1900, 1, 28, 0, 0, 0, 0, TIMECALC_SYSTEM_GREGORIAN_TAI };
    char buf[128];
    timecalc_calendar_t c;
    const char *check1 = "2000-02-29 00:00:00.000000000 TAI";
    const char *check2 = "1900-03-01 00:00:00.000000000 TAI";

    ti.ns = 0;
    ti.s = (86400);
    rv = timecalc_zone_add(gtai, &c, &a, &ti);
    ASSERT_INTEGERS_EQUAL(0, rv, "add() failed");
    rv = timecalc_calendar_sprintf(buf, 128, &c);
    ASSERT_STRINGS_EQUAL(buf, check1, 
			 "Adding a day to 28 Feb 2000");

    rv = timecalc_zone_add(gtai, &c, &b, &ti);
    ASSERT_INTEGERS_EQUAL(0, rv, "add() failed");
    rv = timecalc_calendar_sprintf(buf, 128, &c);
    ASSERT_STRINGS_EQUAL(buf, check2, 
			 "Adding a day to 28 Feb 1900");

  }
    
  // Now check aux
  {
    timecalc_calendar_aux_t aux;
    static timecalc_calendar_t a  =
      { 2010, 8, 1, 13, 0, 0, 0, TIMECALC_SYSTEM_GREGORIAN_TAI };

    // 18th August 1804 was a Saturday
    static timecalc_calendar_t b = 
      { 1804, 7, 18, 13, 0, 0, 0, TIMECALC_SYSTEM_GREGORIAN_TAI };

    char buf[128];

    timecalc_calendar_sprintf(buf, 128, &a);

    // 1st Sept 2010 is a Wednesday
    rv = gtai->aux(gtai, &a, &aux);
    ASSERT_INTEGERS_EQUAL(rv, 0, "Retrieving aux info for 1 Sep 2010");

    // Weds = 2 
    ASSERT_INTEGERS_EQUAL(aux.wday, 3, 
			  "aux.wday is not Wednesday");
    // yday = 31 + 28 + 31 + 30 + 31 + 30 + 31 + 31
    //      = 243
    ASSERT_INTEGERS_EQUAL(aux.yday, 243,
			  "1 Sep is not day 243 of a non-leap-year?");

    
    ASSERT_INTEGERS_EQUAL(aux.is_dst, 0,
			  "a TAI time is DST?!");
    
    rv = gtai->aux(gtai, &b, &aux);
    ASSERT_INTEGERS_EQUAL(rv, 0, "Retrieving aux info for 18th August 1804");

    // Saturday = 5
    ASSERT_INTEGERS_EQUAL(aux.wday, 6,
			  "18th Aug 1804 was not a Saturday?");

    // yday = 31 + 29 + 31 + 30 + 31+ 30 + 31 + 18 -1 (zero based)
    //      = 231
    ASSERT_INTEGERS_EQUAL(aux.yday,
			  230, "18th Aug 1804 was not day 230");

    ASSERT_INTEGERS_EQUAL(aux.is_dst, 0,
			  "A TAI time is DST?!");
  }


  
  rv = timecalc_zone_dispose(&gtai);
  ASSERT_INTEGERS_EQUAL(0, rv, "Cannot dispose gtai");
}

#if 0

static void test_utc(void)
{
  timecalc_zone_t *utc;
  static const char *check_utc_desc = "UTC";
  const char *utc_desc;
  int rv;
  char buf[128];
  timecalc_calendar_t tgt;

    
  utc_desc = timecalc_describe_system(TIMECALC_SYSTEM_UTC);
  ASSERT_STRINGS_EQUAL(utc_desc, check_utc_desc, "UTC descriptions don't match");

  rv = timecalc_zone_new(TIMECALC_SYSTEM_UTC,
			 &utc, 0, NULL);
  ASSERT_INTEGERS_EQUAL(0, rv, "Cannot create UTC timezone");


  // Convert 1 Jan 1900 to UTC.
  {
    static timecalc_calendar_t a = 
      { 1900, TIMECALC_JANUARY, 1, 0, 0, 0, 0, TIMECALC_SYSTEM_GREGORIAN_TAI };
    static const char *result = "1900-01-01 00:00:00.000000000 UTC";

    // Raise to UTC
    rv = timecalc_zone_raise(utc, &tgt, &a);
    ASSERT_INTEGERS_EQUAL(0, rv, "Raise failed");

    rv = timecalc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Conversion to UTC failed  [1]");
  }

  {    
    // Jan 1 1972 00:00:10 is actually UTC 08:<something> because
    // UTC references itself.
    static timecalc_calendar_t b = 
      { 1972, TIMECALC_JANUARY, 1, 0, 0, 9, 100000, TIMECALC_SYSTEM_GREGORIAN_TAI };
    static const char *result = "1972-01-01 00:00:07.577282000 UTC";

    rv = timecalc_zone_raise(utc, &tgt, &b);
    ASSERT_INTEGERS_EQUAL(0, rv, "Raise failed [2]");
    
    rv = timecalc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Conversion to UTC failed [2]");
  }
  

  {    
    // Jan 1 1972 00:00:12 is in fact earlier because it acquires the 10s
    // offset applied at the start of 1972.
    // UTC references itself.
    static timecalc_calendar_t b = 
      { 1972, TIMECALC_JANUARY, 1, 0, 0, 12, 100000, TIMECALC_SYSTEM_GREGORIAN_TAI };
    static const char *result = "1972-01-01 00:00:02.000100000 UTC";

    rv = timecalc_zone_raise(utc, &tgt, &b);
    ASSERT_INTEGERS_EQUAL(0, rv, "Raise failed [2.5]");
    
    rv = timecalc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Conversion to UTC failed [2.5]");
  }
  

  {    
    // According to today's bulletin C, UTC-TAI = -34s (ie. TAI is 34s ahead)
    // UTC references itself.
    static timecalc_calendar_t b = 
      { 2010, TIMECALC_SEPTEMBER, 2, 19, 56, 12, 000000, TIMECALC_SYSTEM_UTC };
    static const char *result = "2010-09-02 19:56:46.000000000 TAI";
    timecalc_zone_t *z;

    rv = timecalc_zone_lower(utc, &tgt, &z, &b);
    ASSERT_INTEGERS_EQUAL(0, rv, "Lower failed [3]");
    
    rv = timecalc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Conversion to TAI failed [3]");
  }


  
  {
    // .. and there was a leap second on Dec 31st 1978, @ TAI -17
    static timecalc_calendar_t b = 
      { 1979, TIMECALC_JANUARY, 1, 0, 0, 18, 0, TIMECALC_SYSTEM_GREGORIAN_TAI };
    static const char *result = "1978-12-31 23:59:60.000000000 UTC";

    rv = timecalc_zone_raise(utc, &tgt, &b);
    ASSERT_INTEGERS_EQUAL(0, rv, "Raise failed [4]");
    
    rv = timecalc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Conversion to UTC failed [4]");
  }

  {
    // .. but adding a month to the start of Dec 1978 takes you to Jan 1978
    static timecalc_calendar_t b = 
      { 1978, TIMECALC_DECEMBER, 1, 0, 0, 0, 0, TIMECALC_SYSTEM_UTC };
    static const char *result = "1979-01-01 00:00:00.000000000 UTC";
    static timecalc_calendar_t add = 
      { 0, 1, 0, 0, 0, 0, 0, TIMECALC_SYSTEM_INVALID };

    rv = timecalc_op_offset(utc, &tgt, TIMECALC_OP_ADD, &b, &add);
    ASSERT_INTEGERS_EQUAL(0, rv, "Offset add failed [5]");
    
    rv = timecalc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Offset add result compare failed [5]");
  }
  {
    // Though adding 31 * 86400 gives you 23:59:60 because of the leap second.
    static timecalc_calendar_t b = 
      { 1978, TIMECALC_DECEMBER, 1, 0, 0, 0, 0, TIMECALC_SYSTEM_UTC };
    static const char *result = "1978-12-31 23:59:60.000000000 UTC";
    static timecalc_calendar_t add = 
      { 0, 0, 0, 0, 0, 31 * 86400, 0, TIMECALC_SYSTEM_INVALID };

    rv = timecalc_op_offset(utc, &tgt, TIMECALC_OP_ADD, &b, &add);
    ASSERT_INTEGERS_EQUAL(0, rv, "Offset add failed [6]");
    
    rv = timecalc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Offset add result compare failed [6]");
  }
  
  {
    // Adding 1s to 23:59:58 gives you 23:59:59
    static timecalc_calendar_t b = 
      { 1978, TIMECALC_DECEMBER, 31, 23, 59, 58, 0, TIMECALC_SYSTEM_UTC };
    static const char *result = "1978-12-31 23:59:59.000000000 UTC";
    static timecalc_calendar_t add = 
      { 0, 0, 0, 0, 0, 1, 0, TIMECALC_SYSTEM_INVALID };

    rv = timecalc_op_offset(utc, &tgt, TIMECALC_OP_ADD, &b, &add);
    ASSERT_INTEGERS_EQUAL(0, rv, "Offset add failed [7]");
    
    rv = timecalc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Offset add result compare failed [7]");
  }

  {
    // Adding 2s to 23:59:58 gives you 23:59:60
    static timecalc_calendar_t b = 
      { 1978, TIMECALC_DECEMBER, 31, 23, 59, 58, 0, TIMECALC_SYSTEM_UTC };
    static const char *result = "1978-12-31 23:59:60.000000000 UTC";
    static timecalc_calendar_t add = 
      { 0, 0, 0, 0, 0, 2, 0, TIMECALC_SYSTEM_INVALID };

    rv = timecalc_op_offset(utc, &tgt, TIMECALC_OP_ADD, &b, &add);
    ASSERT_INTEGERS_EQUAL(0, rv, "Offset add failed [7]");
    
    rv = timecalc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Offset add result compare failed [7]");
  }

  {
    // Adding 3s gives 0:00:01
    static timecalc_calendar_t b = 
      { 1978, TIMECALC_DECEMBER, 31, 23, 59, 58, 0, TIMECALC_SYSTEM_UTC };
    static const char *result = "1979-01-01 00:00:00.000000000 UTC";
    static timecalc_calendar_t add = 
      { 0, 0, 0, 0, 0, 3, 0, TIMECALC_SYSTEM_INVALID };

    rv = timecalc_op_offset(utc, &tgt, TIMECALC_OP_ADD, &b, &add);
    ASSERT_INTEGERS_EQUAL(0, rv, "Offset add failed [8]");
    
    rv = timecalc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Offset add result compare failed [8]");
  }

  {
    // Adding 1s to 23:59:60 gives 00:00:00
    static timecalc_calendar_t b = 
      { 1978, TIMECALC_DECEMBER, 31, 23, 59, 60, 0, TIMECALC_SYSTEM_UTC };
    static const char *result = "1979-01-01 00:00:00.000000000 UTC";
    static timecalc_calendar_t add = 
      { 0, 0, 0, 0, 0, 1, 0, TIMECALC_SYSTEM_INVALID };

    rv = timecalc_op_offset(utc, &tgt, TIMECALC_OP_ADD, &b, &add);
    ASSERT_INTEGERS_EQUAL(0, rv, "Offset add failed [9]");
    
    rv = timecalc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Offset add result compare failed [9]");
  }


  {
    // Adding -1s to 23:59:60 gives 23:59:59
    static timecalc_calendar_t b = 
      { 1978, TIMECALC_DECEMBER, 31, 23, 59, 60, 0, TIMECALC_SYSTEM_UTC };
    static const char *result = "1978-12-31 23:59:59.000000000 UTC";
    static timecalc_calendar_t add = 
      { 0, 0, 0, 0, 0, -1, 0, TIMECALC_SYSTEM_INVALID };

    rv = timecalc_op_offset(utc, &tgt, TIMECALC_OP_ADD, &b, &add);
    ASSERT_INTEGERS_EQUAL(0, rv, "Offset add failed [10]");
    
    rv = timecalc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Offset add result compare failed [10]");
  }


  rv = timecalc_zone_dispose(&utc);
  ASSERT_INTEGERS_EQUAL(0, rv, "Cannot dispose UTC");
}


static void test_utcplus(void)
{
  timecalc_zone_t *utcplus;
  static const char *check_utcplus_dest = "UTC+0223";
  static const char *check_utcminus_dest = "UTC-0114";
  const char *utcplus_desc;
  const char *utcminus_desc;
  int rv;
  char buf[128];
  timecalc_calendar_t tgt;
  int sys = TIMECALC_SYSTEM_UTCPLUS_ZERO + (2*60 + 23);
  int sys2 = TIMECALC_SYSTEM_UTCPLUS_ZERO - (1*60 + 14);

  utcplus_desc = timecalc_describe_system(sys);
  ASSERT_STRINGS_EQUAL(utcplus_desc, check_utcplus_dest, "UTC+ descriptions don't match");

  utcminus_desc = timecalc_describe_system(sys2);
  ASSERT_STRINGS_EQUAL(utcminus_desc, check_utcminus_dest, "UTC- descriptions don't match");

  rv = timecalc_zone_new(sys, &utcplus, 0, NULL);
  ASSERT_INTEGERS_EQUAL(0, rv, "Cannot create UTC+ timezone");
  
  
  // Well before any UTC calculations.
  {
    static timecalc_calendar_t a_value =
      { 1940, TIMECALC_FEBRUARY, 3,  13, 00, 00, 0, TIMECALC_SYSTEM_GREGORIAN_TAI };
    static const char *result = "1940-02-03 15:23:00.000000000 UTC+0223";

    rv = timecalc_zone_raise(utcplus, &tgt, &a_value);
    ASSERT_INTEGERS_EQUAL(0, rv, "Cannot raise TAI to UTCPLUS");
    
    timecalc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Raise result compare failed [0]");
  }


  // UTC is 10s back in 1972
  {
    static timecalc_calendar_t a_value =
      { 1972, TIMECALC_FEBRUARY, 3,  13, 00, 00, 0, TIMECALC_SYSTEM_GREGORIAN_TAI };
    static const char *result = "1972-02-03 15:22:50.000000000 UTC+0223";

    rv = timecalc_zone_raise(utcplus, &tgt, &a_value);
    ASSERT_INTEGERS_EQUAL(0, rv, "Cannot raise TAI to UTCPLUS [1]");
    
    timecalc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Raise result compare failed [1]");
  }

  // The 31 Dec 1990 leap second happens on 1 Jan 1991 @ 02:22:59
  {
    static timecalc_calendar_t a_value = 
      { 1991, TIMECALC_JANUARY, 1, 02, 22, 60, 0, 0 };
    static const char *result = "1990-12-31 23:59:60.000000000 UTC";
    timecalc_zone_t *z;
    
    a_value.system = sys;

    rv = timecalc_zone_lower(utcplus, &tgt, &z, &a_value);
    ASSERT_INTEGERS_EQUAL(0, rv, "Cannot lower UTCPLUS to UTC ");
    
    timecalc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Lower result compare failed [2]");
  }
  
  // There are some wierd addition rules, obviously ..
  {
    static timecalc_calendar_t a_value = 
      { 1990, TIMECALC_DECEMBER, 31, 23, 59, 59, 0};
    static timecalc_calendar_t a_second = 
      { 0, 0, 0, 0, 0, 1, 0, TIMECALC_SYSTEM_INVALID };
    static const char *result = "1991-01-01 00:00:00.000000000 UTC+0223";
    
    a_value.system = sys;
    rv = timecalc_op_offset(utcplus, &tgt, TIMECALC_OP_ADD,
			    &a_value, &a_second);
    ASSERT_INTEGERS_EQUAL(rv, 0, "Cannot add 1s [3]");
    
    timecalc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "+1s result failed [3]");
  }

  {
    static timecalc_calendar_t a_value = 
      { 1991, TIMECALC_JANUARY, 1, 02, 22, 59, 0};
    static timecalc_calendar_t a_second = 
      { 0, 0, 0, 0, 0, 1, 0, TIMECALC_SYSTEM_INVALID };
    static const char *result = "1991-01-01 00:00:00.000000000 UTC+0223";
    
    printf("***\n");

    a_value.system = sys;
    rv = timecalc_op_offset(utcplus, &tgt, TIMECALC_OP_ADD,
			    &a_value, &a_second);
    ASSERT_INTEGERS_EQUAL(rv, 0, "Cannot add 1s [4]");
    
    timecalc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "+1s result failed [4]");
  }








  rv = timecalc_zone_dispose(&utcplus);
  ASSERT_INTEGERS_EQUAL(0, rv, "Cannot dispose UTC+");
}
#endif

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

