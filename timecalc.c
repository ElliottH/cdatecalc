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


static int null_init(struct timecalc_zone_struct *self, void *arg);
static int null_dispose(struct timecalc_zone_struct *self);

static int system_gtai_diff(struct timecalc_zone_struct *self,
			    timecalc_interval_t *ival,
			    const timecalc_calendar_t *before,
			    const timecalc_calendar_t *after);

static int system_gtai_offset(struct timecalc_zone_struct *self,
			      timecalc_calendar_t *offset,
			      const timecalc_calendar_t *src,
			      int *leap_second);

static int system_gtai_aux(struct timecalc_zone_struct *self,
			   const timecalc_calendar_t *calc,
			   timecalc_calendar_aux_t *aux);

static int system_gtai_normalise(struct timecalc_zone_struct *self,
				 timecalc_calendar_t *io_cal);


static int system_gtai_epoch(struct timecalc_zone_struct *self,
			     timecalc_calendar_t *aux);

static int system_gtai_lower_zone(struct timecalc_zone_struct *self,
				  struct timecalc_zone_struct **next);




static timecalc_zone_t s_system_gtai = 
  {
    NULL,
    TIMECALC_SYSTEM_GREGORIAN_TAI,
    null_init,
    null_dispose,
    system_gtai_diff,
    system_gtai_offset,
    system_gtai_normalise,
    system_gtai_aux,
    system_gtai_epoch,
    system_gtai_lower_zone
  };

/* UTC: Applies UTC corrections to TAI
 */

// A table of calendar dates mapping to the difference between
// UTC and TAI.
typedef struct utc_lookup_entry_struct
{
  timecalc_calendar_t when;
 
  //! Value of utc-tai (i.e. add this to TAI to get UTC)
  timecalc_interval_t utctai;
 
} utc_lookup_entry_t;

/* Taken from the NIST's page at
 * http://tf.nist.gov/pubs/bulletin/leapsecond.htm
 *
 * All leap seconds so far have been +ve leap seconds
 * (i.e. there was a 60)
 *
 * To use this table:
 *
 *   - Compute your date in TAI.
 *   - Read up the table, counting leap seconds and adding them
 *      to your time as you go.
 *   - If you are earlier than all entries, there are no leap seconds
 *       (UTC doesn't exist yet)
 *   - If you are later than entry A but earlier than B, you're done.
 *   - If you are entry A, add that leap second. If it's a +ve leap second,
 *     jump ahead. If a -ve one, congratulations! You've got s = 60.
 *
 * The irregular initial time jumps compensate to some extent for the
 *  difference in the length of a second between SI and UTC between 1961 and 
 *  1972.
 */
