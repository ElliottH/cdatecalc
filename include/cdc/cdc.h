/* cdc.h */
/* (C) Kynesim Ltd 2010 */

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


#ifndef CDC_H_INCLUDED
#define CDC_H_INCLUDED

/** @file
 *
 * This is cdc ; a very simple C library of time calculation functions
 *
 * cdc acts as a replacement for the UNIX mktime()/gmtime()/etc functions
 * based on a rather more rigorous notion of time than those functions.
 *
 * Among the things we make explicit are leap second handling and
 *  calendar origin. We also have some support for 'grounded' UTCs -
 *  that is, time zones which began synchronised with UTC at some
 *  point in the past but have since drifted due to the absence of
 *  leap second corrections. 
 *
 * Most non-ntp computer systems in fact use 'grounded' UTCs.
 *
 * Though this library is in the public domain, please _please_ fix
 * any bugs and contribute those fixes back to the main source tree.
 *
 * Some conventions I have tried to stick to:
 *
 *   - Fields here generally mean what they do in struct tm / unix
 *      time_t.
 *
 *   - No unsigned arithmetic. This makes direct ports to languages
 *      like Java easier, allows for more natural behaviour of 
 *      operations like calendar-to-interval which can now easily
 *      produce negative intervals if the time to be converted is
 *      before the epoch, and helps to minimise bugs due to over-
 *      or under-flow.
 *
 * Among the things this library doesn't do:
 *
 *   - Non-Roman calendar systems. We assume that your calendar is
 *     made of years, months, days, hours, minutes, seconds, fs.  Bad
 *     news for any Aztecs using this (any Babylonians out there are
 *     probably in luck, however. Except for their civilisation having
 *     died out several thousand years ago of course).
 *
 *   - Our maximum resolution is 1 ns. This is probably not enough
 *     for most high-precision applications.
 *
 *   - Astronomical time; our maximum interval is about 5e10 s, which
 *      is not long enough if you're into cosmology in any serious
 *      way.
 *
 *   - GR corrections. We assume that you are always at sea level and
 *     this is probably not strictly (or even remotely) true. Again,
 *     feel free to fix this.
 *
 *   - GPS time - I've not had the need yet. This is probably actually
 *     worth fixing.
 *
 *   - I have made precisely no concessions to embedded devices.
 *
 */

/** A sentinel to ensure that this invalid calendar date doesn't
 *  accidentally succeed in being used in calculations
 */
#define CDC_SYSTEM_INVALID          -1

/** TAI, Gregorian calendar */
#define CDC_SYSTEM_GREGORIAN_TAI     0 

/** UTC */
#define CDC_SYSTEM_UTC               2

/** BST */
#define CDC_SYSTEM_BST               3

/** UTC plus an offset of 0 (-1200) to 1440 (+1200) */
#define CDC_SYSTEM_UTCPLUS_BASE           0x1000
#define CDC_SYSTEM_UTCPLUS_ZERO          (CDC_SYSTEM_UTCPLUS_BASE + (60*12))


/** An offset */
#define CDC_SYSTEM_OFFSET            4

/** ORd into a system to indicate that it is in some way unconventional;
 *  typically used for rebased systems
 */
#define CDC_SYSTEM_TAINTED           (1 << 30)

/** Symbolic constant for the lowest system you can find */
#define CDC_SYSTEM_LOWEST            5

/** rebased: this allows you to add or subtract a fixed calendar_t
 *  from a time system. It's typically used to translate a computer
 *  time system to a real time system like BST:
 *
 *   - Computer times are measured in GREGORIAN_TAI
 *   - Human times are measured in BST.
 *   - Construct a scale CDC_SYSTEM_REBASED + offset, 
 *     giving the offset between a human time lowered to
 *     TAI and a computer time.
 *   - Now, when you read a computer time put it in 
 *     REBASED, lower to TAI and raise to BST.
 *
 * cdc_rebased_tai() and cdc_bounce() give some support
 *  for this model.
 */
#define CDC_SYSTEM_REBASED           (CDC_SYSTEM_TAINTED | 6)



//! You've tried to perform an operation on an unknown system.
#define CDC_ERR_NO_SUCH_SYSTEM (-4000)

//! You've tried to operate on dates whose systems don't match.
#define CDC_ERR_SYSTEMS_DO_NOT_MATCH (-3999)

