/* cdc.c */
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
 *  Implementation of cdatecalc.
 *
 * @author Richard Watts <rrw@kynesim.co.uk>
 * @date   2010-09-13
 */

#include <stdint.h>
#include "cdc/cdc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#define DEBUG_GTAI 0
#define DEBUG_UTC 0
#define DEBUG_UTCPLUS 0
#define DEBUG_BST 0
#define DEBUG_REBASED 0
#define DEBUG_RAISE 0
#define DEBUG_LOWER 0

#define DEBUG_ANY (DEBUG_GTAI || DEBUG_UTC || DEBUG_UTCPLUS || DEBUG_BST || \
		   DEBUG_REBASED || DEBUG_RAISE || DEBUG_LOWER)

#define ONE_MILLION 1000000
#define ONE_BILLION (ONE_MILLION * 1000)

//! @todo Could use gcc extensions to evaluate only once ..
#define MIN(x,y) (((x) < (y)) ? (x) : (y))
#define MAX(x,y) (((x) > (y)) ? (x) : (y))
#define SWAP(x,y) { __typeof(x) __tmp; __tmp = (x); (x) = (y); (y) = __tmp; }

#if DEBUG_ANY
static const char *dbg_pdate(const cdc_calendar_t *cal);
#endif


/** A generic diff function: lower both dates and then call diff()
 *  again.
 */
static int system_lower_diff(struct cdc_zone_struct *self,
			     cdc_interval_t *ival,
			     const cdc_calendar_t *before,
			     const cdc_calendar_t *after);


/** Dispose everyone, all the way down the tree */
static int chain_dispose(struct cdc_zone_struct *self);

static int null_init(struct cdc_zone_struct *self, int arg_i, void *arg_n);
static int null_dispose(struct cdc_zone_struct *self);

static int system_gtai_diff(struct cdc_zone_struct *self,
			    cdc_interval_t *ival,
			    const cdc_calendar_t *before,
			    const cdc_calendar_t *after);

static int system_gtai_offset(struct cdc_zone_struct *self,
			      cdc_calendar_t *offset,
			      const cdc_calendar_t *src);

static int system_gtai_aux(struct cdc_zone_struct *self,
			   const cdc_calendar_t *calc,
			   cdc_calendar_aux_t *aux);

static int system_gtai_op(struct cdc_zone_struct *self,
			   cdc_calendar_t *dest,
			   const cdc_calendar_t *src,
			   const cdc_calendar_t *offset, 
			   int op);


static int system_gtai_epoch(struct cdc_zone_struct *self,
			     cdc_calendar_t *aux);

static int system_gtai_lower_zone(struct cdc_zone_struct *self,
				  struct cdc_zone_struct **next);





static cdc_zone_t s_system_gtai = 
  {
    NULL,
    CDC_SYSTEM_GREGORIAN_TAI,
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
  cdc_calendar_t when;
 
  //! Value of utc-tai (i.e. add this to TAI to get UTC)
  cdc_interval_t utctai;
 
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
    { { 0, 0, 0, 0, 0, 0, 0, CDC_SYSTEM_UTC },
      { 0, 0 } 
    },

    // midnight 1 Jan 1961 UTC was TAI 1 Jan 1961 00:00:01.422818
    { { 1961, CDC_JANUARY, 1, 0, 0, 0, 0, CDC_SYSTEM_UTC },
      { -1,
	-422818000
      } 
    },

    // This is The Confused Period where the UTC second and the SI
    // second disagreed.

    // midnight 1 Jan 1972  UTC was TAI 1 Jan 1972 00:00:10 
    {
      { 1972, CDC_JANUARY, 1, 0, 0, 0, 0, CDC_SYSTEM_UTC },
      { -10, 0 }
    },

    // This is the start of leap second calculation
#define UTC_LOOKUP_MIN_LEAP_SECOND 3


    // The June 1972 leap second
    { { 1972, CDC_JUNE, 31, 23, 59, 59, 0, CDC_SYSTEM_UTC }, 
      { -11, 0 }
    },

    { { 1972, CDC_DECEMBER, 31, 23, 59, 59, 0, CDC_SYSTEM_UTC },
      { -12, 0 } 
    },

    { { 1973, CDC_DECEMBER, 31, 23, 59, 59, 0, CDC_SYSTEM_UTC },
      { -13, 0 }
    },

    { { 1974, CDC_DECEMBER, 31, 23, 59, 59, 0, CDC_SYSTEM_UTC },
      { -14, 0 }
    },
    
    { { 1975, CDC_DECEMBER, 31, 23, 59, 59, 0, CDC_SYSTEM_UTC },
      { -15, 0 }
    },
    
    { { 1976, CDC_DECEMBER, 31, 23, 59, 59, 0, CDC_SYSTEM_UTC },
      { -16, 0 }
    },

    { { 1977, CDC_DECEMBER, 31, 23, 59, 59, 0, CDC_SYSTEM_UTC },
      { -17, 0 }
    },
    
    { { 1978, CDC_DECEMBER, 31, 23, 59, 59, 0, CDC_SYSTEM_UTC },
      { -18, 0 }
    },
    
    { { 1979, CDC_DECEMBER, 31, 23, 59, 59, 0, CDC_SYSTEM_UTC },
      { -19, 0 }
    },
    
    { { 1981, CDC_JUNE, 30, 23, 59, 59, 0,  CDC_SYSTEM_UTC },
      { -20, 0 }
    },

    { { 1982, CDC_JUNE, 30, 23, 59, 59, 0,  CDC_SYSTEM_UTC },
      { -21, 0 }
    },

    { { 1983, CDC_JUNE, 30, 23, 59, 59, 0, CDC_SYSTEM_UTC },
      { -22, 0 }
    },

    { { 1985, CDC_JUNE, 30, 23, 59, 59, 0, CDC_SYSTEM_UTC },
      { -23, 0 }
    },
    
    { { 1987, CDC_DECEMBER, 31, 23, 59, 59, 0, CDC_SYSTEM_UTC },
      { -24, 0 }
    },

    { { 1989, CDC_DECEMBER, 31, 23, 59, 59, 0, CDC_SYSTEM_UTC },
      { -25, 0 }
    },

    { { 1990, CDC_DECEMBER, 31, 23, 59, 59, 0, CDC_SYSTEM_UTC },
      { -26, 0 }
    },

    { { 1992, CDC_JUNE, 30, 23, 59, 59, 0, CDC_SYSTEM_UTC },
      { -27, 0 }
    },

    { { 1993, CDC_JUNE, 30, 23, 59, 59, 0, CDC_SYSTEM_UTC },
      { -28, 0 }
    },

    { { 1994, CDC_JUNE, 30, 23, 59, 59, 0, CDC_SYSTEM_UTC },
      { -29, 0 }
    },

    { { 1995, CDC_DECEMBER, 31, 23, 59, 59, 0, CDC_SYSTEM_UTC },
      { -30, 0 }
    },

    { { 1997, CDC_JUNE, 30, 23, 59, 59, 0, CDC_SYSTEM_UTC },
      { -331, 0 }
    },

    { { 1998, CDC_DECEMBER, 31, 23, 59, 59, 0, CDC_SYSTEM_UTC },
      { -32, 0 }
    },

    { { 2005, CDC_DECEMBER, 31, 23, 59, 59, 0, CDC_SYSTEM_UTC },
      { -33 , 0 }
    },

    { { 2008, CDC_DECEMBER, 31, 23, 59, 59, 0, CDC_SYSTEM_UTC },
      { -34, 0 }
    }
  };
     