static utc_lookup_entry_t utc_lookup_table[] =
  {

    // A dummy 0 entry to make conv easier to write.
    { { 0, 0, 0, 0, 0, 0, 0, TIMECALC_SYSTEM_UTC },
      { 0, 0 } 
    },

    // midnight 1 Jan 1961 UTC was TAI 1 Jan 1961 00:00:01.422818
    { { 1961, TIMECALC_JANUARY, 1, 0, 0, 0, 0, TIMECALC_SYSTEM_UTC },
      { -1,
	-422818000
      } 
    },

    // This is The Confused Period where the UTC second and the SI
    // second disagreed.

    // midnight 1 Jan 1972  UTC was TAI 1 Jan 1972 00:00:10 
    {
      { 1972, TIMECALC_JANUARY, 1, 0, 0, 0, 0, TIMECALC_SYSTEM_UTC },
      { -10, 0 }
    },

    // This is the start of leap second calculation
#define UTC_LOOKUP_MIN_LEAP_SECOND 3


    // The June 1972 leap second
    { { 1972, TIMECALC_JULY, 1, 0, 0, 0, 0, TIMECALC_SYSTEM_UTC }, 
      { -11, 0 }
    },

    { { 1973, TIMECALC_JANUARY, 1, 0, 0, 0, 0, TIMECALC_SYSTEM_UTC },
      { -12, 0 } 
    },

    { { 1974, TIMECALC_JANUARY, 1, 0, 0, 0, 0, TIMECALC_SYSTEM_UTC },
      { -13, 0 }
    },

    { { 1975, TIMECALC_JANUARY, 1, 0, 0, 0, 0, TIMECALC_SYSTEM_UTC },
      { -14, 0 }
    },
    
    { { 1976, TIMECALC_JANUARY, 1, 0, 0, 0, 0, TIMECALC_SYSTEM_UTC },
      { -15, 0 }
    },
    
    { { 1977, TIMECALC_JANUARY, 1, 0, 0, 0, 0, TIMECALC_SYSTEM_UTC },
      { -16, 0 }
    },

    { { 1978, TIMECALC_JANUARY, 1, 0, 0, 0, 0, TIMECALC_SYSTEM_UTC },
      { -17, 0 }
    },
    
    { { 1979, TIMECALC_JANUARY, 1, 0, 0, 0, 0, TIMECALC_SYSTEM_UTC },
      { -18, 0 }
    },
    
    { { 1980, TIMECALC_JANUARY, 1, 0, 0, 0, 0, TIMECALC_SYSTEM_UTC },
      { -19, 0 }
    },
    
    { { 1981, TIMECALC_JULY, 1, 0, 0, 0, 0, TIMECALC_SYSTEM_UTC },
      { -20, 0 }
    },

    { { 1982, TIMECALC_JULY, 1, 0, 0, 0, 0, TIMECALC_SYSTEM_UTC },
      { -21, 0 }
    },

    { { 1983, TIMECALC_JULY, 1, 0, 0, 0, 0, TIMECALC_SYSTEM_UTC },
      { -22, 0 }
    },

    { { 1985, TIMECALC_JULY, 1, 0, 0, 0, 0, TIMECALC_SYSTEM_UTC },
      { -23, 0 }
    },
    
    { { 1988, TIMECALC_JANUARY, 1, 0, 0, 0, 0, TIMECALC_SYSTEM_UTC },
      { -24, 0 }
    },

    { { 1990, TIMECALC_JANUARY, 1, 0, 0, 0, 0, TIMECALC_SYSTEM_UTC },
      { -25, 0 }
    },

    { { 1991, TIMECALC_JANUARY, 1, 0, 0, 0, 0, TIMECALC_SYSTEM_UTC },
      { -26, 0 }
    },

    { { 1992, TIMECALC_JULY, 1, 0, 0, 0, 0, TIMECALC_SYSTEM_UTC },
      { -27, 0 }
    },

    { { 1993, TIMECALC_JULY, 1, 0, 0, 0, 0, TIMECALC_SYSTEM_UTC },
      { -28, 0 }
    },

    { { 1994, TIMECALC_JULY, 1, 0, 0, 0, 0, TIMECALC_SYSTEM_UTC },
      { -29, 0 }
    },

    { { 1996, TIMECALC_JANUARY, 1, 0, 0, 0, 0, TIMECALC_SYSTEM_UTC },
      { -30, 0 }
    },

    { { 1997, TIMECALC_JULY, 1, 0, 0, 0, 0, TIMECALC_SYSTEM_UTC },
      { -31, 0 }
    },

    { { 1999, TIMECALC_JANUARY, 1, 0, 0, 0, 0, TIMECALC_SYSTEM_UTC },
      { -32, 0 }
    },

    { { 2006, TIMECALC_JANUARY, 1, 0, 0, 0, 0, TIMECALC_SYSTEM_UTC },
      { -33 , 0 }
    },

    { { 2009, TIMECALC_JANUARY, 1, 0, 0, 0, 0, TIMECALC_SYSTEM_UTC },
      { -34, 0 }
    }
  };
     

static int utc_init(struct timecalc_zone_struct *self,
		    void *arg);

static int utc_dispose(struct timecalc_zone_struct *self);


