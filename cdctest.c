/* cdtest.c */
/* (C) Metropolitan Police 2010 */

/*
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * The Original Code is cdatecalc, http://code.google.com/p/cdatecalc
 * 
 * The Initial Developer of the Original Code is the Metropolitan Police
 * All Rights Reserved.
 */

/** @file
 * 
 * Unit tests for cdc. Exits with zero status iff all tests
 *  have passed.
 *
 * @author Richard Watts <rrw@kynesim.co.uk>
 * @date   2010-08-30
 */

#include <stdint.h>
#include "cdc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASSERT_INTERVALS_EQUAL(x,y,s)		\
  if (cdc_interval_cmp((x),(y))) { faili((x),(y),"Intervals not equal",\
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

static void faili(const cdc_interval_t *a, 
		  const cdc_interval_t *b,
		  const char *leg1,
		  const char *leg2,
		  const char *file,
		  const int line,
		  const char *func);

static void test_gtai(void);
static void test_interval(void);
static void test_calendar(void);
static void test_utc(void);
static void test_utcplus(void);
static void test_bst(void);
static void test_rebased(void);
static void test_bounce(void);

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

   printf(" -- test_utc() \n");
  test_utc();

   printf(" -- test_utcplus() \n");
  test_utcplus();

  printf(" -- test_bst() \n");
  test_bst();

  printf(" -- test_rebased() \n");
  test_rebased();

  printf(" -- test_bounce() \n");
  test_bounce();

  return 0;
}

static void test_calendar(void)
{
  const static cdc_calendar_t t1 = 
    { 1990, 0, 1, 0, 0, 0, 0, CDC_SYSTEM_GREGORIAN_TAI };

  const static cdc_calendar_t t2 = 
    { 1991, 0, 1, 0, 0, 0, 0, CDC_SYSTEM_GREGORIAN_TAI };
  const static cdc_calendar_t t3 = 
    { 1990, 0, 1, 0, 0, 0, -3, CDC_SYSTEM_GREGORIAN_TAI };
  const static char *rep = "1990-01-01 00:00:00.000000000 TAI";
  char buf[128];
  int rv;

  rv = cdc_calendar_cmp(&t1, &t2);
  ASSERT_INTEGERS_EQUAL(-1, rv, "cmp says t1 is not < t2");
  
  rv = cdc_calendar_cmp(&t2, &t1);
  ASSERT_INTEGERS_EQUAL(1, rv, "cmp says t2 is not < t1");

  rv = cdc_calendar_cmp(&t3, &t1);
  ASSERT_INTEGERS_EQUAL(-1, rv, "cmp says t3 is not < t1");
  
  rv = cdc_calendar_cmp(&t1, &t1);
  ASSERT_INTEGERS_EQUAL(0, rv, "cmp says t1 != t1");
  
  rv = cdc_calendar_sprintf(buf, 128, &t1);
  ASSERT_INTEGERS_EQUAL(rv, 33, "Wrong length for sprintf(t1)");
  ASSERT_STRINGS_EQUAL(buf, rep, "Wrong representation for t1");
}


static void test_interval(void)
{
  const static cdc_interval_t a = { 6, -100 }, b = { 4010, 1000004000 };
  const static cdc_interval_t diff = { -4005, -4100 };
  const static cdc_interval_t sum = { 4017, 3900 };
  cdc_interval_t c;
  int rv;
  
  // Check that sum and difference work the way we think they do.
  cdc_interval_add(&c, &a, &b);
  ASSERT_INTERVALS_EQUAL(&sum, &c, "add() doesn't work how we expect");
  
  cdc_interval_subtract(&c, &a, &b);
  ASSERT_INTERVALS_EQUAL(&diff, &c, "subtract() doesn't work how we expect");

  // Check comparisons.
  rv = cdc_interval_cmp(&a, &b);
  ASSERT_INTEGERS_EQUAL(-1, rv, "cdc_interval_cmp(a,b) was not -1");

  rv = cdc_interval_cmp(&b, &a);
  ASSERT_INTEGERS_EQUAL(1, rv, "cdc_interval_cmp(b,a) was not 1");

  rv = cdc_interval_cmp(&a, &a);
  ASSERT_INTEGERS_EQUAL(0, rv, "a does not compare equal to a");

  rv = cdc_interval_cmp(&b, &b);
  ASSERT_INTEGERS_EQUAL(0, rv, "b does not compare equal to b");

  static const char *a_rep = "6 s -100 ns";
  char buf[128];

  rv = cdc_interval_sprintf(buf, 128,
				 &a);
  ASSERT_INTEGERS_EQUAL(11, rv, "String rep of a of wrong length");
  ASSERT_STRINGS_EQUAL(buf, a_rep, "String rep of a not equal to prototype");
  
  rv = cdc_interval_sgn(&a);
  ASSERT_INTEGERS_EQUAL(1,rv, "sgn(a) is not correct");

  const static cdc_interval_t nve = { -20, 200 };
  rv = cdc_interval_sgn(&nve);
  ASSERT_INTEGERS_EQUAL(-1, rv, "sgn(nve) is not correct");

  const static cdc_interval_t zer = { 0, 0 };
  rv = cdc_interval_sgn(&zer);
  ASSERT_INTEGERS_EQUAL(0, rv,  "sgn(zer) is not correct");
}


static void test_gtai(void)
{
  cdc_zone_t *gtai;
  static const char *check_gtai_desc = "TAI";
  const char *gtai_desc;
  static cdc_calendar_t gtai_epoch = 
    { 1958, 0, 1, 0, 0, 0, 0, CDC_SYSTEM_GREGORIAN_TAI };
  cdc_calendar_t a, b;
  int rv;
  char buf[128];

  
  gtai_desc = cdc_describe_system(CDC_SYSTEM_GREGORIAN_TAI);
  ASSERT_STRINGS_EQUAL(gtai_desc, check_gtai_desc, "GTAI descriptions don't match");

  rv = cdc_zone_new(CDC_SYSTEM_GREGORIAN_TAI,
			 &gtai, 0, NULL);
  ASSERT_INTEGERS_EQUAL(0, rv, "Cannot create gtai zone");
  rv = gtai->epoch(gtai, &a);

  rv = cdc_calendar_cmp(&a, &gtai_epoch);
  ASSERT_INTEGERS_EQUAL(0, rv, "Epoch does not compare equal to prototype");

  // Check the epoch
  rv = gtai->epoch(gtai, &a);
  ASSERT_INTEGERS_EQUAL(0, rv, "Cannot get gtai epoch");
  rv = cdc_calendar_cmp(&a, &gtai_epoch);
  ASSERT_INTEGERS_EQUAL(rv, 0, "gtai epoch isn't what we expected");

  // Add a year
  {
    cdc_interval_t ti;
    static const char *check = "1959-01-01 00:00:00.000000000 TAI";
    static cdc_calendar_t check_tm = 
      { 1959, 0, 1, 0, 0, 0, 0, CDC_SYSTEM_GREGORIAN_TAI };


    ti.ns = 0;
    ti.s = (1 * 365 * 86400);
    rv = cdc_zone_add(gtai, &b, &a, &ti);
    ASSERT_INTEGERS_EQUAL(0, rv, "add() failed");
    rv = cdc_calendar_sprintf(buf, 128, &b);
    ASSERT_STRINGS_EQUAL(buf, check,
			 "Adding 1 year of seconds to epoch");
    //printf("b = %s\n", buf);
    // cdc_calendar_sprintf(buf, 128, &check_tm);
    //printf("check = %s\n", buf);
    rv = cdc_calendar_cmp(&b, &check_tm);
    ASSERT_INTEGERS_EQUAL(0, rv, "Adding a year of seconds doesn't lead to year + 1");
  }


  // Now, 1960 was a leap year. If we add 3 * 365 * 86400, we should get december 31st.
  {
    cdc_interval_t ti;
    static const char *check1 = "1960-12-31 00:00:00.000000000 TAI";
    
    ti.ns = 0;
    ti.s = (3 * 365 * 86400);
    rv = cdc_zone_add(gtai, 
			   &b,
			   &a,
			   &ti);
    ASSERT_INTEGERS_EQUAL(0, rv, "add() failed");
    rv = cdc_calendar_sprintf(buf, 128, 
				   &b);
    ASSERT_STRINGS_EQUAL(buf, check1,
			 "Adding 2 years of seconds to epoch failed.");
  }

  // 2000 was a leap year (divisible by 400)
  // 1900 was not (divisible by 100)
  {
    cdc_interval_t ti;
    static cdc_calendar_t a = 
      { 2000, 1, 28, 0, 0, 0, 0, CDC_SYSTEM_GREGORIAN_TAI };

    // 1996 was
    static cdc_calendar_t b = 
      { 1900, 1, 28, 0, 0, 0, 0, CDC_SYSTEM_GREGORIAN_TAI };
    char buf[128];
    cdc_calendar_t c;
    const char *check1 = "2000-02-29 00:00:00.000000000 TAI";
    const char *check2 = "1900-03-01 00:00:00.000000000 TAI";

    ti.ns = 0;
    ti.s = (86400);
    rv = cdc_zone_add(gtai, &c, &a, &ti);
    ASSERT_INTEGERS_EQUAL(0, rv, "add() failed");
    rv = cdc_calendar_sprintf(buf, 128, &c);
    ASSERT_STRINGS_EQUAL(buf, check1, 
			 "Adding a day to 28 Feb 2000");

    rv = cdc_zone_add(gtai, &c, &b, &ti);
    ASSERT_INTEGERS_EQUAL(0, rv, "add() failed");
    rv = cdc_calendar_sprintf(buf, 128, &c);
    ASSERT_STRINGS_EQUAL(buf, check2, 
			 "Adding a day to 28 Feb 1900");

  }
    
  // Now check aux
  {
    cdc_calendar_aux_t aux;
    static cdc_calendar_t a  =
      { 2010, 8, 1, 13, 0, 0, 0, CDC_SYSTEM_GREGORIAN_TAI };

    // 18th August 1804 was a Saturday
    static cdc_calendar_t b = 
      { 1804, 7, 18, 13, 0, 0, 0, CDC_SYSTEM_GREGORIAN_TAI };

    char buf[128];

    cdc_calendar_sprintf(buf, 128, &a);

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


  // ... and diff. 1975 was not a leap year.
  {
    static cdc_calendar_t b = 
      { 1975, CDC_FEBRUARY, 28, 23, 59, 0, 0, CDC_SYSTEM_GREGORIAN_TAI };
    static cdc_calendar_t a = 
      { 1975, CDC_MARCH, 1, 0, 0, 0, 0, CDC_SYSTEM_GREGORIAN_TAI };
    cdc_interval_t iv;
    const char *result = "60 s 0 ns";
    
    rv = cdc_diff(gtai, &iv, &b, &a);
    ASSERT_INTEGERS_EQUAL(0, rv, "diff() failed [0]");
    
    rv = cdc_interval_sprintf(buf, 128, &iv);
    ASSERT_STRINGS_EQUAL(buf, result, "diff() result compare failed [0]");
  }

  // but 1976 was.
  {
    static cdc_calendar_t b = 
      { 1976, CDC_FEBRUARY, 28, 23, 59, 0, 0, CDC_SYSTEM_GREGORIAN_TAI };
    static cdc_calendar_t a = 
      { 1976, CDC_MARCH, 1, 0, 0, 0, 0, CDC_SYSTEM_GREGORIAN_TAI };
    cdc_interval_t iv;
    const char *result = "86460 s 0 ns";
    
    rv = cdc_diff(gtai, &iv, &b, &a);
    ASSERT_INTEGERS_EQUAL(0, rv, "diff() failed [0]");
    
    rv = cdc_interval_sprintf(buf, 128, &iv);
    ASSERT_STRINGS_EQUAL(buf, result, "diff() result compare failed [0]");
  }

  rv = cdc_zone_dispose(&gtai);
  ASSERT_INTEGERS_EQUAL(0, rv, "Cannot dispose gtai");
}



static void test_utc(void)
{
  cdc_zone_t *utc;
  static const char *check_utc_desc = "UTC";
  const char *utc_desc;
  int rv;
  char buf[128];
  cdc_calendar_t tgt;

    
  utc_desc = cdc_describe_system(CDC_SYSTEM_UTC);
  ASSERT_STRINGS_EQUAL(utc_desc, check_utc_desc, "UTC descriptions don't match");

  rv = cdc_utc_new(&utc);
  ASSERT_INTEGERS_EQUAL(0, rv, "Cannot create UTC timezone");

  // First off, we need to check a few things. 
  {
    static cdc_calendar_t a = 
      { 1972, CDC_JANUARY, 1, 0, 0, 0, 1, CDC_SYSTEM_UTC };
    const char *result = "0000-01-00 00:00:-10.000000000 OFF";


    rv = utc->offset(utc, &tgt, &a);
    ASSERT_INTEGERS_EQUAL(0, rv, "Cannot find UTC - TAI offset [A0]");
    // ASSERT_INTEGERS_EQUAL(0, ls, "Jan 1 1972 was not a leap second ?! [A0]");
    
    
    rv = cdc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "UTC-TAI offset test failed [A0]");
  }

  // Offset just before a leap second.
  {
    static cdc_calendar_t a = 
      { 1972, CDC_DECEMBER, 31, 23, 59, 59, 300, CDC_SYSTEM_UTC };
    const char *result = "0000-01-00 00:00:-11.000000000 OFF";

    rv = utc->offset(utc, &tgt, &a);
    ASSERT_INTEGERS_EQUAL(0, rv, "Cannot find UTC - TAI offset [A1]");
    // ASSERT_INTEGERS_EQUAL(1, ls, "Leap second check [A1]");
    
    
    rv = cdc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "UTC-TAI offset test failed [A1]");
  }

  // Offset at a leap second : ls = 0, but offset = -12.
  {
    static cdc_calendar_t a = 
      { 1972, CDC_DECEMBER, 31, 23, 59, 60, 2000, CDC_SYSTEM_UTC };
    const char *result = "0000-01-00 00:00:-12.000000000 OFF";
    //int ls;
    

    rv = utc->offset(utc, &tgt, &a);
    ASSERT_INTEGERS_EQUAL(0, rv, "Cannot find UTC - TAI offset [A2]");
    // ASSERT_INTEGERS_EQUAL(0, ls, "Leap second check [A2]");
    
    
    rv = cdc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "UTC-TAI offset test failed [A2]");
  }


  // Convert 1 Jan 1900 to UTC.
  {
    static cdc_calendar_t a = 
      { 1900, CDC_JANUARY, 1, 0, 0, 0, 0, CDC_SYSTEM_GREGORIAN_TAI };
    static const char *result = "1900-01-01 00:00:00.000000000 UTC";

    // Raise to UTC
    rv = cdc_zone_raise(utc, &tgt, &a);
    ASSERT_INTEGERS_EQUAL(0, rv, "Raise failed");

    rv = cdc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Conversion to UTC failed  [1]");
  }

  {    
    // Jan 1 1972 00:00:10 is actually UTC 08:<something> because
    // UTC references itself.
    static cdc_calendar_t b = 
      { 1972, CDC_JANUARY, 1, 0, 0, 9, 100000, CDC_SYSTEM_GREGORIAN_TAI };
    static const char *result = "1972-01-01 00:00:07.577282000 UTC";

    rv = cdc_zone_raise(utc, &tgt, &b);
    ASSERT_INTEGERS_EQUAL(0, rv, "Raise failed [2]");
    
    rv = cdc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Conversion to UTC failed [2]");
  }
  

  {    
    // Jan 1 1972 00:00:12 is in fact earlier because it acquires the 10s
    // offset applied at the start of 1972.
    // UTC references itself.
    static cdc_calendar_t b = 
      { 1972, CDC_JANUARY, 1, 0, 0, 12, 100000, CDC_SYSTEM_GREGORIAN_TAI };
    static const char *result = "1972-01-01 00:00:02.000100000 UTC";

    rv = cdc_zone_raise(utc, &tgt, &b);
    ASSERT_INTEGERS_EQUAL(0, rv, "Raise failed [2.5]");
    
    rv = cdc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Conversion to UTC failed [2.5]");
  }
  

  {    
    // According to today's bulletin C, UTC-TAI = -34s (ie. TAI is 34s ahead)
    // UTC references itself.
    static cdc_calendar_t b = 
      { 2010, CDC_SEPTEMBER, 2, 19, 56, 12, 000000, CDC_SYSTEM_UTC };
    static const char *result = "2010-09-02 19:56:46.000000000 TAI";
    cdc_zone_t *z;

    rv = cdc_zone_lower(utc, &tgt, &z, &b);
    ASSERT_INTEGERS_EQUAL(0, rv, "Lower failed [3]");
    
    rv = cdc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Conversion to TAI failed [3]");
  }


  
  {
    // .. and there was a leap second on Dec 31st 1978, @ TAI -17
    static cdc_calendar_t b = 
      { 1979, CDC_JANUARY, 1, 0, 0, 17, 0, CDC_SYSTEM_GREGORIAN_TAI };
    static const char *result = "1978-12-31 23:59:60.000000000 UTC";

    rv = cdc_zone_raise(utc, &tgt, &b);
    ASSERT_INTEGERS_EQUAL(0, rv, "Raise failed [4]");
    
    rv = cdc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Conversion to UTC failed [4]");
  }

  {
    // .. but adding a month to the start of Dec 1978 takes you to Jan 1978
    static cdc_calendar_t b = 
      { 1978, CDC_DECEMBER, 1, 0, 0, 0, 0, CDC_SYSTEM_UTC };
    static const char *result = "1979-01-01 00:00:00.000000000 UTC";
    static cdc_calendar_t add = 
      { 0, 1, 0, 0, 0, 0, 0, CDC_SYSTEM_INVALID };

    rv = cdc_op(utc, &tgt,  &b, &add, CDC_OP_COMPLEX_ADD);
    ASSERT_INTEGERS_EQUAL(0, rv, "Offset add failed [5]");
    
    rv = cdc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Offset add result compare failed [5]");
  }
  {
    // Though adding 31 * 86400 gives you 23:59:60 because of the leap second.
    static cdc_calendar_t b = 
      { 1978, CDC_DECEMBER, 1, 0, 0, 0, 0, CDC_SYSTEM_UTC };
    static const char *result = "1978-12-31 23:59:60.000000000 UTC";
    static cdc_calendar_t add = 
      { 0, 0, 0, 0, 0, 31 * 86400, 0, CDC_SYSTEM_INVALID };

    rv = cdc_op(utc, &tgt, &b, &add, CDC_OP_COMPLEX_ADD);
    ASSERT_INTEGERS_EQUAL(0, rv, "Offset add failed [6]");
    
    rv = cdc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Offset add result compare failed [6]");
  }
  
  {
    // Adding 1s to 23:59:58 gives you 23:59:59
    static cdc_calendar_t b = 
      { 1978, CDC_DECEMBER, 31, 23, 59, 58, 0, CDC_SYSTEM_UTC };
    static const char *result = "1978-12-31 23:59:59.000000000 UTC";
    static cdc_calendar_t add = 
      { 0, 0, 0, 0, 0, 1, 0, CDC_SYSTEM_INVALID };

    rv = cdc_op(utc, &tgt, &b, &add, CDC_OP_COMPLEX_ADD);
    ASSERT_INTEGERS_EQUAL(0, rv, "Offset add failed [7]");
    
    rv = cdc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Offset add result compare failed [7]");
  }

  {
    // Adding 2s to 23:59:58 gives you 23:59:60
    static cdc_calendar_t b = 
      { 1978, CDC_DECEMBER, 31, 23, 59, 58, 0, CDC_SYSTEM_UTC };
    static const char *result = "1978-12-31 23:59:60.000000000 UTC";
    static cdc_calendar_t add = 
      { 0, 0, 0, 0, 0, 2, 0, CDC_SYSTEM_INVALID };

    rv = cdc_op(utc, &tgt,  &b, &add, CDC_OP_COMPLEX_ADD);
    ASSERT_INTEGERS_EQUAL(0, rv, "Offset add failed [7]");
    
    rv = cdc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Offset add result compare failed [7]");
  }

  {
    // Adding 3s gives 0:00:01
    static cdc_calendar_t b = 
      { 1978, CDC_DECEMBER, 31, 23, 59, 58, 0, CDC_SYSTEM_UTC };
    static const char *result = "1979-01-01 00:00:00.000000000 UTC";
    static cdc_calendar_t add = 
      { 0, 0, 0, 0, 0, 3, 0, CDC_SYSTEM_INVALID };

    rv = cdc_op(utc, &tgt, &b, &add, CDC_OP_COMPLEX_ADD);
    ASSERT_INTEGERS_EQUAL(0, rv, "Offset add failed [8]");
    
    rv = cdc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Offset add result compare failed [8]");
  }

  {
    // Adding 1s to 23:59:60 gives 00:00:00
    static cdc_calendar_t b = 
      { 1978, CDC_DECEMBER, 31, 23, 59, 60, 0, CDC_SYSTEM_UTC };
    static const char *result = "1979-01-01 00:00:00.000000000 UTC";
    static cdc_calendar_t add = 
      { 0, 0, 0, 0, 0, 1, 0, CDC_SYSTEM_INVALID };

    rv = cdc_op(utc, &tgt, &b, &add, CDC_OP_COMPLEX_ADD);
    ASSERT_INTEGERS_EQUAL(0, rv, "Offset add failed [9]");
    
    rv = cdc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Offset add result compare failed [9]");
  }


  {
    // Adding -1s to 23:59:60 gives 23:59:59
    static cdc_calendar_t b = 
      { 1978, CDC_DECEMBER, 31, 23, 59, 60, 0, CDC_SYSTEM_UTC };
    static const char *result = "1978-12-31 23:59:59.000000000 UTC";
    static cdc_calendar_t add = 
      { 0, 0, 0, 0, 0, -1, 0, CDC_SYSTEM_INVALID };

    rv = cdc_op(utc, &tgt, &b, &add, CDC_OP_COMPLEX_ADD);
    ASSERT_INTEGERS_EQUAL(0, rv, "Offset add failed [10]");
    
    rv = cdc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Offset add result compare failed [10]");
  }

  // Now some diff() tests.
  {
    static cdc_calendar_t b = 
      { 1970, CDC_JANUARY, 2, 0, 0, 0, 0, CDC_SYSTEM_UTC };
    static cdc_calendar_t a = 
      { 1970, CDC_JANUARY, 3, 0, 0, 0, 0, CDC_SYSTEM_UTC };
    cdc_interval_t iv;
    const char *result = "86400 s 0 ns";
    
    rv = cdc_diff(utc, &iv, &b, &a);
    ASSERT_INTEGERS_EQUAL(0, rv, "diff() failed [11]");
    
    rv = cdc_interval_sprintf(buf, 128, &iv);
    ASSERT_STRINGS_EQUAL(buf, result, "diff() result compare failed [11]");
  }

  // There were actually 86401 seconds between 31/12/1975 and 1/1/1976 because
  // of the leap second.
  {
    static cdc_calendar_t b = 
      { 1975, CDC_DECEMBER, 31, 13, 0, 0, 0, CDC_SYSTEM_UTC };
    static cdc_calendar_t a = 
      { 1976, CDC_JANUARY, 1, 13, 0, 0, 0, CDC_SYSTEM_UTC };
    cdc_interval_t iv;
    const char *result = "86401 s 0 ns";
    
    rv = cdc_diff(utc, &iv, &b, &a);
    ASSERT_INTEGERS_EQUAL(0, rv, "diff() failed [11]");
    
    rv = cdc_interval_sprintf(buf, 128, &iv);
    ASSERT_STRINGS_EQUAL(buf, result, "diff() result compare failed [11]");
  }

  {
    static cdc_calendar_t b = 
      { 1975, CDC_DECEMBER, 31, 23, 59, 60, 0, CDC_SYSTEM_UTC };
    static cdc_calendar_t a = 
      { 1976, CDC_JANUARY, 1, 0, 0, 0, 0, CDC_SYSTEM_UTC };
    cdc_interval_t iv;
    const char *result = "1 s 0 ns";
    
    rv = cdc_diff(utc, &iv, &b, &a);
    ASSERT_INTEGERS_EQUAL(0, rv, "diff() failed [12]");
    
    rv = cdc_interval_sprintf(buf, 128, &iv);
    ASSERT_STRINGS_EQUAL(buf, result, "diff() result compare failed [12]");
  }


  rv = cdc_zone_dispose(&utc);
  ASSERT_INTEGERS_EQUAL(0, rv, "Cannot dispose UTC");
}