static int utc_init(struct cdc_zone_struct *self,
		    int arg_i, void *arg_n);


//static int system_utc_diff(struct cdc_zone_struct *self,
//			   cdc_interval_t *ival,
//			   const cdc_calendar_t *before,
//			   const cdc_calendar_t *after);

static int system_utc_offset(struct cdc_zone_struct *self,
			     cdc_calendar_t *offset,
			     const cdc_calendar_t *src);


static int system_utc_op(struct cdc_zone_struct *self,
			 cdc_calendar_t *dest,
			 const cdc_calendar_t *src,
			 const cdc_calendar_t *offset,
			 int op);		    



static int system_utc_aux(struct cdc_zone_struct *self,
			   const cdc_calendar_t *calc,
			   cdc_calendar_aux_t *aux);

static int system_utc_epoch(struct cdc_zone_struct *self,
			    cdc_calendar_t *aux);


static int system_utc_lower_zone(struct cdc_zone_struct *self,
				 struct cdc_zone_struct **next);

static cdc_zone_t s_system_utc = 
  {
    NULL,
    CDC_SYSTEM_UTC,
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
#define UTCPLUS_SYSTEM_TO_MINUTES(x) ((x) - (CDC_SYSTEM_UTCPLUS_BASE + 720))


static int utc_plus_init(struct cdc_zone_struct *self, int arg_i, void *arg_n);

#if 0
static int system_utcplus_diff(struct cdc_zone_struct *self,
			   cdc_interval_t *ival,
			   const cdc_calendar_t *before,
			   const cdc_calendar_t *after);
#endif

static int system_utcplus_offset(struct cdc_zone_struct *self,
			     cdc_calendar_t *offset,
				 const cdc_calendar_t *src);

static int system_utcplus_aux(struct cdc_zone_struct *self,
			      const cdc_calendar_t *calc,
			      cdc_calendar_aux_t *aux);

static int system_utcplus_op(struct cdc_zone_struct *self,
			     cdc_calendar_t *io_cal, 
			     const cdc_calendar_t *src,
			     const cdc_calendar_t *offset,
			     int op);

static int system_utcplus_epoch(struct cdc_zone_struct *self,
				cdc_calendar_t *aux);

static int system_utcplus_lower_zone(struct cdc_zone_struct *self,
				     struct cdc_zone_struct **next);


static cdc_zone_t s_system_utcplus = 
  {
    NULL,
    CDC_SYSTEM_UTCPLUS_BASE,
    utc_plus_init,
    null_dispose,
    system_lower_diff,
    system_utcplus_offset,
    system_utcplus_op,
    system_utcplus_aux,
    system_utcplus_epoch,
    system_utcplus_lower_zone 
  };


static int bst_init(struct cdc_zone_struct *self,
		    int iarg, void *parg);

static int system_bst_offset(struct cdc_zone_struct *self,
			     cdc_calendar_t *offset,
			     const cdc_calendar_t *src);

static int system_bst_op(struct cdc_zone_struct *self,
			 cdc_calendar_t *dest,
			 const cdc_calendar_t *src,
			 const cdc_calendar_t *offset,
			 int op);

static int system_bst_aux(struct cdc_zone_struct *self,
			  const cdc_calendar_t *calc,
			  cdc_calendar_aux_t *aux);

static int system_bst_epoch(struct cdc_zone_struct *self,
			    cdc_calendar_t *aux);

static int system_bst_lower_zone(struct cdc_zone_struct *self,
				 struct cdc_zone_struct **next);


static cdc_zone_t s_system_bst = 
  {
    NULL,
    CDC_SYSTEM_BST,
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


static int system_rebased_init(struct cdc_zone_struct *self,
			       int argi,
			       void *argn);

static int system_rebased_dispose(struct cdc_zone_struct *self);

static int system_rebased_offset(struct cdc_zone_struct *self,
			      cdc_calendar_t *offset,
			      const cdc_calendar_t *src);

static int system_rebased_aux(struct cdc_zone_struct *self,
			   const cdc_calendar_t *calc,
			   cdc_calendar_aux_t *aux);

static int system_rebased_op(struct cdc_zone_struct *self,
			   cdc_calendar_t *dest,
			   const cdc_calendar_t *src,
			   const cdc_calendar_t *offset, 
			   int op);


static int system_rebased_epoch(struct cdc_zone_struct *self,
			     cdc_calendar_t *aux);

static int system_rebased_lower_zone(struct cdc_zone_struct *self,
				  struct cdc_zone_struct **next);

// The handle in a rebased zone is actually quite complex..
typedef struct cdc_rebased_handle_struct
{
  cdc_zone_t *lower;
  cdc_calendar_t offset;
} cdc_rebased_handle_t;


static cdc_zone_t s_system_rebased = 
  {
    NULL,
    CDC_SYSTEM_REBASED,
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
static void do_knockdown(cdc_calendar_t *io_diff, const cdc_calendar_t *offset, 
			 int *do_ls);
static void cdc_negate(cdc_calendar_t *cal);

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

int cdc_interval_add(cdc_interval_t *result,
			  const cdc_interval_t *a,
			  const cdc_interval_t *b)
{
  int64_t tmp = (a->ns + b->ns);

  // The C standard doesn't specify the sign of the result
  // of % applied to -ve numbers :-(
  result->ns = (tmp < 0) ? -((-tmp) % ONE_BILLION) : 
    (tmp%ONE_BILLION);
  result->s = (a->s + b->s + (tmp / ONE_BILLION) );
  return 0;
}

int cdc_interval_subtract(cdc_interval_t *result,
			       const cdc_interval_t *a,
			       const cdc_interval_t *b)
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



int cdc_zone_new(int system,
		      cdc_zone_t **out_zone,
		      int arg_i,
		      void *arg_n)
{
  cdc_zone_t *prototype = NULL;
  int rv;

  if (system >= CDC_SYSTEM_UTCPLUS_BASE && 
      system <= (CDC_SYSTEM_UTCPLUS_BASE + 1440))
    {
      prototype = &s_system_utcplus;
    }
  
  switch (system)
    {
    case CDC_SYSTEM_GREGORIAN_TAI:
      prototype = &s_system_gtai;
      break;
    case CDC_SYSTEM_UTC:
      prototype = &s_system_utc;
      break;
    case CDC_SYSTEM_BST:
      prototype = &s_system_bst;
      break;
    case CDC_SYSTEM_REBASED:
      prototype = &s_system_rebased;
      break;
    default:
        // Intentional dropthrough ..
        break;
    }
  if (prototype == NULL)
    {
      (*out_zone) = NULL;
      return CDC_ERR_NO_SUCH_SYSTEM;
    }

  {
    cdc_zone_t *z = 
      (cdc_zone_t *)malloc(sizeof(cdc_zone_t));

    memcpy(z, prototype, sizeof(cdc_zone_t));
    z->system = system;
    rv = z->init(z, arg_i, arg_n);
    if (rv) 
      {
	free(z);
	return CDC_ERR_INIT_FAILED;
      }

    (*out_zone) = z;
  }
  
  return 0;
}

int cdc_zone_dispose(cdc_zone_t **io_zone)
{
  int rv = 0;
  
  if (!io_zone || !(*io_zone)) { return 0; }
  {
    cdc_zone_t *t = *io_zone;
    rv = t->dispose(t);
    free(t);
  }
  (*io_zone) = NULL;
  return rv;
}

int cdc_tai_new(cdc_zone_t **ozone)
{
  int rv;
  
  rv = cdc_zone_new(CDC_SYSTEM_GREGORIAN_TAI, ozone, 0, NULL);
  return rv;
}

int cdc_utc_new(cdc_zone_t **ozone)
{
  int rv;
  cdc_zone_t *z = NULL;
  
  rv = cdc_zone_new(CDC_SYSTEM_GREGORIAN_TAI, &z,
			 0, NULL);
  if (rv) { return rv; }

  rv = cdc_zone_new(CDC_SYSTEM_UTC, ozone, 0, z);
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

int cdc_utcplus_new(cdc_zone_t **ozone, int offset)
{
  int rv;
  cdc_zone_t *z = NULL;

  rv = cdc_utc_new(&z);
  if (rv) { return rv; }

  rv = cdc_zone_new(CDC_SYSTEM_UTCPLUS_ZERO + offset, ozone, 0, z);
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

int cdc_bst_new(cdc_zone_t **ozone)
{
  int rv;
  cdc_zone_t *z = NULL;

  rv = cdc_utc_new(&z);
  if (rv) { return rv; }

  rv = cdc_zone_new(CDC_SYSTEM_BST, ozone, 0, z);
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


int cdc_calendar_cmp(const cdc_calendar_t *a,
			  const cdc_calendar_t *b)
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
  // cdc_calendar_t s
  CVAL_HELPER(a->system, b->system);
#undef CVAL_HELPER

  return 0;
}

int cdc_interval_cmp(const cdc_interval_t *a,
			  const cdc_interval_t *b)
{
  if (a->s < b->s) { return -1; }
  if (a->s > b->s) { return 1; }
  if (a->ns < b->ns) { return -1; }
  if (a->ns > b->ns) { return 1; }
  return 0;
}

int cdc_zone_add(cdc_zone_t *zone,
		      cdc_calendar_t *out,
		      const cdc_calendar_t *date,
		      const cdc_interval_t *ival)
{
  int rv;
  int64_t s;
  cdc_calendar_t offset, tmp;


  memset(&offset, '\0', sizeof(cdc_calendar_t));
  memcpy(out, date, sizeof(cdc_calendar_t));

  s = ival->s;

  while (s > (1 << 30))
    {
      offset.second = (1<<30); 
      rv = cdc_op(zone, &tmp, out, &offset, CDC_OP_SIMPLE_ADD);
      memcpy(out, &tmp, sizeof(cdc_calendar_t));
      if (rv) { return rv; }
      s -= (1<<30);
    }
  while (s < -(1<<30))
    {
      offset.second = -(1<<30);
      rv = cdc_op(zone, &tmp, out, &offset, CDC_OP_SIMPLE_ADD);
      memcpy(out, &tmp, sizeof(cdc_calendar_t));
      if (rv) { return rv; }
      s += (1<<30);
    }

  offset.second = s;
  offset.ns = ival->ns;
  rv = cdc_op(zone, &tmp, out, &offset, CDC_OP_SIMPLE_ADD);
  memcpy(out, &tmp, sizeof(cdc_calendar_t));
  return rv;
}

int cdc_interval_sprintf(char *buf,
			      int n,
			      const cdc_interval_t *a)
{
  return snprintf(buf, n, "%"PRId64" s %ld ns", a->s, a->ns);
}

int cdc_interval_parse(cdc_interval_t *out,
                       const char *buf,
                       const int n)
{
    int rv;
    long long int s, ns;
    rv = sscanf(buf, "%lld s %lld ns", &s, &ns);
    if (rv != 2)
    {
        return CDC_ERR_CANNOT_CONVERT;
    }
    out->s = s;
    out->ns = ns;
    return 0;
}

int cdc_calendar_sprintf(char *buf,
			      int n,
			      const cdc_calendar_t *date)
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
		  cdc_describe_system(date->system));
}

int cdc_calendar_parse(cdc_calendar_t *date,
                       const char *buf,
                       const int n)
{
    int rv;
    int where = -1;
    int year, month, mday, hour, minute, second;
    long int ns;
                                                   

    rv = sscanf(buf, "%04d-%02d-%02d %02d:%02d:%02d.%09ld %n",
                &year, &month, &mday, &hour, &minute, &second, &ns, 
                &where);
    // %n apparently either does or does not get reflected in the
    // return value, depending on the whim of your C library
    // implementation.
    if (rv != 7 && rv != 8)
    {
        return CDC_ERR_CANNOT_CONVERT;
    }
    date->year = year;
    date->month = month-1;
    date->mday = mday;
    date->hour = hour;
    date->minute = minute;
    date->second = second;
    date->ns = ns;
    // Right. Now then .. 

    if (where < 0) 
    {
        return CDC_ERR_CANNOT_CONVERT;
    }
    rv = cdc_undescribe_system(&date->system, &buf[where]);
    return rv;
}

int cdc_interval_sgn(const cdc_interval_t *a)
{
  if (a->s > 0) { return 1; }
  if (a->s < 0) { return -1; }
  if (a->ns > 0) { return 1; }
  if (a->ns < 0) { return -1; }
  return 0;
}


const char *cdc_describe_system(const int sys)
{
  static char buf[128];
  int system = sys;
  const char *modifier = "";

  if (system & CDC_SYSTEM_TAINTED)
    {
      modifier = "*";
      system &= ~CDC_SYSTEM_TAINTED;
    }


  if (system >= CDC_SYSTEM_UTCPLUS_BASE && 
      system <= (CDC_SYSTEM_UTCPLUS_BASE + 1440))
    {
      int mins = UTCPLUS_SYSTEM_TO_MINUTES(system);

      // We describe 0 as '+' not '-' for convention's sake.
      if (mins >= 0)
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
    case CDC_SYSTEM_GREGORIAN_TAI:
      sprintf(buf, "TAI%s",modifier);
      break;
    case CDC_SYSTEM_UTC:
      sprintf(buf, "UTC%s", modifier);
      break;
    case CDC_SYSTEM_OFFSET:
      sprintf(buf, "OFF%s", modifier);
      break;
    case CDC_SYSTEM_BST:
      sprintf(buf, "BST%s", modifier);
      break;
    case (CDC_SYSTEM_REBASED & ~CDC_SYSTEM_TAINTED):
      sprintf(buf, "REBASED%s", modifier);
      break;
    default:
      return "UNKNOWN";
      break;
    }

  return buf;
}

int cdc_undescribe_system(unsigned int *out, const char *in_sys)
{
    int tainted = 0;
    int out_sys = 0;

    // Find out where in_sys ends.
    const char *sys_endp = in_sys;
    while (*sys_endp != '\0' && 
           !isspace(*sys_endp)) 
    {
        ++sys_endp;
    }
    if (sys_endp == in_sys) { return CDC_ERR_BAD_SYSTEM; }
    if (sys_endp[-1] == '*')
    {
        // A modifier!
        ++tainted;
    }
        
    if (!strncmp(in_sys, "TAI", 3))
    {
        out_sys = CDC_SYSTEM_GREGORIAN_TAI;
    }
    else if (!strncmp(in_sys, "UTC+", 4) || !strncmp(in_sys, "UTC-", 4))
    {
        int rv;
        int hrs, mins;
        int mul = (in_sys[3] == '-' ? -1 : 1);
        rv = sscanf(&in_sys[4], "%02d%02d", &hrs, &mins);

        if (rv != 2) { return CDC_ERR_BAD_SYSTEM; }
        out_sys =(CDC_SYSTEM_UTCPLUS_ZERO + 
             (mul * ((hrs * 60) + mins)));
    }
    else if (!strncmp(in_sys, "UTC", 3))
    {
        out_sys = CDC_SYSTEM_UTC;
    }
    else if (!strncmp(in_sys, "BST", 3))
    {
        out_sys = CDC_SYSTEM_BST;
    }
    else if (!strncmp(in_sys, "UNK", 3) || !strncmp(in_sys, "UNKNOWN", 7))
    {
        out_sys = CDC_SYSTEM_UNKNOWN;
    }
    else
    {
        return CDC_ERR_BAD_SYSTEM;
    }

    if (tainted) { out_sys |= CDC_SYSTEM_TAINTED; }
    (*out) = out_sys;
    return 0;
}


int cdc_op(struct cdc_zone_struct *zone,
		cdc_calendar_t *dst,
		const cdc_calendar_t *opa,
		const cdc_calendar_t *opb,
		int op)
{
  return zone->op(zone, dst, opa, opb, op);
}

int cdc_bounce(struct cdc_zone_struct *down_zone,
		    struct cdc_zone_struct *up_zone,
		    cdc_calendar_t *dst,
		    const cdc_calendar_t *src)
{
  cdc_zone_t *z;
  cdc_calendar_t tmp;
  int rv;

  rv = cdc_zone_lower_to(down_zone, &tmp, &z, src, -1);
  if (rv) { return rv; }

  rv = cdc_zone_raise(up_zone, dst, &tmp);
  return rv;
}
		   

int cdc_zone_raise(cdc_zone_t *zone,
			cdc_calendar_t *dest,
			const cdc_calendar_t *src)
{
  cdc_calendar_t dst_offset;
  cdc_calendar_t tmp;
  int rv;
  cdc_zone_t *low = NULL;
  
  rv = zone->lower_zone(zone, &low);
  if (rv) { return rv; }

#if DEBUG_RAISE
   printf("Raise %s to %d (low system %d)\n", dbg_pdate(src), zone->system, low->system);
#endif

   // If !low , there is no lower system.
   if (!low) { low = zone; }


  if (src->system == low->system) 
    {
      // If the system for src is incorrect, cal_offset() will
      // return an error.
      rv = zone->offset(zone, &dst_offset, src);
    }
  else
    {
      rv = CDC_ERR_NOT_MY_SYSTEM;
    }

  if (rv == CDC_ERR_NOT_MY_SYSTEM)
    {
#if DEBUG_RAISE
      printf("Can't raise %s \n", dbg_pdate(src));
#endif


      // Try lower.
      
      if (low)
	{
	  rv = cdc_zone_raise(low, &tmp, src);
	  if (rv) { return rv; }
	  src = &tmp;
	}
      else
	{
	  return CDC_ERR_NOT_MY_SYSTEM;
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
  

#if DEBUG_RAISE
  printf("Simple add of %s .. \n", dbg_pdate(&dst_offset));
#endif

  {
    cdc_calendar_t tmp;

    memcpy(&tmp, src, sizeof(cdc_calendar_t));
    tmp.system = zone->system;

    rv = cdc_op(zone, 
		     dest,
		     &tmp, &dst_offset, 
		     CDC_OP_ZONE_ADD);
  }

  if (rv) { return rv; }
  dest->system = zone->system;

#if DEBUG_RAISE
  printf("*** raised() dest = %s \n", dbg_pdate(dest));
#endif

  
  return 0;
}

int cdc_zone_lower_to(cdc_zone_t *zone,
			   cdc_calendar_t *dest,
			   cdc_zone_t **lower,
			   const cdc_calendar_t *src, 
			   int to_system)
{
  cdc_calendar_t current;

#if DEBUG_LOWER
  printf("Lower %d to %d (z = %d, h = 0x%08x).. \n", src->system, to_system, zone->system, 
	 (unsigned int)zone->handle);
#endif

  (*lower) = zone; // In case we can't lower any further.
  memcpy(&current, src, sizeof(cdc_calendar_t));
  while (current.system != (unsigned int)to_system)
    {
      cdc_zone_t *l = NULL;
      int rv;

      rv = zone->lower_zone(zone, &l);
      if (rv) 
	{
	  return rv; 
	}

#if DEBUG_LOWER
      printf("lower_zone: l = 0x%08x z = %d\n", (unsigned int) l, (l ? l->system :-1));
#endif

      if (!l) 
	{
#if DEBUG_LOWER
	  printf("No lower zone than %d (cur = %d)\n", zone->system, current.system);
#endif

	  if (to_system == -1)
	    {
	      // This is the lowest zone.
	      break;
	    }
	  else
	    {
	      // Error!
	      return CDC_ERR_CANNOT_CONVERT;
	    }
	}
      
      if (current.system == zone->system)
	{
	  // We can lower it - let's have a go.
#if DEBUG_LOWER
	  printf("Lower..\n");
#endif

	  rv = cdc_zone_lower(zone, dest, &l, &current);
	  if (rv) { return rv; }
#if DEBUG_LOWER
	  printf("lower to %d \n", l->system);
#endif

	  memcpy(&current, dest, sizeof(cdc_calendar_t));
	}
      
      // The lowest zone we've got .. 
      (*lower) = l;

      // Otherwise ..
      zone = l;
    }

  // In case we started off with target_zone == source_zone
  memcpy(dest, &current, sizeof(cdc_calendar_t));
  return 0;
}


int cdc_zone_lower(cdc_zone_t *zone,
			cdc_calendar_t *dest,
			cdc_zone_t **lower,
			const cdc_calendar_t *src)
{
  int rv;
  cdc_calendar_t offset;

  (*lower) = zone; // In case we can't lower any further.
  rv = zone->lower_zone(zone,lower);
  if (rv) { return rv;  }

  if (!(*lower))
    {
      // This is already the lowest zone.
      memcpy(dest, src, sizeof(cdc_calendar_t));
      return 0;
    }

  if (src->system != zone->system)
    {
      return CDC_ERR_NOT_MY_SYSTEM;
    }

  // Compute offset.
  rv = zone->offset(zone, &offset, src);
  if (rv) { return rv; }

  // Now ..

#if DEBUG_LOWER
  printf("src[2] = %s\n", dbg_pdate(src));
#endif


  memcpy(dest, src, sizeof(cdc_calendar_t));
  dest->system = (*lower)->system;  

#if DEBUG_LOWER
  printf("lower offset = %s \n",dbg_pdate(&offset));
#endif


  cdc_negate(&offset);

  // Add the offset in on a field-by-field basis.
  rv = cdc_op((*lower), dest, 
			     dest, &offset, CDC_OP_ZONE_ADD);
  if (rv) { return rv; }
  
  return 0;
}



/* -------------------- Generic NULL functions -------- */
static int null_init(struct cdc_zone_struct *self, int arg_i, void *arg_n)
{
  return 0;
}

static int null_dispose(struct cdc_zone_struct *self)
{
  return 0;
}

static int chain_dispose(struct cdc_zone_struct *self)
{
  cdc_zone_t *z = (cdc_zone_t *)self->handle;
  int rv;

  rv = z->dispose(z);
  free(z);
  return rv;
}

static int system_lower_diff(struct cdc_zone_struct *self,
			     cdc_interval_t *ivalp,
			     const cdc_calendar_t *before,
			     const cdc_calendar_t *after)
{
  cdc_calendar_t bl, al;
  cdc_zone_t *z;
  int rv;


  rv = cdc_zone_lower(self, &bl, &z, before);
  if (rv) { return rv; }

  rv = cdc_zone_lower(self, &al, &z, after);
  if (rv) { return rv; }

#if DEBUG_LOWER
  printf("system_lower_diff[] before = %s \n", dbg_pdate(before));
  printf("system_lower_diff[] bl     = %s \n", dbg_pdate(&bl));

  printf("system_lower_diff[] after   = %s \n", dbg_pdate(after));
  printf("system_lower_diff[] al     = %s \n", dbg_pdate(&al));
#endif

  return z->diff(z, ivalp, &bl, &al);
}


/* -------------------- Gregorian TAI ---------------- */

static int system_gtai_diff(struct cdc_zone_struct *self,
			    cdc_interval_t *ivalp,
			    const cdc_calendar_t *before,
			    const cdc_calendar_t *after)
{
  /** @todo
   *  
   *  We're using the conventional 'spinning counter' algorithm
   *  here. This is gratuitously terrible.
   *
   */

#if DEBUG_GTAI
  printf("---gtai_diff()\n");
#endif


  if (before->system != after->system) { return CDC_ERR_SYSTEMS_DO_NOT_MATCH; }
  if (before->system != CDC_SYSTEM_GREGORIAN_TAI) 
    {
      return CDC_ERR_NOT_MY_SYSTEM;
    }

  if (before->month < 0 || before->month > 12)
  {
      return CDC_ERR_INVALID_ARGUMENT;
  }
      

  if (cdc_calendar_cmp(before, after) > 0)
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
  
  cdc_interval_t ival;
  memcpy(&ival, ivalp, sizeof(cdc_interval_t));


#if DEBUG_GTAI
  printf("init: s = %"PRIu64" ns = %ld \n", 
	 ival.s, ival.ns);
#endif

  {
    int cur = before->month;
    int curday = before->mday;
    int curdays;
    int last = after->month;
    int lastday = after->mday;
    int is_leap = is_gregorian_leap_year(before->year);
    int curyear = before->year;
    int lastyear = after->year;

    curdays = gregorian_months[cur] + 
      ((is_leap && cur == CDC_FEBRUARY) ? 1 : 0);

    while (1)
      {

#if DEBUG_GTAI
          printf("-> d cur = %d curday = %d [ y = %d, leap = %d ]\n", cur, curday, curyear, is_leap);
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
	    ((is_leap && cur == CDC_FEBRUARY) ? 1 : 0);

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
              is_leap = is_gregorian_leap_year(curyear);
	    }
	}	      
      }
  }

#if DEBUG_GTAI
  printf("ival.s = %"PRIu64"\n", ival.s);
#endif

  {
    int hrdiff = after->hour - before->hour;

#if DEBUG_GTAI
    printf("hrdiff = %d \n", hrdiff);
#endif

    ival.s += SECONDS_PER_HOUR * hrdiff;
  }

  {
    int mdiff = after->minute - before->minute;

#if DEBUG_GTAI
    printf("mdiff = %d \n", mdiff);
#endif

    ival.s += SECONDS_PER_MINUTE *mdiff;
  }
#if DEBUG_GTAI
  printf("sdiff = %d \n", (after->second -before->second));
#endif

  ival.s += (after->second - before->second);
  ival.ns = (after->ns - before->ns);
  if (ival.ns < 0)
    {
      --ival.s;
      ival.ns += ONE_BILLION;
    }
  
  memcpy(ivalp, &ival, sizeof(cdc_interval_t));
#if DEBUG_GTAI
  printf("---end gtai_diff()\n");
#endif


  return 0;
}


static int system_gtai_offset(struct cdc_zone_struct *self,
				  cdc_calendar_t *offset,
			      const cdc_calendar_t *src)
{
  // Particularly easy ..
  memset(offset, '\0', sizeof(cdc_calendar_t));

  return 0;
}

static int system_gtai_op(struct cdc_zone_struct *self,
			  cdc_calendar_t *dest,
			  const cdc_calendar_t *src,
			  const cdc_calendar_t *offset,
			  int op)
{
  // And normalise .
  int done = 0;
  int rv;

  rv = cdc_simple_op(dest, src, offset, op);
#if DEBUG_GTAI
  printf("gtai_op: src                        = %s\n", dbg_pdate(src));
  printf("gtai_op: offset                     = %s\n", dbg_pdate(offset));
  printf("gtai_op: cdc_simple_op returns = %s\n", dbg_pdate(dest));
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

#if DEBUG_GTAI
  printf("gtai_op: dest before mday  =            %s\n",dbg_pdate(dest));
#endif


  dest->mday += (dest->hour / HOURS_PER_DAY);
  dest->hour = (dest->hour % HOURS_PER_DAY);

#if DEBUG_GTAI
  printf("gtai_op: dest after mday  =            %s\n",dbg_pdate(dest));
#endif

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

#if DEBUG_GTAI
  printf("gtai_op: normalised                = %s\n", dbg_pdate(dest));
#endif


  return 0;

}

static int system_gtai_aux(struct cdc_zone_struct *self,
			   const cdc_calendar_t *cal,
			   cdc_calendar_aux_t *aux)
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

static int system_gtai_epoch(struct cdc_zone_struct *self,
			     cdc_calendar_t *cal)
{
  // The 'epoch' for TAI is, rather arbitrarily, 1 January 1958
  cal->year = 1958;
  cal->month = 0;
  cal->mday = 1;
  cal->hour = 0;
  cal->minute = 0;
  cal->second = 0;
  cal->ns = 0;
  cal->system = CDC_SYSTEM_GREGORIAN_TAI;
  
  return 0;
}

static int system_gtai_lower_zone(struct cdc_zone_struct *self,
				  struct cdc_zone_struct **next)
{
  (*next) = NULL;
  return 0;
}

/* ---------------------- UTC ---------------------- */

static int system_utc_offset(struct cdc_zone_struct *self,
			     cdc_calendar_t *dest,
			     const cdc_calendar_t *src)
{
  const int nr_entries = sizeof(utc_lookup_table)/sizeof(utc_lookup_entry_t);
  cdc_interval_t iv = { 0, 0 };
  int i;
  int cmp_value;
  int src_tai;
  cdc_calendar_t utcsrc;

  if (src->system == CDC_SYSTEM_GREGORIAN_TAI)
    {
      // The source is in TAI.
      src_tai = 1;
    }
  else if (src->system == CDC_SYSTEM_UTC)
    {
      src_tai = 0;
      memcpy(&utcsrc, src, sizeof(cdc_calendar_t));
    }
  else
    {
      return CDC_ERR_NOT_MY_SYSTEM;
    }


#if DEBUG_UTC
  printf("cal_offset() %s \n", dbg_pdate(src));
#endif

  
  // First entry in the table is a sentinel.
  for (i = 1 ;i < nr_entries; ++i)
    {
      utc_lookup_entry_t *current = &utc_lookup_table[i];
      int current_leap = 0;
      cdc_calendar_t to_cmp;
      
      // UTC references itself (joy!) so that if the source is TAI we need
      // to add the current entry before comparing.
      if (src_tai)
	{
	  cdc_calendar_t off;
	  int rv;

	  memset(&off, '\0', sizeof(cdc_calendar_t));
	  
	  off.ns = utc_lookup_table[i].utctai.ns;
	  off.second = utc_lookup_table[i].utctai.s;

#if DEBUG_UTC
	  printf("src = %s\n", dbg_pdate(src));
#endif

	  rv = self->op(self, &utcsrc, src, &off, CDC_OP_ZONE_ADD);
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
      memcpy(&to_cmp, &utcsrc, sizeof(cdc_calendar_t));
      to_cmp.ns = 0; 
      if (to_cmp.second == 60)
	{
	  // Landed on a leap second. Remember to add one later.
	  current_leap = 1;
	  to_cmp.second = 59;
	}

      cmp_value = cdc_calendar_cmp(&to_cmp, &current->when);

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
	     (cdc_interval_cmp(&utc_lookup_table[i-1].utctai, 
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
  memset(dest, '\0', sizeof(cdc_calendar_t));



  // If we're going to UTC and this is a -ve leap second, add one to 
  // the number of seconds before we normalise and record that we're
  // in the 60th second later.
  dest->second += iv.s;
  dest->ns += iv.ns; 
  dest->system = CDC_SYSTEM_OFFSET;
#if DEBUG_UTC
  printf("utc_offset: dest = %s\n", dbg_pdate(dest));
#endif


  return 0;
}

static int system_utc_op(struct cdc_zone_struct *self,
			 cdc_calendar_t *dest,
			 const cdc_calendar_t *src,
			 const cdc_calendar_t *offset,
			 int op)
{
  cdc_zone_t *gtai = (cdc_zone_t *)self->handle;
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
  cdc_calendar_t src_diff, dst_diff;
  int complex = 0;
  cdc_calendar_t dst_value, tmp;
  int do_ls = 1;

  //if (src->system != CDC_SYSTEM_UTC)
  // {
  //   return CDC_ERR_NOT_MY_SYSTEM;
  // }

  if (op == CDC_OP_COMPLEX_ADD)
    {
      op = CDC_OP_SIMPLE_ADD;
      ++complex;
    }

  if (op == CDC_OP_ZONE_ADD)
    {
      // This is a zone addition and therefore particularly easy.
      rv = gtai->op(gtai, &tmp, src, offset, CDC_OP_ZONE_ADD);
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
      if (!cdc_calendar_cmp(&src_diff, &dst_diff))
	{
	  memcpy(dest, &dst_value, sizeof(cdc_calendar_t));
	  return 0;
	}
      
      // Otherwise, the actual offset is (dst - src) + offset, knocked down by 
      // offset
      rv = cdc_simple_op(&dst_diff, &dst_diff, &src_diff, CDC_OP_SUBTRACT);
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
    cdc_calendar_t one_second, r;
    long int saved_ns;

    memset(&one_second, '\0', sizeof(cdc_calendar_t));

    // For a zone addition, there is no destination value and we've therefore
    // ended up on 'the wrong side' of a leap second.
    if (op == CDC_OP_ZONE_ADD)
      {
	one_second.second = -1; 
      }
    else
      {
	one_second.second = 0;
      }

    rv = gtai->op(gtai, &r, &tmp, &one_second, CDC_OP_SIMPLE_ADD);
    if (rv) { return rv; }

    saved_ns = r.ns; r.ns = 0;

#if DEBUG_UTC
    printf("Searching for leap second after: %s \n", dbg_pdate(&r));
#endif

    
    for (i = UTC_LOOKUP_MIN_LEAP_SECOND; i < nr_entries; ++i)
      {
	utc_lookup_entry_t *current = &utc_lookup_table[i];
	int cmp_value;

	cmp_value = cdc_calendar_cmp(&r, &current->when);
#if DEBUG_UTC
	printf("i = %d, cmp = %d \n", i, cmp_value);
#endif

	if (!cmp_value)
	  {
	    // This is the leap second just after the calculated time.
	    ++r.second;
	    r.ns = saved_ns; // Restore nanoseconds.
	    memcpy(dest, &r, sizeof(cdc_calendar_t));
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
  memcpy(dest, &tmp, sizeof(cdc_calendar_t));
#if DEBUG_UTC
  printf("result was not leap second: %s \n", 
	 dbg_pdate(dest));
#endif

  return 0;
}

static int system_utc_aux(struct cdc_zone_struct *self,
			  const cdc_calendar_t *calc,
			  cdc_calendar_aux_t *aux)
{
  // Same as for the underlying calendar.
  cdc_zone_t *gtai = (cdc_zone_t *)self->handle;

  return gtai->aux(gtai, calc, aux);
}

static int system_utc_epoch(struct cdc_zone_struct *self,
			    cdc_calendar_t *aux)
{
  // cdc_zone_t *gtai = (cdc_zone_t *)self->handle;

  // The UTC epoch is 'properly' in 1961, but 1972 has the advantage
  // that it was when the UTC and TAI seconds harmonised.
  static const cdc_calendar_t epoch = 
    { 1972, CDC_JANUARY, 1, 0, 0, 0, 0, CDC_SYSTEM_UTC };
      
  memcpy(aux, &epoch, sizeof(cdc_calendar_t));
  return 0;
}

static int utc_init(struct cdc_zone_struct *self,
		    int arg_i, void *arg_n)
{
  self->handle = (void *)arg_n;
  return 0;
}


static int system_utc_lower_zone(struct cdc_zone_struct *self,
				 struct cdc_zone_struct **next)
{
  (*next) = (struct cdc_zone_struct *)(self->handle);
  return 0;
}

/* -------------------------------- UTC Plus --------------------------- */



static int utc_plus_init(struct cdc_zone_struct *self, int arg_i, void *arg_n)
{
  self->handle = (void *)arg_n;
  return 0;
}

static int system_utcplus_offset(struct cdc_zone_struct *self,
				 cdc_calendar_t *dest,
				 const cdc_calendar_t *src)
{
  // This is a straightforward offset from UTC. Leap seconds therefore
  // occur at times other than 23:59:59 .. 
  int mins = UTCPLUS_SYSTEM_TO_MINUTES(self->system);

  memset(dest, '\0', sizeof(cdc_calendar_t));
  dest->hour = (mins/60);
  dest->minute = (mins % 60);
  
  return 0;
}

static int system_utcplus_aux(struct cdc_zone_struct *self,
			      const cdc_calendar_t *calc,
			      cdc_calendar_aux_t *aux)
{

  // Works exactly the same way as UTC does.
  cdc_zone_t *utc = (cdc_zone_t *)self->handle;

  return utc->aux(utc, calc, aux);
}



static int system_utcplus_op(struct cdc_zone_struct *self,
			     cdc_calendar_t *dest,
			     const cdc_calendar_t *src,
			     const cdc_calendar_t *offset,
			     int op)
{
  cdc_zone_t *utc = (cdc_zone_t *)self->handle;
  
  // This is actually pretty civilised. For all operations, we first
  // subtract the relevant number of hours and minutes. Then we perform
  // the operation. Then we add the hours and minutes again. Because
  // these are offset additions, they don't change the leap second 
  // indicator and because they are strictly reversible, leap seconds
  // always end up 'where they were generated' in UTC times.
  
  cdc_calendar_t adj, diff, tgt, srcx;
  int rv;
  int mins = UTCPLUS_SYSTEM_TO_MINUTES(self->system);

  memset(&diff, '\0', sizeof(cdc_calendar_t));
  diff.hour  = -(mins/ 60);
  diff.minute = -(mins % 60);

  memcpy(&srcx, src, sizeof(cdc_calendar_t));
  srcx.system = utc->system;

#if DEBUG_UTCPLUS
  printf("utcplus_op: src = %s\n", dbg_pdate(src));
#endif

  rv = utc->op(utc, &adj, &srcx, &diff, CDC_OP_COMPLEX_ADD);
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

    rv = utc->op(utc, dest, &tgt, &diff, CDC_OP_COMPLEX_ADD);
    if (rv) { return rv; }
    
    if (ls) { ++dest->second; }
  }
  dest->system = self->system;

#if DEBUG_UTCPLUS
  printf("utcplus: dest = %s\n", dbg_pdate(dest));
#endif


  return 0;
}

int system_utcplus_epoch(struct cdc_zone_struct *self,
			 cdc_calendar_t *aux)
{
  cdc_zone_t *utc = (cdc_zone_t *)self->handle;
  return utc->epoch(self, aux);
}

int system_utcplus_lower_zone(struct cdc_zone_struct *self,
			      struct cdc_zone_struct **next)
{
  (*next) = (struct cdc_zone_struct *)(self->handle);
  return 0;
}



#if DEBUG_ANY
static const char *dbg_pdate(const cdc_calendar_t *cal)
{
  static char buf[128];
  cdc_calendar_sprintf(buf, 128, cal);
  return buf;
}
#endif

/* ----------------------------- BST ------------------- */


/* BST: Applied to any other time zone.
 *
 * From <http://www.direct.gov.uk/en/Governmentcitizensandrights/LivingintheUK/DG_073741>
 *
 *  - In spring, the clocks go forward 1h at 0100 GMT on the last Sunday in March.
 *  - In autumn, the clocks go back 1h at 0200 BST on the last Sunday in October.
 */

static int is_bst(struct cdc_zone_struct *z, const  cdc_calendar_t *cal);


static int bst_init(struct cdc_zone_struct *self,
	     int iarg, void *parg)
{
  self->handle = parg;
  return 0;
}


static int system_bst_offset(struct cdc_zone_struct *self,
			     cdc_calendar_t *offset,
			     const cdc_calendar_t *src)
{
  // Is it after the last Sunday in march?
  cdc_zone_t *utc = (cdc_zone_t *)self->handle;
  int bst = is_bst(utc, src);
  if (bst < 0) { return bst; }
  

#if DEBUG_BST
  printf("system_bst_offset:  %s is_bst? %d \n",
	 dbg_pdate(src), bst);
#endif

  memset(offset, '\0', sizeof(cdc_calendar_t));
      
  if (bst)
    {
      // One hour ahead
      offset->hour = 1;
    }
  return 0;
}

static int system_bst_op(struct cdc_zone_struct *self,
			 cdc_calendar_t *dest,
			 const cdc_calendar_t *src,
			 const cdc_calendar_t *offset,
			 int op)
{
  cdc_zone_t *utc = (cdc_zone_t *)self->handle;
  cdc_calendar_t adj, diff, tgt, srcx;
  int rv;

#if DEBUG_BST
  printf("bst_op: src = %s \n", dbg_pdate(src));
#endif

  rv = self->offset(self, &diff, src);
  if (rv) { return rv; }

  // We're actually going backward here, so ..
  cdc_negate(&diff);

  memcpy(&srcx, src, sizeof(cdc_calendar_t));
  srcx.system = utc->system;
  
#if DEBUG_BST
  printf("bst_op: initial src offset = %s \n", dbg_pdate(&diff));
#endif


  rv = utc->op(utc, &adj, &srcx, &diff, CDC_OP_COMPLEX_ADD);
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
    
    rv = utc->op(utc, dest, &tgt, &diff, CDC_OP_COMPLEX_ADD);
    if (rv) { return rv; }
    if (ls) { ++dest->second; }
  }

  dest->system = self->system;

#if DEBUG_BST
  printf("bst_op: dest = %s \n", dbg_pdate(dest));
#endif

  return 0;
}

static int system_bst_aux(struct cdc_zone_struct *self,
			  const cdc_calendar_t *calc,
			  cdc_calendar_aux_t *aux)
{
  cdc_zone_t *utc = (cdc_zone_t *)self->handle;
  int rv;

  rv = utc->aux(utc, calc, aux);
  if (rv) { return rv; }

  // Is it DST?
  aux->is_dst = is_bst(utc, calc);
  return 0;
}

static int system_bst_epoch(struct cdc_zone_struct *self,
			    cdc_calendar_t *aux)
{
  cdc_zone_t *utc = (cdc_zone_t *)self->handle;

  return utc->epoch(utc, aux);
}

static int system_bst_lower_zone(struct cdc_zone_struct *self,
				 struct cdc_zone_struct **next)
{
  (*next) = (struct cdc_zone_struct *)self->handle;
  return 0;
}


static int is_bst(struct cdc_zone_struct *utc, const cdc_calendar_t *cal)
{
  // The date actually doesn't matter so ..
  if (cal->month < CDC_MARCH || cal->month > CDC_OCTOBER)
    {
      // Can't possibly be BST.
#if DEBUG_BST
      printf("is_bst: case A (can't be BST)\n");
#endif

      return 0;
    }
  if (cal->month > CDC_MARCH && cal->month < CDC_OCTOBER)
    {
      // Must be BST
#if DEBUG_BST
      printf("is_bst: case B (must be BST)\n");
#endif

      return 1;
    }

  // Otherwise ..
  if (cal->month == CDC_MARCH || cal->month == CDC_OCTOBER)
    {
      int is_march = (cal->month == CDC_MARCH);
      // Luckily, March and October both have 31 days .. 


      if (cal->mday < (31-7))
	{
	  // There must be a sunday to come
#if DEBUG_BST
	  printf("  -> There must be a sunday to come.\n");
#endif

	  return is_march;
	}
      
      cdc_calendar_aux_t aux;
      int rv;

      rv = utc->aux(utc, cal, &aux);
      if (rv) { return rv; }

      if (aux.wday == 0)
	{
	  // It's today!
	  if (cal->system == CDC_SYSTEM_UTC && cal->hour >= 1)
	    {
#if DEBUG_BST
	      printf("  -> After UTC 0100: BST has turned on/off.\n");
#endif

	      return is_march;
	    }
	  if (cal->system == CDC_SYSTEM_BST && cal->hour >= 2)
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


  return CDC_ERR_INTERNAL_ERROR;
}

/* --------------------------- rebased ------------------------ */

int cdc_rebased_new(cdc_zone_t **ozone,
			 const cdc_calendar_t *offset,
			 cdc_zone_t *based_on)
{
  int rv;
  cdc_rebased_handle_t *hndl =
    (cdc_rebased_handle_t *)malloc(sizeof(cdc_rebased_handle_t));
  memset(hndl, '\0', sizeof(cdc_rebased_handle_t));

  memcpy(&hndl->offset, offset, sizeof(cdc_calendar_t));
  hndl->lower = based_on;

  rv = cdc_zone_new(CDC_SYSTEM_REBASED,
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

static int system_rebased_init(struct cdc_zone_struct *self,
			       int argi,
			       void *argn)
{
  self->handle = (void *)argn;
  return 0;
}

static int system_rebased_dispose(cdc_zone_t *self)
{
  cdc_rebased_handle_t *h = 
    (cdc_rebased_handle_t *)self->handle;
  // Don't free h->lower: may be needed elsewhere

  free(h);
  return 0;
}

static int system_rebased_offset(struct cdc_zone_struct *self,
				 cdc_calendar_t *offset,
				 const cdc_calendar_t *src)
{
  cdc_rebased_handle_t *h = 
    (cdc_rebased_handle_t *)self->handle;

  memcpy(offset, &h->offset, sizeof(cdc_calendar_t));
  return 0;
}

static int system_rebased_aux(struct cdc_zone_struct *self,
			      const cdc_calendar_t *calc,
			      cdc_calendar_aux_t *aux)
{
  cdc_rebased_handle_t *h = 
    (cdc_rebased_handle_t *)self->handle;
  
  return h->lower->aux(h->lower, calc, aux);
}

static int system_rebased_op(struct cdc_zone_struct *self,
			     cdc_calendar_t *dest,
			     const cdc_calendar_t *src,
			     const cdc_calendar_t *offset,
			     int op)
{
  cdc_rebased_handle_t *h = 
    (cdc_rebased_handle_t *)self->handle;
  cdc_calendar_t adj, diff, tgt, srcx;
  int rv;

  memcpy(&diff, &h->offset, sizeof(cdc_calendar_t));
  cdc_negate(&diff);
  
  memcpy(&srcx, src, sizeof(cdc_calendar_t));
  srcx.system = h->lower->system;

  // Adjust into lower.
 
  rv = h->lower->op(h->lower, &adj, &srcx, &diff, CDC_OP_COMPLEX_ADD);
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
    
    rv = h->lower->op(h->lower, dest, &tgt, &h->offset, CDC_OP_COMPLEX_ADD);
    if (rv) { return rv; }
    if (ls) { ++dest->second; }
  }

  dest->system = self->system;
#if DEBUG_REBASED
  printf("rebase_op[3]:   dest =  %s \n", dbg_pdate(dest));
#endif

  return 0;
}

static int system_rebased_epoch(struct cdc_zone_struct *self,
				cdc_calendar_t *aux)
{
  cdc_rebased_handle_t *h = 
    (cdc_rebased_handle_t *)self->handle;

  return h->lower->epoch(h->lower, aux);
}

static int system_rebased_lower_zone(struct cdc_zone_struct *self,
				  struct cdc_zone_struct **next)
{
  cdc_rebased_handle_t *h = 
    (cdc_rebased_handle_t *)self->handle;

  (*next) = h->lower;
  return 0;
}


static void do_knockdown(cdc_calendar_t *io_diff, const cdc_calendar_t *offset, int *do_ls)  
{
  // If we've suppressed knockdown, do nothing.
  if (offset->flags & CDC_FLAG_AS_IF_NS) { return; }

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

static void cdc_negate(cdc_calendar_t *cal)
{
  cal->year = -cal->year;
  cal->month = -cal->month;
  cal->mday = -cal->mday;
  cal->hour = -cal->hour;
  cal->minute = -cal->minute;
  cal->second = -cal->second;
  cal->ns  = -cal->ns;
}

 
int cdc_simple_op(cdc_calendar_t *result,
			       const cdc_calendar_t *a,
			       const cdc_calendar_t *b,
			       int op)
{
  switch (op)
    {
    case CDC_OP_SIMPLE_ADD:
    case CDC_OP_COMPLEX_ADD:
    case CDC_OP_ZONE_ADD:
      result->year = a->year + b->year;
      result->month = a->month + b->month;
      result->mday = a->mday + b->mday;
      result->hour = a->hour + b->hour;
      result->minute = a->minute + b->minute;
      result->second = a->second + b->second;
      result->ns = a->ns + b->ns;
      break;
    case CDC_OP_SUBTRACT:
      result->year = a->year - b->year;
      result->month = a->month - b->month;
      result->mday = a->mday - b->mday;
      result->hour = a->hour - b->hour;
      result->minute = a->minute - b->minute;
      result->second = a->second - b->second;
      result->ns = a->ns - b->ns;
      break;
    default:
      return CDC_ERR_INVALID_ARGUMENT;
    }

  result->system = a->system;
  return 0;
}

int cdc_diff(cdc_zone_t *z,
		  cdc_interval_t *result,
		  const cdc_calendar_t *before,
		  const cdc_calendar_t *after)
{
  memset(result, '\0', sizeof(cdc_interval_t));
  return z->diff(z, result, before, after);
}

int cdc_rebased_tai(struct cdc_zone_struct **dst,
			 struct cdc_zone_struct *human_zone,
			 const cdc_calendar_t *human_time,
			 const cdc_calendar_t *machine_time)
{
  int rv;
  cdc_calendar_t c1;
  cdc_interval_t iv;
  cdc_zone_t *lzone;

#if DEBUG_REBASED
  printf("> rebased_tai()\n");
#endif

  rv = cdc_zone_lower_to(human_zone, &c1, &lzone, human_time, machine_time->system);
  if (rv) { return rv; }

  // Otherwise ..
  rv = cdc_diff(lzone, &iv, &c1, machine_time);
  if (rv) { return rv; }

  cdc_calendar_t offset;

  memset(&offset, '\0', sizeof(cdc_calendar_t));
  offset.second = iv.s;
  offset.ns = iv.ns;
  offset.flags |= CDC_FLAG_AS_IF_NS;
  offset.system = CDC_SYSTEM_OFFSET;
  
  rv = cdc_rebased_new(dst, &offset, lzone);
  return rv;
}


/* End file */
