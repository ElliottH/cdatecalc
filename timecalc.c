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

#define DEBUG_GTAI 1
#define DEBUG_UTC 1
#define DEBUG_UTCPLUS 0
#define DEBUG_BST 1
#define DEBUG_REBASED 1

#define ONE_MILLION 1000000
#define ONE_BILLION (ONE_MILLION * 1000)

//! @todo Could use gcc extensions to evaluate only once ..
#define MIN(x,y) (((x) < (y)) ? (x) : (y))
#define MAX(x,y) (((x) > (y)) ? (x) : (y))
#define SWAP(x,y) { __typeof(x) __tmp; __tmp = (x); (x) = (y); (y) = __tmp; }

static const char *dbg_pdate(const timecalc_calendar_t *cal);


/** A generic diff function: lower both dates and then call diff()
 *  again.
 */
static int system_lower_diff(struct timecalc_zone_struct *self,
			     timecalc_interval_t *ival,
			     const timecalc_calendar_t *before,
			     const timecalc_calendar_t *after);


/** Dispose everyone, all the way down the tree */
static int chain_dispose(struct timecalc_zone_struct *self);

static int null_init(struct timecalc_zone_struct *self, int arg_i, void *arg_n);
static int null_dispose(struct timecalc_zone_struct *self);

static int system_gtai_diff(struct timecalc_zone_struct *self,
			    timecalc_interval_t *ival,
			    const timecalc_calendar_t *before,
			    const timecalc_calendar_t *after);

static int system_gtai_offset(struct timecalc_zone_struct *self,
			      timecalc_calendar_t *offset,
			      const timecalc_calendar_t *src);

static int system_gtai_aux(struct timecalc_zone_struct *self,
			   const timecalc_calendar_t *calc,
			   timecalc_calendar_aux_t *aux);

static int system_gtai_op(struct timecalc_zone_struct *self,
			   timecalc_calendar_t *dest,
			   const timecalc_calendar_t *src,
			   const timecalc_calendar_t *offset, 
			   int op);


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
    system_gtai_op,
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
    { { 1972, TIMECALC_JUNE, 31, 23, 59, 59, 0, TIMECALC_SYSTEM_UTC }, 
      { -11, 0 }
    },

    { { 1972, TIMECALC_DECEMBER, 31, 23, 59, 59, 0, TIMECALC_SYSTEM_UTC },
      { -12, 0 } 
    },

    { { 1973, TIMECALC_DECEMBER, 31, 23, 59, 59, 0, TIMECALC_SYSTEM_UTC },
      { -13, 0 }
    },

    { { 1974, TIMECALC_DECEMBER, 31, 23, 59, 59, 0, TIMECALC_SYSTEM_UTC },
      { -14, 0 }
    },
    
    { { 1975, TIMECALC_DECEMBER, 31, 23, 59, 59, 0, TIMECALC_SYSTEM_UTC },
      { -15, 0 }
    },
    
    { { 1976, TIMECALC_DECEMBER, 31, 23, 59, 59, 0, TIMECALC_SYSTEM_UTC },
      { -16, 0 }
    },

    { { 1977, TIMECALC_DECEMBER, 31, 23, 59, 59, 0, TIMECALC_SYSTEM_UTC },
      { -17, 0 }
    },
    
    { { 1978, TIMECALC_DECEMBER, 31, 23, 59, 59, 0, TIMECALC_SYSTEM_UTC },
      { -18, 0 }
    },
    
    { { 1979, TIMECALC_DECEMBER, 31, 23, 59, 59, 0, TIMECALC_SYSTEM_UTC },
      { -19, 0 }
    },
    
    { { 1981, TIMECALC_JUNE, 30, 23, 59, 59, 0,  TIMECALC_SYSTEM_UTC },
      { -20, 0 }
    },

    { { 1982, TIMECALC_JUNE, 30, 23, 59, 59, 0,  TIMECALC_SYSTEM_UTC },
      { -21, 0 }
    },

    { { 1983, TIMECALC_JUNE, 30, 23, 59, 59, 0, TIMECALC_SYSTEM_UTC },
      { -22, 0 }
    },

    { { 1985, TIMECALC_JUNE, 30, 23, 59, 59, 0, TIMECALC_SYSTEM_UTC },
      { -23, 0 }
    },
    
    { { 1987, TIMECALC_DECEMBER, 31, 23, 59, 59, 0, TIMECALC_SYSTEM_UTC },
      { -24, 0 }
    },

    { { 1989, TIMECALC_DECEMBER, 31, 23, 59, 59, 0, TIMECALC_SYSTEM_UTC },
      { -25, 0 }
    },

    { { 1990, TIMECALC_DECEMBER, 31, 23, 59, 59, 0, TIMECALC_SYSTEM_UTC },
      { -26, 0 }
    },

    { { 1992, TIMECALC_JUNE, 30, 23, 59, 59, 0, TIMECALC_SYSTEM_UTC },
      { -27, 0 }
    },

    { { 1993, TIMECALC_JUNE, 30, 23, 59, 59, 0, TIMECALC_SYSTEM_UTC },
      { -28, 0 }
    },

    { { 1994, TIMECALC_JUNE, 30, 23, 59, 59, 0, TIMECALC_SYSTEM_UTC },
      { -29, 0 }
    },

    { { 1995, TIMECALC_DECEMBER, 31, 23, 59, 59, 0, TIMECALC_SYSTEM_UTC },
      { -30, 0 }
    },

    { { 1997, TIMECALC_JUNE, 30, 23, 59, 59, 0, TIMECALC_SYSTEM_UTC },
      { -331, 0 }
    },

    { { 1998, TIMECALC_DECEMBER, 31, 23, 59, 59, 0, TIMECALC_SYSTEM_UTC },
      { -32, 0 }
    },

    { { 2005, TIMECALC_DECEMBER, 31, 23, 59, 59, 0, TIMECALC_SYSTEM_UTC },
      { -33 , 0 }
    },

    { { 2008, TIMECALC_DECEMBER, 31, 23, 59, 59, 0, TIMECALC_SYSTEM_UTC },
      { -34, 0 }
    }
  };
     

static int utc_init(struct timecalc_zone_struct *self,
		    int arg_i, void *arg_n);


//static int system_utc_diff(struct timecalc_zone_struct *self,
//			   timecalc_interval_t *ival,
//			   const timecalc_calendar_t *before,
//			   const timecalc_calendar_t *after);

static int system_utc_offset(struct timecalc_zone_struct *self,
			     timecalc_calendar_t *offset,
			     const timecalc_calendar_t *src);


static int system_utc_op(struct timecalc_zone_struct *self,
			 timecalc_calendar_t *dest,
			 const timecalc_calendar_t *src,
			 const timecalc_calendar_t *offset,
			 int op);		    



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
    null_dispose,
    system_lower_diff,
    system_utc_offset,
    system_utc_op,
    system_utc_aux,
    system_utc_epoch ,
    system_utc_lower_zone
  };