//! Attempt to perform an operation on system A with a date of
//! system B
#define CDC_ERR_NOT_MY_SYSTEM        (-3998)

//! Attempt to perform an operation on a date for which the given
//! timezone is not (well) defined.
#define CDC_ERR_UNDEFINED_DATE       (-3997)

//! Initialisation or disposal failed.
#define CDC_ERR_INIT_FAILED          (-3996)

//! Bad time system
#define CDC_ERR_BAD_SYSTEM           (-3995)

//! Invalid argument
#define CDC_ERR_INVALID_ARGUMENT     (-3994)

//! Internal error
#define CDC_ERR_INTERNAL_ERROR       (-3993)

//! Cannot convert
#define CDC_ERR_CANNOT_CONVERT       (-3992)


/** Represents an interval.
 *
 *  2^63s is about 5e10 years. Note that over such long timescales the
 * leap second model will fail in quite nasty ways. Though since the
 * sun's remaining lifespan is only about 2-4e9 years you hopefully
 * don't care.
 */
typedef struct cdc_interval_struct
{
  /** Elapsed seconds */
  int64_t s;

  /** Elapsed nanoseconds */
  long int ns;
  
} cdc_interval_t;

/** Represents a calendar time (wall clock or zone corrected) */
typedef struct cdc_calendar_struct
{
  /** Year */
  int year;


#define CDC_JANUARY    0
#define CDC_FEBRUARY   1
#define CDC_MARCH      2
#define CDC_APRIL      3
#define CDC_MAY        4
#define CDC_JUNE       5
#define CDC_JULY       6
#define CDC_AUGUST     7 
#define CDC_SEPTEMBER  8
#define CDC_OCTOBER    9
#define CDC_NOVEMBER  10
#define CDC_DECEMBER  11 


  /** Month (0-11) */
  int month;

  /** Day of the month (1-31) */
  int mday;

  /** Hour (0-23) */
  int hour;

  /** Minute (0-59) */
  int minute;

  /** Second (0-60) [ leap seconds show up as '60' ] */
  int second;

  /** fs */
  long int ns;

  /** The time system in which this calendar time is expressed */
  uint32_t system;

#define CDC_FLAG_AS_IF_NS (1<<0)
  /** Flags: the only important one at the moment is AS_IF_NS: this
   *  means that an offset added to a time will not suppress the
   *  carry-forward of any DST offsets below that number 
   *  (i.e. it's treated as an offset in nanoseconds)
   */
  uint32_t flags;

} cdc_calendar_t;

/** Other things you might like to know when converting to a calendar
 *  time but which aren't strictly part of the time itself.
 */
typedef struct cdc_calendar_aux_struct
{
  /** Day of the week (Sunday = 0, Saturday = 6) */
  int wday;

#define CDC_SUNDAY 0
#define CDC_MONDAY 1
#define CDC_TUESDAY 2
#define CDC_WEDNESDAY 3
#define CDC_THURSDAY 4
#define CDC_FRIDAY 5
#define CDC_SATURDAY 6

  /** Day of the year (0-365) */
  int yday;

  /** Are we in some form of DST? */
  int is_dst;

} cdc_calendar_aux_t;

/** Represents a time zone 
 *
 *  Each time zone has a calendar and an offset.
 *
 *  The calendar is the underlying monotonic date system to which this time zone
 *  applies. It is required to be at least piecewise continuous and attempts to
 *  perform calculations with discontinuous portions of the calendar (e.g. the 
 *  'lost days' between Julian and Gregorian) are likely to return 
 *  CDC_ERR_UNDEFINED_DATE.
 *
 *  The offset is the (typically discontinuous) offset from that monotonic calendar
 *  induced by the conversion to sidereal time,  your current longitude, DST, etc.
 *
 *  The distinction is that offsets to your current date and time are applied to
 *  the calendar and then corrected by the offset.
 */
