/* timecalc.c */
/* (C) Kynesim Ltd 2010 */

/*
 *  This file is hereby placed in the public domain; do what you like
 *  with it. But please don't introduce yet more spurious time related
 *  bugs into the world!
 *
 */

#include <stdint.h>
#include "timecalc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ONE_MILLION 1000000
#define ONE_BILLION (ONE_MILLION * 1000)

//! @todo Could use gcc extensions to evaluate only once ..
#define MIN(x,y) (((x) < (y)) ? (x) : (y))
#define MAX(x,y) (((x) > (y)) ? (x) : (y))
#define SWAP(x,y) { __typeof(x) __tmp; __tmp = (x); (x) = (y); (y) = __tmp; }


static void null_init(struct timecalc_zone_struct *self, void *arg);
static void null_dispose(struct timecalc_zone_struct *self);

static int system_gtai_diff(struct timecalc_zone_struct *self,
			    timecalc_interval_t *ival,
			    const timecalc_calendar_t *before,
			    const timecalc_calendar_t *after);

static int system_gtai_normalise(struct timecalc_zone_struct *self,
				 timecalc_calendar_t *io_cal);

static int system_gtai_aux(struct timecalc_zone_struct *self,
			   const timecalc_calendar_t *calc,
			   timecalc_calendar_aux_t *aux);


static int system_gtai_epoch(struct timecalc_zone_struct *self,
			     timecalc_calendar_t *aux);


static timecalc_zone_t s_system_gtai = 
  {
    NULL,
    null_init,
    null_dispose,
    system_gtai_diff,
    system_gtai_normalise,
    system_gtai_aux,
    system_gtai_epoch
  };
  

static inline int is_gregorian_leap_year(int yr)
{
  // "Every year that is exactly divisible by 4 is a leap year,
  //   except years which are exactly divisible by 100. Years
  //   divisible by 400 are still leap years."
  return (!(yr%400) ? 
	  1 : 
	  (!(yr%4) ? !!(yr%100) : 0));
}

static int gregorian_months[] = 
  { 
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 
  };

#define GREGORIAN_DAYS_IN_YEAR (365)
#define SECONDS_PER_MINUTE (60)
#define MINUTES_PER_HOUR   (60)
#define HOURS_PER_DAY      (24)

#define SECONDS_PER_DAY    (SECONDS_PER_MINUTE * MINUTES_PER_HOUR * HOURS_PER_DAY)
#define SECONDS_PER_YEAR   (GREGORIAN_DAYS_IN_YEAR * SECONDS_PER_DAY)

int timecalc_interval_add(timecalc_interval_t *result,
			  const timecalc_interval_t *a,
			  const timecalc_interval_t *b)
{
  int64_t tmp = (a->ns + b->ns);

  // The C standard doesn't specify the sign of the result
  // of % applied to -ve numbers :-(
  result->ns = (tmp < 0) ? -((-tmp) % ONE_BILLION) : 
    (tmp%ONE_BILLION);
  result->s = (a->s + b->s + (tmp / ONE_BILLION) );
  return 0;
}

int timecalc_interval_subtract(timecalc_interval_t *result,
			       const timecalc_interval_t *a,
			       const timecalc_interval_t *b)
{
  int64_t s_off = 0;

  result->ns = (a->ns - b->ns);
  s_off = (result->ns / ONE_BILLION );
  if (result->ns < 0)
    {
      result->ns = -((-result->ns) % ONE_BILLION);
    }
  else
    {
      result->ns = (result->ns % ONE_BILLION);
    }

  result->s = (a->s - b->s) + s_off;
  return 0;
}

int timecalc_zone_new(int system,
		      timecalc_zone_t **out_zone,
		      void *arg)
{
  timecalc_zone_t *prototype = NULL;

  switch (system)
    {
    case TIMECALC_SYSTEM_GREGORIAN_TAI:
      prototype = &s_system_gtai;
      break;
    }
  if (prototype == NULL)
    {
      (*out_zone) = NULL;
      return TIMECALC_ERR_NO_SUCH_SYSTEM;
    }

  {
    timecalc_zone_t *z = 
      (timecalc_zone_t *)malloc(sizeof(timecalc_zone_t));

    memcpy(z, prototype, sizeof(timecalc_zone_t));
    z->init(z, arg);
    (*out_zone) = z;
  }

  return 0;
}

int timecalc_zone_dispose(timecalc_zone_t **io_zone)
{
  if (!io_zone || !(*io_zone)) { return 0; }
  {
    timecalc_zone_t *t = *io_zone;
    t->dispose(t);
    free(t);
  }
  (*io_zone) = NULL;
  return 0;
}