static void test_utcplus(void)
{
  cdc_zone_t *utcplus;
  static const char *check_utcplus_dest = "UTC+0223";
  static const char *check_utcminus_dest = "UTC-0114";
  const char *utcplus_desc;
  const char *utcminus_desc;
  int rv;
  char buf[128];
  cdc_calendar_t tgt;
  int sys = CDC_SYSTEM_UTCPLUS_ZERO + (2*60 + 23);
  int sys2 = CDC_SYSTEM_UTCPLUS_ZERO - (1*60 + 14);

  utcplus_desc = cdc_describe_system(sys);
  ASSERT_STRINGS_EQUAL(utcplus_desc, check_utcplus_dest, "UTC+ descriptions don't match");

  utcminus_desc = cdc_describe_system(sys2);
  ASSERT_STRINGS_EQUAL(utcminus_desc, check_utcminus_dest, "UTC- descriptions don't match");

  rv = cdc_utcplus_new(&utcplus, 2*60 + 23);
  ASSERT_INTEGERS_EQUAL(0, rv, "Cannot create UTC+ timezone");
  
  
  // Well before any UTC calculations.
  {
    static cdc_calendar_t a_value =
      { 1940, CDC_FEBRUARY, 3,  13, 00, 00, 0, CDC_SYSTEM_GREGORIAN_TAI };
    static const char *result = "1940-02-03 15:23:00.000000000 UTC+0223";

    rv = cdc_zone_raise(utcplus, &tgt, &a_value);
    ASSERT_INTEGERS_EQUAL(0, rv, "Cannot raise TAI to UTCPLUS");
    
    cdc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Raise result compare failed [0]");
  }


  // UTC is 10s back in 1972
  {
    static cdc_calendar_t a_value =
      { 1972, CDC_FEBRUARY, 3,  13, 00, 00, 0, CDC_SYSTEM_GREGORIAN_TAI };
    static const char *result = "1972-02-03 15:22:50.000000000 UTC+0223";

    rv = cdc_zone_raise(utcplus, &tgt, &a_value);
    ASSERT_INTEGERS_EQUAL(0, rv, "Cannot raise TAI to UTCPLUS [1]");
    
    cdc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Raise result compare failed [1]");
  }

  // The 31 Dec 1990 leap second happens on 1 Jan 1991 @ 02:22:59
  {
    static cdc_calendar_t a_value = 
      { 1991, CDC_JANUARY, 1, 02, 22, 60, 0, 0 };
    static const char *result = "1990-12-31 23:59:60.000000000 UTC";
    cdc_zone_t *z;
    
    a_value.system = sys;

    rv = cdc_zone_lower(utcplus, &tgt, &z, &a_value);
    ASSERT_INTEGERS_EQUAL(0, rv, "Cannot lower UTCPLUS to UTC ");
    
    cdc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Lower result compare failed [2]");
  }
  
  // There are some wierd addition rules, obviously ..
  {
    static cdc_calendar_t a_value = 
      { 1990, CDC_DECEMBER, 31, 23, 59, 59, 0};
    static cdc_calendar_t a_second = 
      { 0, 0, 0, 0, 0, 1, 0, CDC_SYSTEM_INVALID };
    static const char *result = "1991-01-01 00:00:00.000000000 UTC+0223";
    
    a_value.system = sys;
    rv = cdc_op(utcplus, &tgt,
		     &a_value, &a_second, CDC_OP_COMPLEX_ADD);
    ASSERT_INTEGERS_EQUAL(rv, 0, "Cannot add 1s [3]");
    
    cdc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "+1s result failed [3]");
  }

  {
    static cdc_calendar_t a_value = 
      { 1991, CDC_JANUARY, 1, 02, 22, 59, 0};
    static cdc_calendar_t a_second = 
      { 0, 0, 0, 0, 0, 1, 0, CDC_SYSTEM_INVALID };
    static const char *result = "1991-01-01 02:22:60.000000000 UTC+0223";
    
    printf("***\n");

    a_value.system = sys;
    rv = cdc_op(utcplus, &tgt, 
		     &a_value, &a_second, 
		     CDC_OP_COMPLEX_ADD);
    ASSERT_INTEGERS_EQUAL(rv, 0, "Cannot add 1s [4]");
    
    cdc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "+1s result failed [4]");
  }

  {
    static cdc_calendar_t a_value = 
      { 1991, CDC_JANUARY, 1, 02, 22, 59, 0};
    static cdc_calendar_t a_second = 
      { 0, 0, 0, 0, 0, 2, 0, CDC_SYSTEM_INVALID };
    static const char *result = "1991-01-01 02:23:00.000000000 UTC+0223";
    
    printf("***\n");

    a_value.system = sys;
    rv = cdc_op(utcplus, &tgt, 
		     &a_value, &a_second, 
		     CDC_OP_COMPLEX_ADD);
    ASSERT_INTEGERS_EQUAL(rv, 0, "Cannot add 2s [4]");
    
    cdc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "+1s result failed [4]");
  }

  {
    static cdc_calendar_t a_value = 
      { 1990, CDC_DECEMBER, 31, 13, 0, 0, 0 };
    static cdc_calendar_t a_month = 
      { 0, 2, 0, 0, 0, 0, 0, CDC_SYSTEM_OFFSET };
    static const char *result = "1991-03-03 13:00:00.000000000 UTC+0223";
    printf("***\n");

    a_value.system = sys;
    rv = cdc_op(utcplus, &tgt, 
		     &a_value, &a_month, 
		     CDC_OP_COMPLEX_ADD);
    ASSERT_INTEGERS_EQUAL(rv, 0, "Cannot add 1m [5]");
    
    cdc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "+1m result failed [5]");
  }
    
  rv = cdc_zone_dispose(&utcplus);
  ASSERT_INTEGERS_EQUAL(0, rv, "Cannot dispose UTC+");
}