typedef struct cdc_zone_struct
{
  //! 'user' handle.
  void *handle;
  
  //! The system this zone handles.
  uint32_t system;

  /** Initialize this zone structure; mainly used internally */
  int (*init)(struct cdc_zone_struct *self, int arg_i, void *arg_n);

  /** Kill this zone structure (but don't free() it - just any 
   *   internal data) - mainly used by cdc_zone_dispose() 
   */
  int (*dispose)(struct cdc_zone_struct *self);


  /** Add the amount of time elapsed between 'before' and 'after' to 
   *  ival.
   *
   * @return 0 on success , < 0 on failure.
   */
  int (*diff)(struct cdc_zone_struct *self,
	      cdc_interval_t *ival,
	      const cdc_calendar_t *before,
	      const cdc_calendar_t *after);

  /** Obtain the calendar_t required to be added to the underlying
   *  calendar to get it into this time zone.
   *
   *  Note that the quantities in this calendar_t are not normalised;
   *   they are in 'natural' form, because cdc_add_offset() needs
   *   to know which additional offsets to knock out when running -
   *   see the documentation for that function for details.
   *
   *  Though it makes no difference to the actual offset, (*leap_second)
   *   will be set to 1 when a leap second follows the time in src.
   *
   * cal_offset() should normally be able to take a src of either 
   *  the native system for this zone or that of the lower zone.
   */
  int (*offset)(struct cdc_zone_struct *self,
		cdc_calendar_t *offset,
		const cdc_calendar_t *src);

  /** Fieldwise addition.
   */
  int (*op)(struct cdc_zone_struct *self,
	    cdc_calendar_t *dest,
	    const cdc_calendar_t *src,
	    const cdc_calendar_t *offset,
	    int op);		    

  /** Compute auxilliary info for a calendar date */
  int (*aux)(struct cdc_zone_struct *self,
	     const cdc_calendar_t *cal,
	     cdc_calendar_aux_t *aux);

  /** Retrieve the epoch for this system; this isn't the minimum
   *  (or maximum) value representable, but it does provide some 
   *  sort of anchor to base the system on.
   */
  int (*epoch)(struct cdc_zone_struct *self,
	       cdc_calendar_t *epoch);

  /** Obtain a pointer to 'the next zone down' in the heirarchy,
   *   if there is one 
   */
  int (*lower_zone)(struct cdc_zone_struct *self,
		    struct cdc_zone_struct **next);

} cdc_zone_t;

/** Add two intervals */
int cdc_interval_add(cdc_interval_t *result,
			  const cdc_interval_t *a,
			  const cdc_interval_t *b);

/** Compute a-b */
int cdc_interval_subtract(cdc_interval_t *result,
			       const cdc_interval_t *a,
			       const cdc_interval_t *b);

/** Compare two dates numerically (note that this says, in general,
 *  nothing about which came first). 
 *
 * @return -1 if a < b, 0 if a==b, 1 if a > b 
 */
int cdc_calendar_cmp(const cdc_calendar_t *a, 
			  const cdc_calendar_t *b);

/** Compare two intervals
 *
 * @return -1 if a < b, 0 if a == b, 1 if a > b 
 */
int cdc_interval_cmp(const cdc_interval_t *a,
			  const cdc_interval_t *b);

/** Print an interval in %lld.%lld s format */
int cdc_interval_sprintf(char *buf, 
			      int n,
			      const cdc_interval_t *a);

/** Print a calendar time in ISO format */
int cdc_calendar_sprintf(char *buf,
			      int n,
			      const cdc_calendar_t *date);

/** Return the sign of an interval */
int cdc_interval_sgn(const cdc_interval_t *a);			      

/** Describe this system as a string, returned in a static buffer */
const char *cdc_describe_system(const int system);

/** Add two calendar times fieldwise */
int cdc_simple_op(cdc_calendar_t *result,
		       const cdc_calendar_t *a,
			const cdc_calendar_t *b, 
			int op);
			

/** Find the difference between two calendar times; this is 
 *  really a shim on top of the diff() method 
 *
 *  Unlike the diff() method, this function zeroes result,
 *  so can't (easily) accumulate dates.
 */
int cdc_diff(cdc_zone_t *z,
		  cdc_interval_t *result,
		  const cdc_calendar_t *before,
		  const cdc_calendar_t *after);



/** Add an interval to a calendar time and normalise. This is essentially
 *  a repeated add.
 */
int cdc_zone_add(cdc_zone_t *zone,
		      cdc_calendar_t *out,
		      const cdc_calendar_t *date,
		      const cdc_interval_t *ival);