static int system_utc_diff(struct timecalc_zone_struct *self,
			   timecalc_interval_t *ival,
			   const timecalc_calendar_t *before,
			   const timecalc_calendar_t *after);

static int system_utc_offset(struct timecalc_zone_struct *self,
			     timecalc_calendar_t *offset,
			     const timecalc_calendar_t *src,
			     int *leap_second);


static int system_utc_normalise(struct timecalc_zone_struct *self,
					  timecalc_calendar_t *io_cal);


static int system_utc_aux(struct timecalc_zone_struct *self,
			   const timecalc_calendar_t *calc,
			   timecalc_calendar_aux_t *aux);

static int system_utc_epoch(struct timecalc_zone_struct *self,
			    timecalc_calendar_t *aux);


static int system_utc_lower_zone(struct timecalc_zone_struct *self,
				 struct timecalc_zone_struct **next);

static timecalc_zone_t s_system_utc = 
  {
    NULL,
    TIMECALC_SYSTEM_UTC,
    utc_init,
    utc_dispose,
    system_utc_diff,
    system_utc_offset,
    system_utc_normalise,
    system_utc_aux,
    system_utc_epoch ,
    system_utc_lower_zone
  };


/* BST: Applied to any other time zone.
 *
 * From <http://www.direct.gov.uk/en/Governmentcitizensandrights/LivingintheUK/DG_073741>
 *
 *  - In spring, the clocks go forward 1h at 0100 GMT on the last Sunday in March.
 *  - In autumn, the clocks go back 1h at 0200 BST on the last Sunday in October.
 */

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
  int rv;
  
  switch (system)
    {
    case TIMECALC_SYSTEM_GREGORIAN_TAI:
      prototype = &s_system_gtai;
      break;
    case TIMECALC_SYSTEM_UTC:
      prototype = &s_system_utc;
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
    rv = z->init(z, arg);
    if (rv) 
      {
	free(z);
	return TIMECALC_ERR_INIT_FAILED;
      }

    (*out_zone) = z;
  }
  
  return 0;
}

int timecalc_zone_dispose(timecalc_zone_t **io_zone)
{
  int rv = 0;
  
  if (!io_zone || !(*io_zone)) { return 0; }
  {
    timecalc_zone_t *t = *io_zone;
    rv = t->dispose(t);
    free(t);
  }
  (*io_zone) = NULL;
  return rv;
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
  timecalc_calendar_t offset, tmp;


  memset(&offset, '\0', sizeof(timecalc_calendar_t));
  memcpy(out, date, sizeof(timecalc_calendar_t));

  s = ival->s;

  while (s > (1 << 30))
    {
      offset.second = (1<<30); 
      rv = timecalc_op_offset(zone, &tmp, TIMECALC_OP_ADD, out, &offset);
      memcpy(out, &tmp, sizeof(timecalc_calendar_t));
      if (rv) { return rv; }
      s -= (1<<30);
    }
  while (s < -(1<<30))
    {
      offset.second = -(1<<30);
      rv = timecalc_op_offset(zone, &tmp, TIMECALC_OP_ADD, out, &offset);
      memcpy(out, &tmp, sizeof(timecalc_calendar_t));
      if (rv) { return rv; }
      s += (1<<30);
    }

  offset.second = s;
  offset.ns = ival->ns;
  rv = timecalc_op_offset(zone, &tmp, TIMECALC_OP_ADD, out, &offset);
  memcpy(out, &tmp, sizeof(timecalc_calendar_t));
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
    case TIMECALC_SYSTEM_UTC:
      return "UTC";
      break;
    default:
      return "UNKNOWN";
      break;
    }
}