// UTC+XXX


// 0 = -720 m = -12h , 1440 = 720m = +12h
#define UTCPLUS_SYSTEM_TO_MINUTES(x) ((x) - (TIMECALC_SYSTEM_UTCPLUS_BASE + 720))


static int utc_plus_init(struct timecalc_zone_struct *self, int arg_i, void *arg_n);

#if 0
static int system_utcplus_diff(struct timecalc_zone_struct *self,
			   timecalc_interval_t *ival,
			   const timecalc_calendar_t *before,
			   const timecalc_calendar_t *after);
#endif

static int system_utcplus_offset(struct timecalc_zone_struct *self,
			     timecalc_calendar_t *offset,
				 const timecalc_calendar_t *src);

static int system_utcplus_aux(struct timecalc_zone_struct *self,
			      const timecalc_calendar_t *calc,
			      timecalc_calendar_aux_t *aux);

static int system_utcplus_op(struct timecalc_zone_struct *self,
			     timecalc_calendar_t *io_cal, 
			     const timecalc_calendar_t *src,
			     const timecalc_calendar_t *offset,
			     int op);

static int system_utcplus_epoch(struct timecalc_zone_struct *self,
				timecalc_calendar_t *aux);

static int system_utcplus_lower_zone(struct timecalc_zone_struct *self,
				     struct timecalc_zone_struct **next);


static timecalc_zone_t s_system_utcplus = 
  {
    NULL,
    TIMECALC_SYSTEM_UTCPLUS_BASE,
    utc_plus_init,
    null_dispose,
    system_lower_diff,
    system_utcplus_offset,
    system_utcplus_op,
    system_utcplus_aux,
    system_utcplus_epoch,
    system_utcplus_lower_zone 
  };


static int bst_init(struct timecalc_zone_struct *self,
		    int iarg, void *parg);

static int system_bst_offset(struct timecalc_zone_struct *self,
			     timecalc_calendar_t *offset,
			     const timecalc_calendar_t *src);

static int system_bst_op(struct timecalc_zone_struct *self,
			 timecalc_calendar_t *dest,
			 const timecalc_calendar_t *src,
			 const timecalc_calendar_t *offset,
			 int op);

static int system_bst_aux(struct timecalc_zone_struct *self,
			  const timecalc_calendar_t *calc,
			  timecalc_calendar_aux_t *aux);

static int system_bst_epoch(struct timecalc_zone_struct *self,
			    timecalc_calendar_t *aux);

static int system_bst_lower_zone(struct timecalc_zone_struct *self,
				 struct timecalc_zone_struct **next);


static timecalc_zone_t s_system_bst = 
  {
    NULL,
    TIMECALC_SYSTEM_BST,
    bst_init,
    null_dispose,
    system_lower_diff,
    system_bst_offset,
    system_bst_op,
    system_bst_aux,
    system_bst_epoch,
    system_bst_lower_zone
  };

/* -------------------------- Rebase ------------------- */


static int system_rebased_init(struct timecalc_zone_struct *self,
			       int argi,
			       void *argn);

static int system_rebased_dispose(struct timecalc_zone_struct *self);

static int system_rebased_offset(struct timecalc_zone_struct *self,
			      timecalc_calendar_t *offset,
			      const timecalc_calendar_t *src);

static int system_rebased_aux(struct timecalc_zone_struct *self,
			   const timecalc_calendar_t *calc,
			   timecalc_calendar_aux_t *aux);

static int system_rebased_op(struct timecalc_zone_struct *self,
			   timecalc_calendar_t *dest,
			   const timecalc_calendar_t *src,
			   const timecalc_calendar_t *offset, 
			   int op);


static int system_rebased_epoch(struct timecalc_zone_struct *self,
			     timecalc_calendar_t *aux);

static int system_rebased_lower_zone(struct timecalc_zone_struct *self,
				  struct timecalc_zone_struct **next);

// The handle in a rebased zone is actually quite complex..
typedef struct timecalc_rebased_handle_struct
{
  timecalc_zone_t *lower;
  timecalc_calendar_t offset;
} timecalc_rebased_handle_t;


static timecalc_zone_t s_system_rebased = 
  {
    NULL,
    TIMECALC_SYSTEM_REBASED,
    system_rebased_init,
    system_rebased_dispose,
    system_lower_diff,
    system_rebased_offset,
    system_rebased_op,
    system_rebased_aux,
    system_rebased_epoch,
    system_rebased_lower_zone
  };


/** Do knockdown of a DST difference by an offset, as required for a complex
 *  add 
 */
static void do_knockdown(timecalc_calendar_t *io_diff, const timecalc_calendar_t *offset, 
			 int *do_ls);
static void timecalc_negate(timecalc_calendar_t *cal);

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