int timecalc_calendar_cmp(const timecalc_calendar_t *a,
			  const timecalc_calendar_t *b)
{
#define CVAL_HELPER(x,y) \
  if ((x) < (y)) { return -1; }			\
  if ((x) > (y)) { return 1; }

  CVAL_HELPER(a->year, b->year);
  CVAL_HELPER(a->month, b->month);
  CVAL_HELPER(a->mday, b->mday);
  CVAL_HELPER(a->hour, b->hour);
  CVAL_HELPER(a->minute, b->minute);
  CVAL_HELPER(a->second, b->second);
  CVAL_HELPER(a->ns, b->ns);

  // Exists only to make equality an equivalence relation on 
  // timecalc_calendar_t s
  CVAL_HELPER(a->system, b->system);
#undef CVAL_HELPER

  return 0;
}

int timecalc_interval_cmp(const timecalc_interval_t *a,
			  const timecalc_interval_t *b)
{
  if (a->s < b->s) { return -1; }
  if (a->s > b->s) { return 1; }
  if (a->ns < b->ns) { return -1; }
  if (a->ns > b->ns) { return 1; }
  return 0;
}

int timecalc_zone_add(timecalc_zone_t *zone,
		      timecalc_calendar_t *out,
		      const timecalc_calendar_t *date,
		      const timecalc_interval_t *ival)
{
  int rv;
  int64_t s;


  memcpy(out, date, sizeof(timecalc_calendar_t));

  s = ival->s;

  while (s > (1 << 30))
    {
      out->second += (1<<30);
      rv = zone->normalise(zone, out);
      if (rv) { return rv; }
      s -= (1<<30);
    }
  while (s < -(1<<30))
    {
      out->second -= (1<<30);
      rv = zone->normalise(zone, out);
      if (rv) { return rv; }
      s += (1<<30);
    }

  out->ns += ival->ns;
  out->second += s;
  rv = zone->normalise(zone, out);
  return rv;
}

int timecalc_interval_sprintf(char *buf,
			      int n,
			      const timecalc_interval_t *a)
{
  return snprintf(buf, n, 
		  "%lld s %ld ns",
		  a->s, a->ns);
}

int timecalc_calendar_sprintf(char *buf,
			      int n,
			      const timecalc_calendar_t *date)
{
  return snprintf(buf, n, 
		  "%04d-%02d-%02d %02d:%02d:%02d.%09ld %s",
		  date->year,
		  date->month+1,
		  date->mday,
		  date->hour,
		  date->minute,
		  date->second,
		  date->ns,
		  timecalc_describe_system(date->system));
}

int timecalc_interval_sgn(const timecalc_interval_t *a)
{
  if (a->s > 0) { return 1; }
  if (a->s < 0) { return -1; }
  if (a->ns > 0) { return 1; }
  if (a->ns < 0) { return -1; }
  return 0;
}


const char *timecalc_describe_system(const int system)
{
  switch (system)
    {
    case TIMECALC_SYSTEM_GREGORIAN_TAI:
      return "TAI";
      break;
    default:
      return "UNKNOWN";
      break;
    }
}


/* -------------------- Generic NULL functions -------- */
static void null_init(struct timecalc_zone_struct *self, void *arg)
{
  return;
}

static void null_dispose(struct timecalc_zone_struct *self)
{
  return;
}

/* -------------------- Gregorian TAI ---------------- */

static int system_gtai_diff(struct timecalc_zone_struct *self,
			    timecalc_interval_t *ivalp,
			    const timecalc_calendar_t *before,
			    const timecalc_calendar_t *after)
{
  /** @todo
   *  
   *  We're using the conventional 'spinning counter' algorithm
   *  here. This is gratuitously terrible.
   */

  if (before->system != after->system) { return TIMECALC_ERR_SYSTEMS_DO_NOT_MATCH; }
  if (before->system != TIMECALC_SYSTEM_GREGORIAN_TAI) 
    {
      return TIMECALC_ERR_NOT_MY_SYSTEM;
    }

  // Should we invert the sense of the result?
  if (timecalc_calendar_cmp(before, after) > 0)
    {
      return system_gtai_diff(self, ivalp, after, before);
    }

  // Bring the interval onto the stack for manipulation
  // (also helps gcc to optimise this code)
  
  timecalc_interval_t ival;
  memcpy(&ival, ivalp, sizeof(timecalc_interval_t));

  {
    int cur = before->year;
    int last = after->year;
    
    for ( ; cur != last; ++cur)
      {
	ival.s += SECONDS_PER_YEAR;
	if (is_gregorian_leap_year(cur)) 
	  {
	    // February has an extra day.
	    ival.s += SECONDS_PER_DAY;
	  }
      }
  }

  {
    int cur = before->month;
    int last = after->month;

    for (; cur != last; ++cur)
      {
	ival.s += SECONDS_PER_DAY * gregorian_months[cur];
      }
  }

  ival.s += SECONDS_PER_DAY * (after->mday - before->mday);
  ival.ns = 0;
  
  memcpy(ivalp, &ival, sizeof(timecalc_interval_t));

  return 0;
}