int timecalc_op_fieldwise(struct timecalc_zone_struct *zone,
			  timecalc_calendar_t *dst,
			  int op,
			  const timecalc_calendar_t *opa,
			  const timecalc_calendar_t *opb)
{
  int rv;

  switch (op)
    {
    case TIMECALC_OP_ADD:
      dst->year = opa->year + opb->year;
      dst->month = opa->month + opb->month;
      dst->mday = opa->mday + opb->mday;
      dst->hour = opa->hour + opb->hour;
      dst->minute = opa->minute + opb->minute;
      dst->second = opa->second + opb->second;
      dst->ns = opa->ns + opb->ns;
      break;
    case TIMECALC_OP_SUBTRACT:
      dst->year = opa->year - opb->year;
      dst->month = opa->month - opb->month;
      dst->mday = opa->mday - opb->mday;
      dst->hour = opa->hour - opb->hour;
      dst->minute = opa->minute - opb->minute;
      dst->second = opa->second - opb->second;
      dst->ns = opa->ns - opb->ns;
      break;
    default:
      return TIMECALC_ERR_INVALID_ARGUMENT;
    }

  // If we wound up on a leap second, we need to know about it and
  // add it across normalisation.
  dst->system = opa->system;
  
  rv = zone->cal_normalise(zone, dst);
  if (rv) { return rv; }
  return 0;
}

int timecalc_op_offset(struct timecalc_zone_struct *zone,
		       timecalc_calendar_t *io_cal,
		       int op,
		       const timecalc_calendar_t *src,
		       const timecalc_calendar_t *offset)
{
  timecalc_calendar_t work, src_offset, dst_offset;
  int ls, rv;
  timecalc_zone_t *lower = NULL;
  
  // Only works in the target zone
  if (src->system != zone->system)
    {
      return TIMECALC_ERR_NOT_MY_SYSTEM;
    }

  rv = zone->lower_zone(zone, &lower);
  if (rv) { return rv; }
  
  // Work out the source offset.
  rv = zone->cal_offset(zone, &src_offset, src, &ls);
  if (rv) { return rv; }

  // If the source was a leap second, add it into the source offset.
  if (ls) { ++src_offset.second ; }

  // printf("src_ls = %d \n", ls);

  rv = timecalc_op_fieldwise(zone, &work, TIMECALC_OP_ADD, src, offset);
  if (rv) { return rv; }

  // Now work out what the offset for the target is, remembering that the
  // system is now the underlying system for this zone.
  rv = zone->cal_offset(zone, &dst_offset, &work, &ls);
  if (rv) { return rv; }


  // For obscure reasons, we need to add the leap second back in 
  // twice - once to get the differential right, once to actually
  // apply the leap second if we're in it.
  if (ls) { ++dst_offset.second; }
  // printf("dst_ls = %d \n", ls);

  int skip_leap_second_check = 0;

  // Set dst = src for all offsets that are less than the most significant
  // non-zero value in the offset (but not equal!)
  {
    // Year offsets are always applied.
    int go = !!offset->year;
    if (go) { dst_offset.month = src_offset.month; }
    go = go || !!offset->month; 
    if (go) { dst_offset.mday = src_offset.mday; }
    go = go || !!offset->mday;
    if (go) { dst_offset.hour = src_offset.hour; }
    go = go || !!offset->minute;
    if (go) { dst_offset.second = src_offset.second; ++skip_leap_second_check; }
    go = go || !!offset->second;
    if (go) { dst_offset.ns = src_offset.ns; }
  }

  // printf("ds = %d ,ss = %d \n",
  // dst_offset.second, src_offset.second);

  // Now apply the offsets .. 
  work.year += dst_offset.year - src_offset.year;
  work.month += dst_offset.month - src_offset.month;
  work.mday += dst_offset.mday - src_offset.mday;
  work.hour += dst_offset.hour - src_offset.hour;
  work.minute += dst_offset.minute - src_offset.minute;
  work.second += dst_offset.second - src_offset.second;
  work.ns += dst_offset.ns - src_offset.ns;

  // We are now back in the higher zone.
  work.system = zone->system;

  // .. and normalise one last time.
  rv = zone->cal_normalise(zone, &work);
  if (rv) { return rv; }

  // printf("ls %d sls = %d \n", ls, skip_leap_second_check);
  
  if (ls && !skip_leap_second_check) { ++work.second; }


  memcpy(io_cal, &work, sizeof(timecalc_calendar_t));
  return 0;
}