#define SECONDS_PER_HOUR   (SECONDS_PER_MINUTE * MINUTES_PER_HOUR)
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
		      int arg_i,
		      void *arg_n)
{
  timecalc_zone_t *prototype = NULL;
  int rv;


  if (system >= TIMECALC_SYSTEM_UTCPLUS_BASE && 
      system <= (TIMECALC_SYSTEM_UTCPLUS_BASE + 1440))
    {
      prototype = &s_system_utcplus;
    }
  
  switch (system)
    {
    case TIMECALC_SYSTEM_GREGORIAN_TAI:
      prototype = &s_system_gtai;
      break;
    case TIMECALC_SYSTEM_UTC:
      prototype = &s_system_utc;
      break;
    case TIMECALC_SYSTEM_BST:
      prototype = &s_system_bst;
      break;
    case TIMECALC_SYSTEM_REBASED:
      prototype = &s_system_rebased;
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
    z->system = system;
    rv = z->init(z, arg_i, arg_n);
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

int timecalc_tai_new(timecalc_zone_t **ozone)
{
  int rv;
  
  rv = timecalc_zone_new(TIMECALC_SYSTEM_GREGORIAN_TAI, ozone, 0, NULL);
  return rv;
}

int timecalc_utc_new(timecalc_zone_t **ozone)
{
  int rv;
  timecalc_zone_t *z = NULL;
  
  rv = timecalc_zone_new(TIMECALC_SYSTEM_GREGORIAN_TAI, &z,
			 0, NULL);
  if (rv) { return rv; }

  rv = timecalc_zone_new(TIMECALC_SYSTEM_UTC, ozone, 0, z);
  if (rv) 
    {
      z->dispose(z);
    }
  else
    {
      (*ozone)->dispose = chain_dispose;
    }

  return rv;
}

int timecalc_utcplus_new(timecalc_zone_t **ozone, int offset)
{
  int rv;
  timecalc_zone_t *z = NULL;

  rv = timecalc_utc_new(&z);
  if (rv) { return rv; }

  rv = timecalc_zone_new(TIMECALC_SYSTEM_UTCPLUS_ZERO + offset, ozone, 0, z);
  if (rv) 
    {
      z->dispose(z);
    }
  else
    {
      (*ozone)->dispose = chain_dispose;
    }
  return rv;
}

int timecalc_bst_new(timecalc_zone_t **ozone)
{
  int rv;
  timecalc_zone_t *z = NULL;

  rv = timecalc_utc_new(&z);
  if (rv) { return rv; }

  rv = timecalc_zone_new(TIMECALC_SYSTEM_BST, ozone, 0, z);
  if (rv) 
    {
      z->dispose(z);
    }
  else
    {
      (*ozone)->dispose = chain_dispose;
    }
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
      rv = timecalc_op(zone, &tmp, out, &offset, TIMECALC_OP_SIMPLE_ADD);
      memcpy(out, &tmp, sizeof(timecalc_calendar_t));
      if (rv) { return rv; }
      s -= (1<<30);
    }
  while (s < -(1<<30))
    {
      offset.second = -(1<<30);
      rv = timecalc_op(zone, &tmp, out, &offset, TIMECALC_OP_SIMPLE_ADD);
      memcpy(out, &tmp, sizeof(timecalc_calendar_t));
      if (rv) { return rv; }
      s += (1<<30);
    }

  offset.second = s;
  offset.ns = ival->ns;
  rv = timecalc_op(zone, &tmp, out, &offset, TIMECALC_OP_SIMPLE_ADD);
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


const char *timecalc_describe_system(const int sys)
{
  static char buf[128];
  int system = sys;
  const char *modifier = "";

  if (system & TIMECALC_SYSTEM_TAINTED)
    {
      modifier = "*";
      system &= ~TIMECALC_SYSTEM_TAINTED;
    }


  if (system >= TIMECALC_SYSTEM_UTCPLUS_BASE && 
      system <= (TIMECALC_SYSTEM_UTCPLUS_BASE + 1440))
    {
      int mins = UTCPLUS_SYSTEM_TO_MINUTES(system);
      if (mins > 0)
	{
	  sprintf(buf, "UTC+%02d%02d%s", (mins/60), (mins%60), modifier);
	}
      else
	{
	  sprintf(buf, "UTC-%02d%02d%s", (-mins/60), (-mins)%60, modifier);
	}
      return buf;
    }
      

  switch (system)
    {
    case TIMECALC_SYSTEM_GREGORIAN_TAI:
      sprintf(buf, "TAI%s",modifier);
      break;
    case TIMECALC_SYSTEM_UTC:
      sprintf(buf, "UTC%s", modifier);
      break;
    case TIMECALC_SYSTEM_OFFSET:
      sprintf(buf, "OFF%s", modifier);
      break;
    case TIMECALC_SYSTEM_BST:
      sprintf(buf, "BST%s", modifier);
      break;
    case (TIMECALC_SYSTEM_REBASED & ~TIMECALC_SYSTEM_TAINTED):
      sprintf(buf, "REBASED%s", modifier);
      break;
    default:
      return "UNKNOWN";
      break;
    }

  return buf;
}


int timecalc_op(struct timecalc_zone_struct *zone,
		timecalc_calendar_t *dst,
		const timecalc_calendar_t *opa,
		const timecalc_calendar_t *opb,
		int op)
{
  return zone->op(zone, dst, opa, opb, op);
}

int timecalc_zone_raise(timecalc_zone_t *zone,
			timecalc_calendar_t *dest,
			const timecalc_calendar_t *src)
{
  timecalc_calendar_t dst_offset;
  timecalc_calendar_t tmp;
  int rv;
  timecalc_zone_t *low;
  
  printf("!\n");
  rv = zone->lower_zone(zone, &low);
  if (rv) { return rv; }

  printf("Raise %s to %d (low system %d)\n", dbg_pdate(src), zone->system, low->system);

  if (src->system == low->system) 
    {
      // If the system for src is incorrect, cal_offset() will
      // return an error.
      rv = zone->offset(zone, &dst_offset, src);
    }
  else
    {
      rv = TIMECALC_ERR_NOT_MY_SYSTEM;
    }

  if (rv == TIMECALC_ERR_NOT_MY_SYSTEM)
    {
      printf("Can't raise %s \n", dbg_pdate(src));

      // Try lower.
      
      if (low)
	{
	  rv = timecalc_zone_raise(low, &tmp, src);
	  if (rv) { return rv; }
	  src = &tmp;
	}
      else
	{
	  return TIMECALC_ERR_NOT_MY_SYSTEM;
	}
      
      rv = zone->offset(zone, &dst_offset, src);
      if (rv)
	{
	  return rv;
	}
    }
  else if (rv)
    {
      return rv; 
    }
  

  printf("Simple add of %s .. \n", dbg_pdate(&dst_offset));
  {
    timecalc_calendar_t tmp;

    memcpy(&tmp, src, sizeof(timecalc_calendar_t));
    tmp.system = zone->system;

    rv = timecalc_op(zone, 
		     dest,
		     &tmp, &dst_offset, 
		     TIMECALC_OP_ZONE_ADD);
  }

  if (rv) { return rv; }
  dest->system = zone->system;

  printf("*** raised() dest = %s \n", dbg_pdate(dest));
  
  return 0;
}

int timecalc_zone_lower_to(timecalc_zone_t *zone,
			   timecalc_calendar_t *dest,
			   timecalc_zone_t **lower,
			   const timecalc_calendar_t *src, 
			   int to_system)
{
  timecalc_calendar_t current;

  memcpy(&current, src, sizeof(timecalc_calendar_t));
  while (current.system != to_system)
    {
      timecalc_zone_t *l = NULL;
      int rv;

      rv = zone->lower_zone(zone, &l);
      if (rv) 
	{ return rv; }
      if (!l) 
	{
	  return TIMECALC_ERR_CANNOT_CONVERT;
	}
      
      rv = timecalc_zone_lower(zone, dest, &l, &current);
      if (rv) { return rv; }
      (*lower) = l;

      // Otherwise ..
      zone = l;
      memcpy(&current, dest, sizeof(timecalc_calendar_t));
    }

  return 0;
}


int timecalc_zone_lower(timecalc_zone_t *zone,
			timecalc_calendar_t *dest,
			timecalc_zone_t **lower,
			const timecalc_calendar_t *src)
{
  int rv;
  timecalc_calendar_t offset;

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
  rv = zone->offset(zone, &offset, src);
  if (rv) { return rv; }

  // Now ..

  printf("src[2] = %s\n", dbg_pdate(src));

  memcpy(dest, src, sizeof(timecalc_calendar_t));
  dest->system = (*lower)->system;  

  printf("lower offset = %s \n",dbg_pdate(&offset));

  timecalc_negate(&offset);

  // Add the offset in on a field-by-field basis.
  rv = timecalc_op((*lower), dest, 
			     dest, &offset, TIMECALC_OP_ZONE_ADD);
  if (rv) { return rv; }
  
  return 0;
}



/* -------------------- Generic NULL functions -------- */
static int null_init(struct timecalc_zone_struct *self, int arg_i, void *arg_n)
{
  return 0;
}

static int null_dispose(struct timecalc_zone_struct *self)
{
  return 0;
}

static int chain_dispose(struct timecalc_zone_struct *self)
{
  timecalc_zone_t *z = (timecalc_zone_t *)self->handle;
  int rv;

  rv = z->dispose(z);
  free(z);
  return rv;
}

static int system_lower_diff(struct timecalc_zone_struct *self,
			     timecalc_interval_t *ivalp,
			     const timecalc_calendar_t *before,
			     const timecalc_calendar_t *after)
{
  timecalc_calendar_t bl, al;
  timecalc_zone_t *z;
  int rv;


  rv = timecalc_zone_lower(self, &bl, &z, before);
  if (rv) { return rv; }

  rv = timecalc_zone_lower(self, &al, &z, after);
  if (rv) { return rv; }

  printf("system_lower_diff[] before = %s \n", dbg_pdate(before));
  printf("system_lower_diff[] bl     = %s \n", dbg_pdate(&bl));

  printf("system_lower_diff[] after   = %s \n", dbg_pdate(after));
  printf("system_lower_diff[] al     = %s \n", dbg_pdate(&al));

  return z->diff(z, ivalp, &bl, &al);
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
   *
   */

  printf("---gtai_diff()\n");

  if (before->system != after->system) { return TIMECALC_ERR_SYSTEMS_DO_NOT_MATCH; }
  if (before->system != TIMECALC_SYSTEM_GREGORIAN_TAI) 
    {
      return TIMECALC_ERR_NOT_MY_SYSTEM;
    }

  if (timecalc_calendar_cmp(before, after) > 0)
    {
      int rv;
      rv =  system_gtai_diff(self, ivalp, after, before);
      if (rv) { return rv; }
      ivalp->s = -ivalp->s;
      ivalp->ns = -ivalp->ns;

      return rv;
    }

  // Bring the interval onto the stack for manipulation
  // (also helps gcc to optimise this code)
  
  timecalc_interval_t ival;
  memcpy(&ival, ivalp, sizeof(timecalc_interval_t));


  printf("init: s = %lld ns = %ld \n", 
	 ival.s, ival.ns);
  {
    int cur = before->month;
    int curday = before->mday;
    int curdays;
    int last = after->month;
    int lastday = after->mday;
    int is_leap = is_gregorian_leap_year(after->year);
    int curyear = before->year;
    int lastyear = after->year;

    curdays = gregorian_months[cur] + 
      ((is_leap && cur == TIMECALC_FEBRUARY) ? 1 : 0);

    while (1)
      {

#if DEBUG_GTAI
	printf("-> d cur = %d curday = %d \n", cur, curday);
#endif

	// Advance by a day each time, until we hit 'after'.
	if (cur == last && curday == lastday && curyear == lastyear) 
	  {
	    // There!
	    break;
	  }

	// Otherwise .. 
	ival.s += SECONDS_PER_DAY;
	++curday;

	{
	  int daysin = gregorian_months[cur] + 
	    ((is_leap && cur == TIMECALC_FEBRUARY) ? 1 : 0);

	  if (curday > daysin)
	  {
	    curday = 1;
	    ++cur;
	  }
	}

	{
	  if (cur >= 12)
	    {
	      cur = 0;
	      ++curyear;
	    }
	}	      
      }
  }

  printf("ival.s = %lld\n", ival.s);
  {
    int hrdiff = after->hour - before->hour;

    printf("hrdiff = %d \n", hrdiff);
    ival.s += SECONDS_PER_HOUR * hrdiff;
  }

  {
    int mdiff = after->minute - before->minute;

    printf("mdiff = %d \n", mdiff);
    ival.s += SECONDS_PER_MINUTE *mdiff;
  }
  printf("sdiff = %d \n", (after->second -before->second));
  ival.s += (after->second - before->second);
  ival.ns = (after->ns - before->ns);
  if (ival.ns < 0)
    {
      --ival.s;
      ival.ns += ONE_BILLION;
    }
  
  memcpy(ivalp, &ival, sizeof(timecalc_interval_t));
  printf("---end gtai_diff()\n");

  return 0;
}


static int system_gtai_offset(struct timecalc_zone_struct *self,
				  timecalc_calendar_t *offset,
			      const timecalc_calendar_t *src)
{
  // Particularly easy ..
  memset(offset, '\0', sizeof(timecalc_calendar_t));

  return 0;
}

static int system_gtai_op(struct timecalc_zone_struct *self,
			  timecalc_calendar_t *dest,
			  const timecalc_calendar_t *src,
			  const timecalc_calendar_t *offset,
			  int op)
{
  // And normalise .
  int done = 0;
  int rv;

  rv = timecalc_simple_op(dest, src, offset, op);
#if DEBUG_GTAI
  printf("gtai_op: src                        = %s\n", dbg_pdate(src));
  printf("gtai_op: offset                     = %s\n", dbg_pdate(offset));
  printf("gtai_op: timecalc_simple_op returns = %s\n", dbg_pdate(dest));
#endif

  if (rv) { return rv; }

  // Convert any negatives to positives ..
  while (dest->ns < 0)
    {
      --dest->second;
      dest->ns += ONE_BILLION;
    }
  while (dest->second < 0)
    {
      --dest->minute;
      dest->second += SECONDS_PER_MINUTE;
    }

  while (dest->minute < 0)
    {
      --dest->hour;
      dest->minute += MINUTES_PER_HOUR;
    }

  while (dest->hour < 0)
    {
      --dest->mday;
      dest->hour += HOURS_PER_DAY;
    }

  while (dest->month < 0)
    {
      --dest->year;
      dest->month += 12;
    }
  
  while (dest->mday < 1)
    {
      // Actually in the previous month. Ugh.
      --dest->month;
      if (dest->month < 0)
	{
	  --dest->year;
	  dest->month += 12;
	}
      
      dest->mday += (gregorian_months[dest->month]);
      if (dest->month == 1 &&
	  is_gregorian_leap_year(dest->year))
	{
	  ++dest->mday; // Leap year correction
	}
    }
  
  // We can do the time independently of date.
  dest->second += (dest->ns / ONE_BILLION);
  dest->ns = (dest->ns % ONE_BILLION);

  dest->minute += (dest->second / SECONDS_PER_MINUTE);
  dest->second = (dest->second % SECONDS_PER_MINUTE);

  dest->hour += (dest->minute / MINUTES_PER_HOUR);
  dest->minute = (dest->minute % MINUTES_PER_HOUR);

  dest->mday += (dest->hour / HOURS_PER_DAY);
  dest->hour = (dest->hour % HOURS_PER_DAY);

  // Now the tricky part .. 
  while (!done)
    {
      // Make sure the month is valid.
      while (dest->month > 11)
	{
	  ++dest->year; 
	  dest->month -= 12;
	}
      
      int nr_mday;
      
      // Adjust mday
      nr_mday = (gregorian_months[dest->month]);

      if (dest->month == 1 && 
	  is_gregorian_leap_year(dest->year))
	{
	  ++nr_mday;
	}


      if (dest->mday > nr_mday)
	{
	  dest->mday -= nr_mday;
	  ++dest->month;
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
			     const timecalc_calendar_t *src)
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
      return TIMECALC_ERR_NOT_MY_SYSTEM;
    }


#if DEBUG_UTC
  printf("cal_offset() %s \n", dbg_pdate(src));
#endif

  
  // First entry in the table is a sentinel.
  for (i = 1 ;i < nr_entries; ++i)
    {
      utc_lookup_entry_t *current = &utc_lookup_table[i];
      int current_leap = 0;
      timecalc_calendar_t to_cmp;
      
      // UTC references itself (joy!) so that if the source is TAI we need
      // to add the current entry before comparing.
      if (src_tai)
	{
	  timecalc_calendar_t off;
	  int rv;

	  memset(&off, '\0', sizeof(timecalc_calendar_t));
	  
	  off.ns = utc_lookup_table[i].utctai.ns;
	  off.second = utc_lookup_table[i].utctai.s;

#if DEBUG_UTC
	  printf("src = %s\n", dbg_pdate(src));
#endif

	  rv = self->op(self, &utcsrc, src, &off, TIMECALC_OP_ZONE_ADD);
	  if (rv) { return rv; }
#if DEBUG_UTC
	  printf("opout = %s\n", dbg_pdate(&utcsrc));
#endif

	}

      // We synthetically zero utcsrc.ns so that the compare function
      // will return 0 when we are exactly on a leap second.
      // 
      // Also make sure that offsets work properly for leap seconds.
      // (could also duplicate entries in the table)
      memcpy(&to_cmp, &utcsrc, sizeof(timecalc_calendar_t));
      to_cmp.ns = 0; 
      if (to_cmp.second == 60)
	{
	  // Landed on a leap second. Remember to add one later.
	  current_leap = 1;
	  to_cmp.second = 59;
	}

      cmp_value = timecalc_calendar_cmp(&to_cmp, &current->when);

#if DEBUG_UTC
      printf("cmp[%d] = %d (%s)\n", i, cmp_value, dbg_pdate(&utcsrc));
      printf("          current: %s \n", dbg_pdate(&current->when));
#endif

      
      // If we are before this entry, the previous entry applies
      if (cmp_value < 0)
	{
	  break;
	}
      
      
      if (cmp_value == 0)
	{
	  // There is a leap second immediately following. The 
	  //  value is < current, we're going forward, else we're going back.
	  //
	  // Also table indices below UTC_LOOKUP_MIN_LEAP_SECOND are sync points
	  // and not leap seconds per se.
	  //
	  // If we landed on a leap second, there isn't one following. This is it.
	  int is_leap_second;

	  is_leap_second = !current_leap && 
	    (i >= UTC_LOOKUP_MIN_LEAP_SECOND) && 
	     (timecalc_interval_cmp(&utc_lookup_table[i-1].utctai, 
				    &current->utctai) > 0);

	  // If we'd hit a leap second or we were a few nanoseconds ahead, 
	  // it's this entry that applies, not the last one.
	  if (/* current_leap || */(!(is_leap_second) && utcsrc.ns))
	    {
	      iv = utc_lookup_table[i].utctai;
	    }
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
  dest->system = TIMECALC_SYSTEM_OFFSET;
#if DEBUG_UTC
  printf("utc_offset: dest = %s\n", dbg_pdate(dest));
#endif


  return 0;
}

static int system_utc_op(struct timecalc_zone_struct *self,
			 timecalc_calendar_t *dest,
			 const timecalc_calendar_t *src,
			 const timecalc_calendar_t *offset,
			 int op)
{
  timecalc_zone_t *gtai = (timecalc_zone_t *)self->handle;
  int rv ;

  // Right. To perform a fieldwise add on a UTC time, we:
  //
  //  - Work out the offset between src and TAI. 
  //  - Add src-1s to offset in TAI -> tdest.
  //  - Work out the offset betwen tdest and TAI
  //  - If a leap second follows,  this is that leap second.
  //  - If a leap second doesn't follow, add 1s and go again.
  //  - Either way, add the offset between src and dest to 
  //      the result in TAI.
  //
  // @todo There must be a better way .. 

  // OK. First off, work out the offset for the source.
  timecalc_calendar_t src_diff, dst_diff;
  int complex = 0;
  timecalc_calendar_t dst_value, tmp;
  int do_ls = 1;

  //if (src->system != TIMECALC_SYSTEM_UTC)
  // {
  //   return TIMECALC_ERR_NOT_MY_SYSTEM;
  // }

  if (op == TIMECALC_OP_COMPLEX_ADD)
    {
      op = TIMECALC_OP_SIMPLE_ADD;
      ++complex;
    }

  if (op == TIMECALC_OP_ZONE_ADD)
    {
      // This is a zone addition and therefore particularly easy.
      rv = gtai->op(gtai, &tmp, src, offset, TIMECALC_OP_ZONE_ADD);
#if DEBUG_UTC
      printf("utc_zone_op (for zone addition) rv = %d result = %s\n", rv, dbg_pdate(&tmp));
#endif

      if (rv) { return rv; }
    }
  else
    {
      rv = self->offset(self, &src_diff, src);
      if (rv < 0) { return rv; }
            
      rv = gtai->op(gtai, &dst_value, src, offset, op);
      if (rv < 0) { return rv; }
      
      // Now the destination offset.
      rv = self->offset(self, &dst_diff, &dst_value);
      if (rv < 0) { return rv; }

#if DEBUG_UTC
      printf("utc_op:  src_diff         = %s \n", dbg_pdate(&src_diff));
      printf("utc_op:  dst_value        = %s \n", dbg_pdate(&dst_value));
      printf("utc_op:  dst_diff         = %s \n", dbg_pdate(&dst_diff));
#endif
      
      // If source and destination diffs are the same, we can just return the result.
      if (!timecalc_calendar_cmp(&src_diff, &dst_diff))
	{
	  memcpy(dest, &dst_value, sizeof(timecalc_calendar_t));
	  return 0;
	}
      
      // Otherwise, the actual offset is (dst - src) + offset, knocked down by 
      // offset
      rv = timecalc_simple_op(&dst_diff, &dst_diff, &src_diff, TIMECALC_OP_SUBTRACT);
      if (rv) { return rv; }
      if (complex)
	{
	  do_knockdown(&dst_diff, offset, &do_ls);
	}
      
      rv = gtai->op(gtai, &tmp, &dst_value, &dst_diff, op);
      if (rv < 0) { return rv; }
#if DEBUG_UTC
      printf("-- utc_zone_op: Came up with %s \n", dbg_pdate(&tmp));
#endif

    }
  
  // By definition, tmp is now either a leap second or not. Note that we 
  // didn't know if it was a leap second in the previous step because the
  // leap seconds between two widely spaced dates will bring dates calculated
  // as e.g. 1980-01-01 00:00:02 down to 23:59:59 .
  //
  if (do_ls)
  {
    int i;
    const int nr_entries = sizeof(utc_lookup_table)/sizeof(utc_lookup_entry_t);
    timecalc_calendar_t one_second, r;
    long int saved_ns;

    memset(&one_second, '\0', sizeof(timecalc_calendar_t));

    // For a zone addition, there is no destination value and we've therefore
    // ended up on 'the wrong side' of a leap second.
    if (op == TIMECALC_OP_ZONE_ADD)
      {
	one_second.second = -1; 
      }
    else
      {
	one_second.second = 0;
      }

    rv = gtai->op(gtai, &r, &tmp, &one_second, TIMECALC_OP_SIMPLE_ADD);
    if (rv) { return rv; }

    saved_ns = r.ns; r.ns = 0;

#if DEBUG_UTC
    printf("Searching for leap second after: %s \n", dbg_pdate(&r));
#endif

    
    for (i = UTC_LOOKUP_MIN_LEAP_SECOND; i < nr_entries; ++i)
      {
	utc_lookup_entry_t *current = &utc_lookup_table[i];
	int cmp_value;

	cmp_value = timecalc_calendar_cmp(&r, &current->when);
#if DEBUG_UTC
	printf("i = %d, cmp = %d \n", i, cmp_value);
#endif

	if (!cmp_value)
	  {
	    // This is the leap second just after the calculated time.
	    ++r.second;
	    r.ns = saved_ns; // Restore nanoseconds.
	    memcpy(dest, &r, sizeof(timecalc_calendar_t));
#if DEBUG_UTC
	    printf("result was leap second : %s\n", 
		   dbg_pdate(dest));
#endif

	    return 0;
	  }
	if (cmp_value < 0)
	  {
	    // tmp < current->when - never going to happen now
	    break;
	  }
      }
  }

  // Not a leap second.
  memcpy(dest, &tmp, sizeof(timecalc_calendar_t));
#if DEBUG_UTC
  printf("result was not leap second: %s \n", 
	 dbg_pdate(dest));
#endif

  return 0;
}

#if 0
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
				 before, &src_offset, 1);
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
				 after, &dst_offset, 1);
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
#endif


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
		    int arg_i, void *arg_n)
{
  self->handle = (void *)arg_n;
  return 0;
}


static int system_utc_lower_zone(struct timecalc_zone_struct *self,
				 struct timecalc_zone_struct **next)
{
  (*next) = (struct timecalc_zone_struct *)(self->handle);
  return 0;
}

/* -------------------------------- UTC Plus --------------------------- */



static int utc_plus_init(struct timecalc_zone_struct *self, int arg_i, void *arg_n)
{
  self->handle = (void *)arg_n;
  return 0;
}

static int system_utcplus_offset(struct timecalc_zone_struct *self,
				 timecalc_calendar_t *dest,
				 const timecalc_calendar_t *src)
{
  // This is a straightforward offset from UTC. Leap seconds therefore
  // occur at times other than 23:59:59 .. 
  int mins = UTCPLUS_SYSTEM_TO_MINUTES(self->system);

  memset(dest, '\0', sizeof(timecalc_calendar_t));
  dest->hour = (mins/60);
  dest->minute = (mins % 60);
  
  return 0;
}

static int system_utcplus_aux(struct timecalc_zone_struct *self,
			      const timecalc_calendar_t *calc,
			      timecalc_calendar_aux_t *aux)
{

  // Works exactly the same way as UTC does.
  timecalc_zone_t *utc = (timecalc_zone_t *)self->handle;

  return utc->aux(utc, calc, aux);
}



static int system_utcplus_op(struct timecalc_zone_struct *self,
			     timecalc_calendar_t *dest,
			     const timecalc_calendar_t *src,
			     const timecalc_calendar_t *offset,
			     int op)
{
  timecalc_zone_t *utc = (timecalc_zone_t *)self->handle;
  
  // This is actually pretty civilised. For all operations, we first
  // subtract the relevant number of hours and minutes. Then we perform
  // the operation. Then we add the hours and minutes again. Because
  // these are offset additions, they don't change the leap second 
  // indicator and because they are strictly reversible, leap seconds
  // always end up 'where they were generated' in UTC times.
  
  timecalc_calendar_t adj, diff, tgt, srcx;
  int rv;
  int mins = UTCPLUS_SYSTEM_TO_MINUTES(self->system);

  memset(&diff, '\0', sizeof(timecalc_calendar_t));
  diff.hour  = -(mins/ 60);
  diff.minute = -(mins % 60);

  memcpy(&srcx, src, sizeof(timecalc_calendar_t));
  srcx.system = utc->system;

#if DEBUG_UTCPLUS
  printf("utcplus_op: src = %s\n", dbg_pdate(src));
#endif

  rv = utc->op(utc, &adj, &srcx, &diff, TIMECALC_OP_COMPLEX_ADD);
  if (rv) { return rv; }
  
#if DEBUG_UTCPLUS
  printf("utcplus_op: adj = %s \n", dbg_pdate(&adj));
#endif


  // Now perform whatever operation was originally required.
  rv = utc->op(utc, &tgt, &adj, offset, op);
  if (rv) { return rv; }

  // And adjust back.
  
  diff.hour = (mins/60);
  diff.minute = (mins%60);
  
  {
    int ls = 0;

    if (tgt.second == 60)
      {
	// Leap second! Pull it around . 
	ls =1;
	--tgt.second;
      }
    

#if DEBUG_UTCPLUS
    printf("utcplus_op: tgt = %s \n", dbg_pdate(&tgt));
#endif

    rv = utc->op(utc, dest, &tgt, &diff, TIMECALC_OP_COMPLEX_ADD);
    if (rv) { return rv; }
    
    if (ls) { ++dest->second; }
  }
  dest->system = self->system;

#if DEBUG_UTCPLUS
  printf("utcplus: dest = %s\n", dbg_pdate(dest));
#endif


  return 0;
}

int system_utcplus_epoch(struct timecalc_zone_struct *self,
			 timecalc_calendar_t *aux)
{
  timecalc_zone_t *utc = (timecalc_zone_t *)self->handle;
  return utc->epoch(self, aux);
}

int system_utcplus_lower_zone(struct timecalc_zone_struct *self,
			      struct timecalc_zone_struct **next)
{
  (*next) = (struct timecalc_zone_struct *)(self->handle);
  return 0;
}



static const char *dbg_pdate(const timecalc_calendar_t *cal)
{
  static char buf[128];
  timecalc_calendar_sprintf(buf, 128, cal);
  return buf;
}

/* ----------------------------- BST ------------------- */


/* BST: Applied to any other time zone.
 *
 * From <http://www.direct.gov.uk/en/Governmentcitizensandrights/LivingintheUK/DG_073741>
 *
 *  - In spring, the clocks go forward 1h at 0100 GMT on the last Sunday in March.
 *  - In autumn, the clocks go back 1h at 0200 BST on the last Sunday in October.
 */

static int is_bst(struct timecalc_zone_struct *z, const  timecalc_calendar_t *cal);


static int bst_init(struct timecalc_zone_struct *self,
	     int iarg, void *parg)
{
  self->handle = parg;
  return 0;
}


static int system_bst_offset(struct timecalc_zone_struct *self,
			     timecalc_calendar_t *offset,
			     const timecalc_calendar_t *src)
{
  // Is it after the last Sunday in march?
  timecalc_zone_t *utc = (timecalc_zone_t *)self->handle;
  int bst = is_bst(utc, src);
  if (bst < 0) { return bst; }
  

#if DEBUG_BST
  printf("system_bst_offset:  %s is_bst? %d \n",
	 dbg_pdate(src), bst);
#endif

  memset(offset, '\0', sizeof(timecalc_calendar_t));
      
  if (bst)
    {
      // One hour ahead
      offset->hour = 1;
    }
  return 0;
}

static int system_bst_op(struct timecalc_zone_struct *self,
			 timecalc_calendar_t *dest,
			 const timecalc_calendar_t *src,
			 const timecalc_calendar_t *offset,
			 int op)
{
  timecalc_zone_t *utc = (timecalc_zone_t *)self->handle;
  timecalc_calendar_t adj, diff, tgt, srcx;
  int rv;

#if DEBUG_BST
  printf("bst_op: src = %s \n", dbg_pdate(src));
#endif

  rv = self->offset(self, &diff, src);
  if (rv) { return rv; }

  // We're actually going backward here, so ..
  timecalc_negate(&diff);

  memcpy(&srcx, src, sizeof(timecalc_calendar_t));
  srcx.system = utc->system;
  
#if DEBUG_BST
  printf("bst_op: initial src offset = %s \n", dbg_pdate(&diff));
#endif


  rv = utc->op(utc, &adj, &srcx, &diff, TIMECALC_OP_COMPLEX_ADD);
  if (rv) { return rv; }

#if DEBUG_BST
  printf("bst_op: adj = %s \n", dbg_pdate(&adj));
#endif

  // Now we're in UTC.
  adj.system = utc->system;

#if DEBUG_BST
  printf("bst_op: -------- before utc->op ---- \n");
#endif


  rv = utc->op(utc, &tgt, &adj, offset, op);
  if (rv) { return rv; }

#if DEBUG_BST
  printf("bst_op: ------- after utc->op ----\n");
#endif
  
  rv = self->offset(self, &diff, &tgt);
  if (rv) { return rv; }
#if DEBUG_BST
  printf("bst_op: initial tgt = %s \n", dbg_pdate(&tgt));
  printf("bst_op: initial tgt offset = %s \n", dbg_pdate(&diff));
#endif

  // Luckily we need not think about this too hard as leap seconds
  // never happen at the same time as BST transitions.
  
  {
    int ls = 0;
    if (tgt.second == 60) { ls = 1; --tgt.second; }
    
    rv = utc->op(utc, dest, &tgt, &diff, TIMECALC_OP_COMPLEX_ADD);
    if (rv) { return rv; }
    if (ls) { ++dest->second; }
  }

  dest->system = self->system;

#if DEBUG_BST
  printf("bst_op: dest = %s \n", dbg_pdate(dest));
#endif

  return 0;
}

static int system_bst_aux(struct timecalc_zone_struct *self,
			  const timecalc_calendar_t *calc,
			  timecalc_calendar_aux_t *aux)
{
  timecalc_zone_t *utc = (timecalc_zone_t *)self->handle;
  int rv;

  rv = utc->aux(utc, calc, aux);
  if (rv) { return rv; }

  // Is it DST?
  aux->is_dst = is_bst(utc, calc);
  return 0;
}

static int system_bst_epoch(struct timecalc_zone_struct *self,
			    timecalc_calendar_t *aux)
{
  timecalc_zone_t *utc = (timecalc_zone_t *)self->handle;

  return utc->epoch(utc, aux);
}

static int system_bst_lower_zone(struct timecalc_zone_struct *self,
				 struct timecalc_zone_struct **next)
{
  (*next) = (struct timecalc_zone_struct *)self->handle;
  return 0;
}


static int is_bst(struct timecalc_zone_struct *utc, const timecalc_calendar_t *cal)
{
  // The date actually doesn't matter so ..
  if (cal->month < TIMECALC_MARCH || cal->month > TIMECALC_OCTOBER)
    {
      // Can't possibly be BST.
#if DEBUG_BST
      printf("is_bst: case A (can't be BST)\n");
#endif

      return 0;
    }
  if (cal->month > TIMECALC_MARCH && cal->month < TIMECALC_OCTOBER)
    {
      // Must be BST
#if DEBUG_BST
      printf("is_bst: case B (must be BST)\n");
#endif

      return 1;
    }

  // Otherwise ..
  if (cal->month == TIMECALC_MARCH || cal->month == TIMECALC_OCTOBER)
    {
      int is_march = (cal->month == TIMECALC_MARCH);
      // Luckily, March and October both have 31 days .. 


      if (cal->mday < (31-7))
	{
	  // There must be a sunday to come
#if DEBUG_BST
	  printf("  -> There must be a sunday to come.\n");
#endif

	  return is_march;
	}
      
      timecalc_calendar_aux_t aux;
      int rv;

      rv = utc->aux(utc, cal, &aux);
      if (rv) { return rv; }

      if (aux.wday == 0)
	{
	  // It's today!
	  if (cal->system == TIMECALC_SYSTEM_UTC && cal->hour >= 1)
	    {
#if DEBUG_BST
	      printf("  -> After UTC 0100: BST has turned on/off.\n");
#endif

	      return is_march;
	    }
	  if (cal->system == TIMECALC_SYSTEM_BST && cal->hour >= 2)
	    {
#if DEBUG_BST
	      printf("  -> After BST 0200: BST has turned on/off.\n");
#endif
	      return is_march;
	    }

#if DEBUG_BST
	  printf("On Sunday, but before BST switch.\n");
#endif

	  // Otherwise, the switch point occurs after this.
	  return !is_march;
	}

#if DEBUG_BST
      printf("is_bst: aux.wday = %d cal->mday = %d \n", 
	     aux.wday, cal->mday);
#endif

      // Is there going to be a Sunday?
      if ((7-aux.wday) <= (31-cal->mday))
	{
	  // Yes.
#if DEBUG_BST
	  printf("   -> There will be a sunday after this.\n");
#endif

	  return !is_march;
	}
	  
      // Otherwise, no
#if DEBUG_BST
      printf("  -> We're after last sunday in March/October.\n");
#endif

      return is_march;  
    }


  return TIMECALC_ERR_INTERNAL_ERROR;
}

/* --------------------------- rebased ------------------------ */

int timecalc_rebased_new(timecalc_zone_t **ozone,
			 const timecalc_calendar_t *offset,
			 timecalc_zone_t *based_on)
{
  int rv;
  timecalc_rebased_handle_t *hndl =
    (timecalc_rebased_handle_t *)malloc(sizeof(timecalc_rebased_handle_t));
  memset(hndl, '\0', sizeof(timecalc_rebased_handle_t));

  memcpy(&hndl->offset, offset, sizeof(timecalc_calendar_t));
  hndl->lower = based_on;

  rv = timecalc_zone_new(TIMECALC_SYSTEM_REBASED,
			 ozone,
			 0,
			 (void *)hndl);
  if (rv)
    {
      free(hndl);
      return rv;
    }
  
  return 0;
}

static int system_rebased_init(struct timecalc_zone_struct *self,
			       int argi,
			       void *argn)
{
  self->handle = (void *)argn;
  return 0;
}

static int system_rebased_dispose(timecalc_zone_t *self)
{
  int rv;
  timecalc_rebased_handle_t *h = 
    (timecalc_rebased_handle_t *)self->handle;

  rv = h->lower->dispose(h->lower);
  free(h->lower);
  free(h);
  return rv;
}

static int system_rebased_offset(struct timecalc_zone_struct *self,
				 timecalc_calendar_t *offset,
				 const timecalc_calendar_t *src)
{
  timecalc_rebased_handle_t *h = 
    (timecalc_rebased_handle_t *)self->handle;

  memcpy(offset, &h->offset, sizeof(timecalc_calendar_t));
  return 0;
}

static int system_rebased_aux(struct timecalc_zone_struct *self,
			      const timecalc_calendar_t *calc,
			      timecalc_calendar_aux_t *aux)
{
  timecalc_rebased_handle_t *h = 
    (timecalc_rebased_handle_t *)self->handle;
  
  return h->lower->aux(h->lower, calc, aux);
}

static int system_rebased_op(struct timecalc_zone_struct *self,
			     timecalc_calendar_t *dest,
			     const timecalc_calendar_t *src,
			     const timecalc_calendar_t *offset,
			     int op)
{
  timecalc_rebased_handle_t *h = 
    (timecalc_rebased_handle_t *)self->handle;
  timecalc_calendar_t adj, diff, tgt, srcx;
  int rv;

  memcpy(&diff, &h->offset, sizeof(timecalc_calendar_t));
  timecalc_negate(&diff);
  
  memcpy(&srcx, src, sizeof(timecalc_calendar_t));
  srcx.system = h->lower->system;

  // Adjust into lower.
 
  rv = h->lower->op(h->lower, &adj, &srcx, &diff, TIMECALC_OP_COMPLEX_ADD);
  if (rv) { return rv; }

#if DEBUG_REBASED
  printf("rebase_op:  src         = %s\n", dbg_pdate(&srcx));
  printf("            diff        = %s\n", dbg_pdate(&diff));
  printf("            adj         = %s\n", dbg_pdate(&adj));
#endif

  rv = h->lower->op(h->lower, &tgt, &adj, offset, op);
  if (rv) { return rv; }

#if DEBUG_REBASED
  printf("rebase_op[2]: offset   = %s\n", dbg_pdate(offset));
  printf("rebase_op[2]: tgt      = %s\n", dbg_pdate(&tgt));
#endif
  
  {
    int ls = 0;
    if (tgt.second == 60) { ls =1; --tgt.second; }
    
    rv = h->lower->op(h->lower, dest, &tgt, &h->offset, TIMECALC_OP_COMPLEX_ADD);
    if (rv) { return rv; }
    if (ls) { ++dest->second; }
  }

  dest->system = self->system;
#if DEBUG_REBASED
  printf("rebase_op[3]:   dest =  %s \n", dbg_pdate(dest));
#endif

  return 0;
}

static int system_rebased_epoch(struct timecalc_zone_struct *self,
				timecalc_calendar_t *aux)
{
  timecalc_rebased_handle_t *h = 
    (timecalc_rebased_handle_t *)self->handle;

  return h->lower->epoch(h->lower, aux);
}

static int system_rebased_lower_zone(struct timecalc_zone_struct *self,
				  struct timecalc_zone_struct **next)
{
  timecalc_rebased_handle_t *h = 
    (timecalc_rebased_handle_t *)self->handle;

  (*next) = h->lower;
  return 0;
}


static void do_knockdown(timecalc_calendar_t *io_diff, const timecalc_calendar_t *offset, int *do_ls)  
{
  int go = !!offset->year;
  // Year offsets always happen
  if (go) { io_diff->month = 0; }
  go = go || !!offset->month;
  if (go) { io_diff->mday = 0; }
  go = go || !!offset->hour; 
  if (go) { io_diff->hour = 0; }
  go = go || !!offset->minute;
  if (go) { io_diff->second = 0; (*do_ls) = 0; } 
  go = go || !!offset->second;
  if (go) { io_diff->ns = 0; }
}

static void timecalc_negate(timecalc_calendar_t *cal)
{
  cal->year = -cal->year;
  cal->month = -cal->month;
  cal->mday = -cal->mday;
  cal->hour = -cal->hour;
  cal->minute = -cal->minute;
  cal->second = -cal->second;
  cal->ns  = -cal->ns;
}

 
int timecalc_simple_op(timecalc_calendar_t *result,
			       const timecalc_calendar_t *a,
			       const timecalc_calendar_t *b,
			       int op)
{
  switch (op)
    {
    case TIMECALC_OP_SIMPLE_ADD:
    case TIMECALC_OP_COMPLEX_ADD:
    case TIMECALC_OP_ZONE_ADD:
      result->year = a->year + b->year;
      result->month = a->month + b->month;
      result->mday = a->mday + b->mday;
      result->hour = a->hour + b->hour;
      result->minute = a->minute + b->minute;
      result->second = a->second + b->second;
      result->ns = a->ns + b->ns;
      break;
    case TIMECALC_OP_SUBTRACT:
      result->year = a->year - b->year;
      result->month = a->month - b->month;
      result->mday = a->mday - b->mday;
      result->hour = a->hour - b->hour;
      result->minute = a->minute - b->minute;
      result->second = a->second - b->second;
      result->ns = a->ns - b->ns;
      break;
    default:
      return TIMECALC_ERR_INVALID_ARGUMENT;
    }

  result->system = a->system;
  return 0;
}

int timecalc_diff(timecalc_zone_t *z,
		  timecalc_interval_t *result,
		  const timecalc_calendar_t *before,
		  const timecalc_calendar_t *after)
{
  memset(result, '\0', sizeof(timecalc_interval_t));
  return z->diff(z, result, before, after);
}


/* End file */