static int system_gtai_normalise(struct timecalc_zone_struct *self,
				 timecalc_calendar_t *io_cal)
{
  int done = 0;

  // Convert any negatives to positives ..
  while (io_cal->ns < 0)
    {
      io_cal->second -= ONE_BILLION;
      io_cal->ns += ONE_BILLION;
    }
  while (io_cal->second < 0)
    {
      --io_cal->minute;
      io_cal->second += SECONDS_PER_MINUTE;
    }

  while (io_cal->minute < 0)
    {
      --io_cal->hour;
      io_cal->hour += MINUTES_PER_HOUR;
    }

  while (io_cal->hour < 0)
    {
      --io_cal->mday;
      io_cal->hour += HOURS_PER_DAY;
    }

  while (io_cal->month < 0)
    {
      --io_cal->year;
      io_cal->month += 12;
    }
  
  while (io_cal->mday < 1)
    {
      // Actually in the previous month. Ugh.
      --io_cal->month;
      if (io_cal->month < 0)
	{
	  --io_cal->year;
	  io_cal->month += 12;
	}
      
      io_cal->mday += (gregorian_months[io_cal->month]);
      if (io_cal->month == 1 &&
	  is_gregorian_leap_year(io_cal->year))
	{
	  ++io_cal->mday; // Leap year correction
	}
    }
  
  // We can do the time independently of date.
  io_cal->second += (io_cal->ns / ONE_BILLION);
  io_cal->ns = (io_cal->ns % ONE_BILLION);

  io_cal->minute += (io_cal->second / SECONDS_PER_MINUTE);
  io_cal->second = (io_cal->second % SECONDS_PER_MINUTE);

  io_cal->hour += (io_cal->minute / MINUTES_PER_HOUR);
  io_cal->minute = (io_cal->minute % MINUTES_PER_HOUR);

  io_cal->mday += (io_cal->hour / HOURS_PER_DAY);
  io_cal->hour = (io_cal->hour % HOURS_PER_DAY);

  // Now the tricky part .. 
  while (!done)
    {
      // Make sure the month is valid.
      while (io_cal->month > 11)
	{
	  ++io_cal->year; 
	  io_cal->month -= 12;
	}
      
      int nr_mday;
      
      // Adjust mday
      nr_mday = (gregorian_months[io_cal->month]);

      if (io_cal->month == 1 && 
	  is_gregorian_leap_year(io_cal->year))
	{
	  ++nr_mday;
	}


      if (io_cal->mday > nr_mday)
	{
	  io_cal->mday -= nr_mday;
	  ++io_cal->month;
	  continue;
	}

      // We're valid.
      ++done;
    }

  return 0; 
}

static int system_gtai_aux(struct timecalc_zone_struct *self,
			   const timecalc_calendar_t *cal,
			   timecalc_calendar_aux_t *aux)
{
  // There is no DST in TAI
  aux->is_dst = 0;

  {
    int yday = 0;
    int i;
    for (i = 0; i < cal->month; ++i)
      {
	yday += gregorian_months[i];
	// Leap year correction
	if (i == 1 && is_gregorian_leap_year(cal->year)) { ++yday; }
      }
    yday += cal->mday;
    aux->yday = yday;
  }

  // The century start day of week follows a 4,2,0,6 pattern. 17XX == 4 */
  static const int cstart[] = { 4,2,0, 6};
  static const int wday[] = { 0, 3, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5 };
  static const int leap_wday[] = { 6, 2, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5 };
 int dow;

  // &3 is here to make sure the result is +ve.
  int idx = ((cal->year / 100) - 17)&3;
  int yoff = (cal->year % 100);

  dow = cstart[idx] + yoff + (yoff / 4) + 
    (is_gregorian_leap_year(cal->year) ? leap_wday[cal->month] : 
     wday[cal->month]) + (cal->mday - 1) ;
  aux->wday = (dow % 7);

  return 0;
}

static int system_gtai_epoch(struct timecalc_zone_struct *self,
			     timecalc_calendar_t *cal)
{
  // The 'epoch' for TAI is, rather arbitrarily, 1 January 1958
  cal->year = 1958;
  cal->month = 0;
  cal->mday = 1;
  cal->hour = 0;
  cal->minute = 0;
  cal->second = 0;
  cal->ns = 0;
  cal->system = TIMECALC_SYSTEM_GREGORIAN_TAI;
  
  return 0;
}


/* End file */