static void test_bst(void)
{
  cdc_zone_t *bst;
  static const char *check_bst_desc = "BST";
  const char *bst_desc;
  int rv;
  cdc_calendar_t tgt;
  char buf[128];

  rv = cdc_bst_new(&bst);
  ASSERT_INTEGERS_EQUAL(0, rv, "Cannot create bst.");

  bst_desc = cdc_describe_system(CDC_SYSTEM_BST);
  ASSERT_STRINGS_EQUAL(check_bst_desc, bst_desc, "BST descriptions don't match");
  
  // 1 Jan 1980 is basically the same as it always was.
  {
    static cdc_calendar_t a_value = 
      { 1980, CDC_JANUARY, 1, 01, 0, 0, 0, CDC_SYSTEM_GREGORIAN_TAI };
    // Remember the UTC - TAI correction .. 
    static const char *result = "1980-01-01 00:59:41.000000000 BST";

    rv = cdc_zone_raise(bst, &tgt, &a_value);
    ASSERT_INTEGERS_EQUAL(0, rv, "Cannot raise TAI to BST [0]");
    
    cdc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Raise result compare failed [0]");
  }

  {
    static cdc_calendar_t a_value = 
      { 1980, CDC_JANUARY, 1, 01, 0, 0, 0, CDC_SYSTEM_UTC };
    // Remember the UTC - TAI correction .. 
    static const char *result = "1980-01-01 01:00:00.000000000 BST";

    rv = cdc_zone_raise(bst, &tgt, &a_value);
    ASSERT_INTEGERS_EQUAL(0, rv, "Cannot raise UTC to BST [1]");
    
    cdc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Raise result compare failed [1]");
  }

  // In 2010: last sunday in March = 28
  // last sunday in October = 31
  printf(" ----------------- \n");
  {
    static cdc_calendar_t a_value = 
      { 2010, CDC_MARCH, 28, 13, 0, 0, 0, CDC_SYSTEM_UTC };
    static const char *result = "2010-03-28 14:00:00.000000000 BST";

    rv = cdc_zone_raise(bst, &tgt, &a_value);
    ASSERT_INTEGERS_EQUAL(0, rv, "Cannot raise UTC to BST [2]");
    
    cdc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Raise result compare failed [2]");
  }
    
  printf(" ----------------- \n");

  // BST starts at 0100, so 0059 UTC -> 0059 BST
  {
    static cdc_calendar_t a_value = 
      { 2010, CDC_MARCH, 28, 00, 59, 59, 0, CDC_SYSTEM_UTC };
    static const char *result = "2010-03-28 00:59:59.000000000 BST";

    rv = cdc_zone_raise(bst, &tgt, &a_value);
    ASSERT_INTEGERS_EQUAL(0, rv, "Cannot raise UTC to BST [2]");
    
    cdc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Raise result compare failed [2]");
  }

  // 0200 BST is 0100 UTC.
  {
    static cdc_calendar_t a_value = 
      { 2010, CDC_MARCH, 28, 02, 00, 00, 0, CDC_SYSTEM_BST };
    static const char *result = "2010-03-28 01:00:00.000000000 UTC";
    cdc_zone_t *z;

    rv = cdc_zone_lower(bst, &tgt, &z, &a_value);
    ASSERT_INTEGERS_EQUAL(0, rv, "Cannot lower BST to UTC [3]");
    
    cdc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Lower result compare failed [3]");
  }

  // However, there is 1s of elapsed time between 00:59:59 BST and 02:00:00 BST.
  {
    static cdc_calendar_t a_value = 
      { 2010, CDC_MARCH, 28, 00, 59, 59, 0, CDC_SYSTEM_BST };
    static cdc_calendar_t b_value = 
      { 2010, CDC_MARCH, 28, 02, 0, 0, 0, CDC_SYSTEM_BST };
    cdc_interval_t iv;
    static const char *result = "1 s 0 ns";

    rv = cdc_diff(bst, &iv, &a_value, &b_value);
    ASSERT_INTEGERS_EQUAL(0, rv, "Cannot measure interval [4]");
    
    cdc_interval_sprintf(buf, 128, &iv);
    ASSERT_STRINGS_EQUAL(buf, result, "Interval result compare failed [4]");
  }

  // Now, a tricky case.
  {
    // The last Sunday in October 2020 is 25th.
    static cdc_calendar_t a_value = 
      { 2020, CDC_OCTOBER, 26, 00, 59, 59, 0, CDC_SYSTEM_UTC };
    static const char *result = "2020-10-26 00:59:59.000000000 BST";

    rv = cdc_zone_raise(bst, &tgt, &a_value);
    ASSERT_INTEGERS_EQUAL(0, rv, "Cannot raise UTC to BST [5]");
    
    cdc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Raise result compare failed [5]");
  }

  {
    // The last Sunday in October 2020 is 25th.
    static cdc_calendar_t a_value = 
      { 2020, CDC_OCTOBER, 24, 00, 59, 59, 0, CDC_SYSTEM_UTC };
    static const char *result = "2020-10-24 01:59:59.000000000 BST";

    rv = cdc_zone_raise(bst, &tgt, &a_value);
    ASSERT_INTEGERS_EQUAL(0, rv, "Cannot raise UTC to BST [5]");
    
    cdc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Raise result compare failed [5]");
  }

  // Add 1s to a totally uncontroversial time.
  {
    static cdc_calendar_t a_value = 
      { 1983, CDC_DECEMBER, 1, 0, 59, 59, 0, CDC_SYSTEM_BST };
    static cdc_calendar_t one_second = 
      { 0, 0, 0, 0, 0, 1, 0, CDC_SYSTEM_OFFSET };
    static const char *result = "1983-12-01 01:00:00.000000000 BST";
    
    rv = cdc_op(bst, &tgt, &a_value, &one_second, CDC_OP_COMPLEX_ADD);
    ASSERT_INTEGERS_EQUAL(0, rv, "Cannot add 1s to BST time [6]");

    cdc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Add result compare failed [6]");
  }

  // .. and whilst BST is on.
  {
    static cdc_calendar_t a_value = 
      { 1983, CDC_APRIL, 1, 23, 59, 59, 0, CDC_SYSTEM_BST };
    static cdc_calendar_t one_second = 
      { 0, 0, 0, 0, 0, 1, 0, CDC_SYSTEM_OFFSET };
    static const char *result = "1983-04-02 00:00:00.000000000 BST";
    
    rv = cdc_op(bst, &tgt, &a_value, &one_second, CDC_OP_COMPLEX_ADD);
    ASSERT_INTEGERS_EQUAL(0, rv, "Cannot add 1s to BST time [7]");

    cdc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Add result compare failed [7]");
  }
  

  printf("**************\n");
  // The UTC leap second in June 1983 happened in BST, apparently at 0:59:59 on
  // 1 July.
  {
    static cdc_calendar_t a_value = 
      { 1983, CDC_JULY, 1, 0, 59, 59, 0, CDC_SYSTEM_BST };
    static cdc_calendar_t one_second = 
      { 0, 0, 0, 0, 0, 1, 0, CDC_SYSTEM_OFFSET };
    static const char *result = "1983-07-01 00:59:60.000000000 BST";
    
    rv = cdc_op(bst, &tgt, &a_value, &one_second, CDC_OP_COMPLEX_ADD);
    ASSERT_INTEGERS_EQUAL(0, rv, "Cannot add 1s to BST time [7]");

    cdc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Add result compare failed [7]");
  }

  {
    static cdc_calendar_t a_value =
      { 1984, CDC_JULY, 1, 02, 00, 00, 0, CDC_SYSTEM_BST };
    static const char *result = "1984-07-01 01:00:22.000000000 TAI";
    cdc_zone_t *z;

    rv = cdc_zone_lower_to(bst, &tgt, &z, &a_value, CDC_SYSTEM_GREGORIAN_TAI);
    ASSERT_INTEGERS_EQUAL(0, rv, "Cannot convert to TAI [8]");
    
    cdc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "TAI conversion compare failed [8]");
  }

  rv = cdc_zone_dispose(&bst);
  ASSERT_INTEGERS_EQUAL(0, rv, "Cannot dispose() bst");
}