int timecalc_zone_raise(timecalc_zone_t *zone,
			timecalc_calendar_t *dest,
			const timecalc_calendar_t *src)
{
  timecalc_calendar_t dst_offset;
  int rv;
  int ls;

  // If the system for src is incorrect, cal_offset() will
  // return an error.
  rv = zone->cal_offset(zone, &dst_offset, src, &ls);
  if (rv) { return rv; }

  rv = timecalc_op_fieldwise(zone, 
			     dest,
			     TIMECALC_OP_ADD,
			     src, &dst_offset);

  if (rv) { return rv; }
  dest->system = zone->system;
  
  rv = zone->cal_normalise(zone, dest);
  if (rv) { return rv; }
  
  if (ls) { ++dest->second; }

  return 0;
}


int timecalc_zone_lower(timecalc_zone_t *zone,
			timecalc_calendar_t *dest,
			timecalc_zone_t **lower,
			const timecalc_calendar_t *src)
{
  int rv;
  timecalc_calendar_t offset;
  int ls;

  rv = zone->lower_zone(zone,lower);
  if (rv) { return rv;  }

  if (!(*lower))
    {
      // This is already the lowest zone.
      memcpy(dest, src, sizeof(timecalc_calendar_t));
      return 0;
    }

  if (src->system != zone->system)
    {
      return TIMECALC_ERR_NOT_MY_SYSTEM;
    }

  // Compute offset.
  rv = zone->cal_offset(zone, &offset, src, &ls);
  if (rv) { return rv; }

  // Since we are about to subtract.. 
  if (ls) { ++offset.second; }

  // Now ..
  memcpy(dest, src, sizeof(timecalc_calendar_t));
  dest->system = (*lower)->system;  

  // Add the offset in on a field-by-field basis.
  rv = timecalc_op_fieldwise((*lower), dest, TIMECALC_OP_SUBTRACT,
			     dest, &offset);
  if (rv) { return rv; }

  // And normalise in the upper timezone (because cal_normalise()
  //  steps down implicitly)
  rv = zone->cal_normalise(zone, dest);


  
  return rv;
}



/* -------------------- Generic NULL functions -------- */
static int null_init(struct timecalc_zone_struct *self, void *arg)
{
  return 0;
}