// dst = a + b
#define CDC_OP_SIMPLE_ADD 0

// dst = a - b 
#define CDC_OP_SUBTRACT 1

/** Add 'offset' to 'src', and put the result in 'dst'
 *
 *  After discussions with Tony and JC, it appears that the correct
 *  way to do this is as follows;
 *
 *   * Convert src to TAI , noting the calendar_t -type offset 'o_src'
 *   * Add offset to src in TAI, obtaining d_1
 *   * Work out what the calendar_t offset between d_1 and dst is , 'o_dst'
 *   * For all fields in offset which are non-zero, make o_dst.field = o_src.field for
 *       fields below them in the order year > month > day > hour > minute > second > ns
 *   * Add (o_dst - o_src) to get the result.
 *   * Normalise again to get a normalised result.
 *
 *  The result's system is src->system .
 * 
 */
#define CDC_OP_COMPLEX_ADD 2

/* A timezone addition performs addition with no consideration of time zone
 *  offsets at all - it's used when raising and lowering calendar_ts 
 */
#define CDC_OP_ZONE_ADD 3


int cdc_op(struct cdc_zone_struct *zone,
		cdc_calendar_t *dst,
		const cdc_calendar_t *src,
		const cdc_calendar_t *offset,
		int op);

/** 'Bounce' a date down from 'down_zone' to its base, then up-convert
 *  to 'up_zone' . 
 */
int cdc_bounce(struct cdc_zone_struct *down_zone,
		    struct cdc_zone_struct *up_zone,
		    cdc_calendar_t *dst,
		    const cdc_calendar_t *src);

/** Create a rebased TAI; feed in:
 *
 *   - A human zone.
 *   - The time in that zone
 *   - The time in an equivalent TAI
 *  
 *  You'll get out a timezone that maps that time in TAI 
 *  to that time in the human zone. The offset is made
 *  'as_if_ns' so it doesn't cause offset knockdown during
 *  future translations (i.e. so it acts as you'd expect)
 */
int cdc_rebased_tai(struct cdc_zone_struct **dst,
			 struct cdc_zone_struct *human_zone,
			 const cdc_calendar_t *human_time,
			 const cdc_calendar_t *machine_time);
			 


/** "Raise" a date from the underlying calendar type of a DST 
 *  timezone to a full member of that timezone (so e.g. a Gregorian
 *  TAI to UTC)
 *
 *  If the source requires several raises to get to the target type
 *  of zone, we'll do that too.
 */
int cdc_zone_raise(cdc_zone_t *zone,
			cdc_calendar_t *dest,
			const cdc_calendar_t *src);

/** "Lower" a date from a timezone to its immediate underlying
 *  calendar type.
 *
 *  Note that raise will raise to the current zone. Lower will lower
 *   one zone.
 */
int cdc_zone_lower(cdc_zone_t *zone,
			cdc_calendar_t *dest,
			cdc_zone_t **lzone,
			const cdc_calendar_t *src);

/** Lower a date to a given system, or if the system is -1, down to 
 *  the lowest zone we can.
 */
int cdc_zone_lower_to(cdc_zone_t *zone,
			   cdc_calendar_t *dest,
			   cdc_zone_t **lzone,
			   const cdc_calendar_t *src,
			   int to_system);


/** Retrieve a timezone for a given zone code 
 *
 * @return 0 on success, < 0 on failure, 0 with 
 *          result == NULL for 'couldn't find it' 
 */
int cdc_zone_new(int system, 
		      cdc_zone_t **out_zone,
		      int i,
		      void *n);

/** Dispose of a zone */
int cdc_zone_dispose(cdc_zone_t **io_zone);

/** Easy creation functions for common time zones */

/** Create a new UTC zone: dispose() is overridden to dispose of
 *  any sub-zones
 */
int cdc_utc_new(cdc_zone_t **ozone);
int cdc_tai_new(cdc_zone_t **ozone);

int cdc_utcplus_new(cdc_zone_t **ozone, int offset);
int cdc_bst_new(cdc_zone_t **ozone);

/** The rebased zone DOES NOT take ownership of the zone it's based on */
int cdc_rebased_new(cdc_zone_t **ozone, 
			 const cdc_calendar_t *offset,
			 cdc_zone_t *based_on);

#endif

/* End File */