static void test_rebased(void)
{
  cdc_zone_t *rb;
  static const char *check_rb_desc = "REBASED*";
  const char *rb_desc;
  int rv;
  cdc_calendar_t tgt;
  char buf[128];
  static const cdc_calendar_t offset = 
    { 0, 0, 0,  -1, -14, -3, 0,  CDC_SYSTEM_OFFSET };
   
  cdc_zone_t *tai;

  rv = cdc_tai_new(&tai);
  ASSERT_INTEGERS_EQUAL(rv, 0, "Cannot create UTC timezone");

  // We check TAI - 1hr 14m 3s
  rv = cdc_rebased_new(&rb, &offset, tai);
  ASSERT_INTEGERS_EQUAL(rv, 0, "Cannot create offset zone");

  rb_desc = cdc_describe_system(rb->system);
  ASSERT_STRINGS_EQUAL(check_rb_desc, rb_desc, "Rebased descriptions don't work");

  {
    static cdc_calendar_t a_value = 
      { 1980, CDC_JANUARY, 1, 0, 0, 0, CDC_SYSTEM_GREGORIAN_TAI };
    static const char *result = "1979-12-31 22:45:57.000000000 REBASED*";

    rv = cdc_zone_raise(rb, &tgt, &a_value);
    ASSERT_INTEGERS_EQUAL(rv, 0, "Cannot raise [0]");
    
    cdc_calendar_sprintf(buf, 128, &tgt);
    ASSERT_STRINGS_EQUAL(buf, result, "Raise result compare failed [0]");

  }
  
  {
    static cdc_calendar_t a_value = 
      { 1979, CDC_DECEMBER, 31, 0, 0, 0, 0, CDC_SYSTEM_REBASED };
    static cdc_calendar_t b_value = 
      { 1979, CDC_DECEMBER, 30, 23, 59, 59, 0, CDC_SYSTEM_REBASED };
    cdc_interval_t iv;
    static const char *result = "-1 s 0 ns";

    rv = cdc_diff(rb, &iv, &a_value, &b_value);
    ASSERT_INTEGERS_EQUAL(rv, 0, "Cannot diff [1]");
    
    cdc_interval_sprintf(buf, 128, &iv);
    ASSERT_STRINGS_EQUAL(buf, result, "diff result compare failed [1]");
  }
    

    
  rv = cdc_zone_dispose(&rb);
  ASSERT_INTEGERS_EQUAL(0, rv, "Cannot dispose() rebased");

  rv = cdc_zone_dispose(&tai);
  ASSERT_INTEGERS_EQUAL(0, rv, "Cannot dispose() tai");

}