static int null_dispose(struct timecalc_zone_struct *self)
{
  return 0;
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


static int system_gtai_offset(struct timecalc_zone_struct *self,
				  timecalc_calendar_t *offset,
				  const timecalc_calendar_t *src,
				  int *leap_second)
{
  // Particularly easy ..
  memset(offset, '\0', sizeof(timecalc_calendar_t));
  *leap_second = 0;
  return 0;
}

static int system_gtai_normalise(struct timecalc_zone_struct *self,
				 timecalc_calendar_t *io_cal)
{
  // And normalise .
  int done = 0;

  // Convert any negatives to positives ..
  while (io_cal->ns < 0)
    {
      --io_cal->second;
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
      io_cal->minute += MINUTES_PER_HOUR;
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

    // We want our ydays to be zero-based.
    aux->yday = yday -1;
  }

  // The century start day of week follows a 4,2,0,6 pattern. 17XX == 4 */
  static const int cstart[] = { 4,2,0, 6};
  static const int wday[] = { 0, 3, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5 };
  static const int leap_wday[] = { 6, 2, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5 };
  int dow;

  // &3 is here to make sure the result is +ve.
  int idx = ((cal->year / 100) - 17)&3;
  int yoff = (cal->year % 100);

  //printf("cstart[%d] = %d \n", 
  //idx, cstart[idx]);
  // printf("yoff = %d \n", cal->year%100);

  // This calculation actually expects mday to be 1-based.
  dow = cstart[idx] + yoff + (yoff / 4) + 
    (is_gregorian_leap_year(cal->year) ? leap_wday[cal->month] : 
     wday[cal->month]) + (cal->mday) ;
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

static int system_gtai_lower_zone(struct timecalc_zone_struct *self,
				  struct timecalc_zone_struct **next)
{
  (*next) = NULL;
  return 0;
}

/* ---------------------- UTC ---------------------- */

static int system_utc_offset(struct timecalc_zone_struct *self,
			     timecalc_calendar_t *dest,
			     const timecalc_calendar_t *src,
			     int *is_leap_second)
{
  const int nr_entries = sizeof(utc_lookup_table)/sizeof(utc_lookup_entry_t);
  timecalc_interval_t iv = { 0, 0 };
  int i;
  int cmp_value;
  int src_tai;
  timecalc_calendar_t utcsrc;

  if (src->system == TIMECALC_SYSTEM_GREGORIAN_TAI)
    {
      // The source is in TAI.
      src_tai = 1;
    }
  else if (src->system == TIMECALC_SYSTEM_UTC)
    {
      src_tai = 0;
      memcpy(&utcsrc, src, sizeof(timecalc_calendar_t));
    }
  else
    {
      return TIMECALC_ERR_BAD_SYSTEM;
    }

  
  (*is_leap_second) = 0;

  // First entry in the table is a sentinel.
  for (i = 1 ;i < nr_entries; ++i)
    {
      utc_lookup_entry_t *current = &utc_lookup_table[i];


      // UTC references itself (joy!) so that if the source is TAI we need
      // to add the current entry before comparing.
      if (src_tai)
	{
	  memcpy(&utcsrc, src, sizeof(timecalc_calendar_t));
	  utcsrc.second += utc_lookup_table[i].utctai.s;
	  utcsrc.ns += utc_lookup_table[i].utctai.ns;
	  utcsrc.system = TIMECALC_SYSTEM_UTC;
	  self->cal_normalise(self, &utcsrc);
	}

      // We synthetically zero utcsrc.ns so that the compare function
      // will return 0 when we are exactly on a leap second.
      utcsrc.ns = 0;

      cmp_value = timecalc_calendar_cmp(&utcsrc, &current->when);
      
      // If we are before this entry, the previous entry applies
      if (cmp_value < 0)
	{
	  break;
	}
      
      
      if (cmp_value == 0)
	{
	  // This is a leap second. Are we going forward or back? If the previous
	  // value is < current, we're going forward, else we're going back.
	  //
	  // Also table indices below UTC_LOOKUP_MIN_LEAP_SECOND are sync points
	  // and not leap seconds per se.
	  (*is_leap_second) = (i >= UTC_LOOKUP_MIN_LEAP_SECOND) && 
	    (timecalc_interval_cmp(&utc_lookup_table[i-1].utctai, 
				   &current->utctai) > 0);
	  iv = current->utctai;
	  break;
	}

      // If we pass this point, this table entry applies.
      iv = utc_lookup_table[i].utctai;


    }
	 
  
  // Right. If we're going to UTC, add the correction. If to
  // TAI, subtract it.
  memset(dest, '\0', sizeof(timecalc_calendar_t));


  // If we're going to UTC and this is a -ve leap second, add one to 
  // the number of seconds before we normalise and record that we're
  // in the 60th second later.
  dest->second += iv.s;
  dest->ns += iv.ns; 
  if (*is_leap_second) { --dest->second; }

  return 0;
}

static int system_utc_normalise(struct timecalc_zone_struct *self,
				timecalc_calendar_t *io_cal)
{
  timecalc_zone_t *gtai = (timecalc_zone_t *)self->handle;
  int rv ;
  // And normalise .
  
  // Normalise in the underlying system.
  rv = gtai->cal_normalise(gtai, io_cal);

  return 0;
}

static int system_utc_diff(struct timecalc_zone_struct *self,
			   timecalc_interval_t *ival,
			   const timecalc_calendar_t *before,
			   const timecalc_calendar_t *after)
{
  // Convert UTC to TAI for both before and after, then call down.
  int ls;
  timecalc_calendar_t src_offset, dst_offset;
  timecalc_calendar_t tai_b, tai_a;
  int rv;

  if (!(before->system == TIMECALC_SYSTEM_UTC || 
	before->system == TIMECALC_SYSTEM_GREGORIAN_TAI) || 
      !(after->system == TIMECALC_SYSTEM_UTC ||
	after->system == TIMECALC_SYSTEM_GREGORIAN_TAI))
    {
      return TIMECALC_ERR_BAD_SYSTEM;
    }
  
  // Number of seconds between two dates. Get the offsets for both..
  if (before->system == TIMECALC_SYSTEM_UTC)
    {
      rv = self->cal_offset(self, &src_offset, before, &ls);
      if (rv) { return rv; }
      if (ls) { ++src_offset.second; }

      // .. and subtract .. 
      rv = timecalc_op_fieldwise(self, 
				 &tai_b,
				 TIMECALC_OP_SUBTRACT,
				 before, &src_offset);
      if (rv) { return rv; }
      tai_b.system = TIMECALC_SYSTEM_GREGORIAN_TAI;
    }

  if (after->system == TIMECALC_SYSTEM_UTC)
    {
      rv =self->cal_offset(self, &dst_offset, after, &ls);
      if (rv) { return rv; }
      if (ls) { ++dst_offset.second; }

      // And subtract.
      rv = timecalc_op_fieldwise(self, 
				 &tai_a,
				 TIMECALC_OP_SUBTRACT,
				 after, &dst_offset);
      if (rv) { return rv; }
      tai_a.system = TIMECALC_SYSTEM_GREGORIAN_TAI;
    }

  if (rv) { return rv; }

  // Now our times are both in gregorian tai .. 
  {
    timecalc_zone_t *tai = (timecalc_zone_t *)(self->handle);

    return tai->diff(tai, ival, &tai_a, &tai_b);
  }
}

static int system_utc_aux(struct timecalc_zone_struct *self,
			  const timecalc_calendar_t *calc,
			  timecalc_calendar_aux_t *aux)
{
  // Same as for the underlying calendar.
  timecalc_zone_t *gtai = (timecalc_zone_t *)self->handle;

  return gtai->aux(gtai, calc, aux);
}

static int system_utc_epoch(struct timecalc_zone_struct *self,
			    timecalc_calendar_t *aux)
{
  // timecalc_zone_t *gtai = (timecalc_zone_t *)self->handle;

  // The UTC epoch is 'properly' in 1961, but 1972 has the advantage
  // that it was when the UTC and TAI seconds harmonised.
  static const timecalc_calendar_t epoch = 
    { 1972, TIMECALC_JANUARY, 1, 0, 0, 0, 0, TIMECALC_SYSTEM_UTC };
      
  memcpy(aux, &epoch, sizeof(timecalc_calendar_t));
  return 0;
}

static int utc_init(struct timecalc_zone_struct *self,
	      void *arg)
{
  int rv;
  timecalc_zone_t *q = NULL;

  rv = timecalc_zone_new(TIMECALC_SYSTEM_GREGORIAN_TAI, &q, NULL);
  if (rv) { return rv; }
  self->handle = (void *)q;
  return 0;
}

static int utc_dispose(struct timecalc_zone_struct *self)
{
  timecalc_zone_t *q = (timecalc_zone_t *)self->handle;
  int rv;

  rv = timecalc_zone_dispose(&q);
  self->handle = NULL;
  return rv;
}

static int system_utc_lower_zone(struct timecalc_zone_struct *self,
				 struct timecalc_zone_struct **next)
{
  (*next) = (struct timecalc_zone_struct *)(self->handle);
  return 0;
}



/* End file */