static void test_bounce(void)
{
  cdc_zone_t *bst;
  cdc_zone_t *offset;
  int rv;
  char buf[128];

  {
    // 1 hr
    cdc_calendar_t human = 
      { 2010, CDC_JUNE, 4, 12, 23, 04, 0, CDC_SYSTEM_BST };
    cdc_calendar_t computer =
      { 2010, CDC_JUNE, 4, 12, 23, 04, 0, CDC_SYSTEM_GREGORIAN_TAI };
    cdc_calendar_t tgt;
    
    rv = cdc_bst_new(&bst);
    ASSERT_INTEGERS_EQUAL(0, rv, "Cannot construct bst");
    
    rv = cdc_rebased_tai(&offset, bst, &human, &computer);
    ASSERT_INTEGERS_EQUAL(0, rv, "Cannot construct offset");
    
    {
      static cdc_calendar_t a = 
	{ 2010, CDC_NOVEMBER, 5, 15, 00, 00, 0, CDC_SYSTEM_BST };
      // Actually an hour ahead because BST has ticked back an hour at the end
      // of BST, but the computer clock has just carried on ticking.
      static const char *result = "2010-11-05 16:00:00.000000000 REBASED*";
      
      rv = cdc_bounce(bst, offset, &tgt, &a);
      ASSERT_INTEGERS_EQUAL(0, rv, "Cannot bounce time [0]");
      
      cdc_calendar_sprintf(buf, 128, &tgt);
      ASSERT_STRINGS_EQUAL(buf, result, "Bounce time check failed [0]");
    }
  }

  {
    // 1yr 5 months, 3 days, 12 hours, 23 minutes, 04 s
    cdc_calendar_t human = 
      { 2010, CDC_JUNE, 4, 12, 23, 04, 0, CDC_SYSTEM_BST };
    cdc_calendar_t computer =
      { 2009, CDC_JANUARY, 1, 0, 0, 0, 0, CDC_SYSTEM_GREGORIAN_TAI };
    cdc_calendar_t tgt;
    
    rv = cdc_bst_new(&bst);
    ASSERT_INTEGERS_EQUAL(0, rv, "Cannot construct bst");
    
    rv = cdc_rebased_tai(&offset, bst, &human, &computer);
    ASSERT_INTEGERS_EQUAL(0, rv, "Cannot construct offset");
    
    {
      // We are actually one hour ahead, but so was the original human
      // time, so the apparent difference is what you see above, and
      // we should expect 2009 - 01 - 02 02:36:56.
      static cdc_calendar_t a = 
	{ 2010, CDC_JUNE, 5, 15, 00, 00, 0, CDC_SYSTEM_BST };
      static const char *result = "2009-01-02 02:36:56.000000000 REBASED*";
      
      rv = cdc_bounce(bst, offset, &tgt, &a);
      ASSERT_INTEGERS_EQUAL(0, rv, "Cannot bounce time [0]");
      
      cdc_calendar_sprintf(buf, 128, &tgt);
      ASSERT_STRINGS_EQUAL(buf, result, "Bounce time check failed [0]");
    }

    printf("------------\n");
    {
      // 1y 5m 4d 15h after the original time point.
      // => we should expect 2011-11-08 03:23:04 -1hr for BST.
      // However, the number of days between Jan and Jun is one
      // less than between Jun and Nov, so in fact we get 2011-11-07
      static cdc_calendar_t a = 
	{ 2010, CDC_JUNE, 5, 15, 00, 00, 0, CDC_SYSTEM_REBASED };
      static const char *result = "2011-11-07 02:23:04.000000000 BST";

      rv = cdc_bounce(offset, bst, &tgt, &a);
      ASSERT_INTEGERS_EQUAL(0, rv, "Cannot bounce time [1]");
      
      cdc_calendar_sprintf(buf, 128, &tgt);
      ASSERT_STRINGS_EQUAL(buf, result, "Bounce time check failed [1]");
    }

    printf("------------\n");
    {
      // .. and now ahead by an hour because we've landed in BST.
      static cdc_calendar_t a = 
	{ 2010, CDC_OCTOBER, 5, 15, 00, 00, 0, CDC_SYSTEM_REBASED };
      static const char *result = "2012-03-08 03:23:04.000000000 BST";

      rv = cdc_bounce(offset, bst, &tgt, &a);
      ASSERT_INTEGERS_EQUAL(0, rv, "Cannot bounce time [2]");
      
      cdc_calendar_sprintf(buf, 128, &tgt);
      ASSERT_STRINGS_EQUAL(buf, result, "Bounce time check failed [2]");
    }

  }



  


  rv = cdc_zone_dispose(&offset);
  ASSERT_INTEGERS_EQUAL(0, rv, "Cannot dispose() offset");
  rv = cdc_zone_dispose(&bst);
  ASSERT_INTEGERS_EQUAL(0, rv, "Cannot dispose() bst");
}


static void faili(const cdc_interval_t *a,
		  const cdc_interval_t *b,
		  const char *leg1,
		  const char *leg2,
		  const char *file,
		  const int line,
		  const char *func)
{
  char bx[128], bx2[128];
  cdc_interval_sprintf(bx, 128, a);
  cdc_interval_sprintf(bx2, 128, b);

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

