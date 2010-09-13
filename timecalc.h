/* timecalc.h */
/* (C) Kynesim Ltd 2010 */

/*
 *  This file is hereby placed in the public domain; do what you like
 *  with it. But please don't introduce yet more spurious time related
 *  bugs into the world!
 *
 */

#ifndef TIMECALC_H_INCLUDED
#define TIMECALC_H_INCLUDED

/** @file
 *
 * This is timecalc ; a very simple C library of time calculation functions
 *
 * timecalc acts as a replacement for the UNIX mktime()/gmtime()/etc functions
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
#define TIMECALC_SYSTEM_INVALID          -1

/** TAI, Gregorian calendar */
#define TIMECALC_SYSTEM_GREGORIAN_TAI     0 


//! You've tried to perform an operation on an unknown system.
#define TIMECALC_ERR_NO_SUCH_SYSTEM (-4000)

//! You've tried to operate on dates whose systems don't match.
#define TIMECALC_ERR_SYSTEMS_DO_NOT_MATCH (-3999)

//! Attempt to perform an operation on system A with a date of
//! system B
#define TIMECALC_ERR_NOT_MY_SYSTEM        (-3998)


/** Represents an interval.
 *
 *  2^63s is about 5e10 years. Note that over such long timescales the
 * leap second model will fail in quite nasty ways. Though since the
 * sun's remaining lifespan is only about 2-4e9 years you hopefully
 * don't care.
 */
typedef struct timecalc_interval_struct
{
  /** Elapsed seconds */
  int64_t s;

  /** Elapsed nanoseconds */
  long int ns;
  
} timecalc_interval_t;

/** Represents a calendar time (wall clock or zone corrected) */
typedef struct timecalc_calendar_struct
{
  /** Year */
  int year;

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

} timecalc_calendar_t;

/** Other things you might like to know when converting to a calendar
 *  time but which aren't strictly part of the time itself.
 */
typedef struct timecalc_calendar_aux_struct
{
  /** Day of the week (monday = 0) */
  int wday;

  /** Day of the year (0-365) */
  int yday;

  /** Are we in some form of DST? */
  int is_dst;

} timecalc_calendar_aux_t;

/** Represents a time zone */
typedef struct timecalc_zone_struct
{
  //! 'user' handle.
  void *handle;

  /** Initialize this zone structure; mainly used internally */
  void (*init)(struct timecalc_zone_struct *self, void *arg);

  /** Kill this zone structure (but don't free() it - just any 
   *   internal data) - mainly used by timecalc_zone_dispose() 
   */
  void (*dispose)(struct timecalc_zone_struct *self);

  /** How much time has elapsed between 'before' and 'after' ? 
   *
   * @return 0 on success , < 0 on failure.
   */
  int (*diff)(struct timecalc_zone_struct *self,
	      timecalc_interval_t *ival,
	      const timecalc_calendar_t *before,
	      const timecalc_calendar_t *after);

  /** Normalise the given calendar time (if you can)
   */
  int (*normalise)(struct timecalc_zone_struct *self,
		   timecalc_calendar_t *io_cal);

  /** Compute auxilliary info for a calendar date */
  int (*aux)(struct timecalc_zone_struct *self,
	     const timecalc_calendar_t *cal,
	     timecalc_calendar_aux_t *aux);

  /** Retrieve the epoch for this system; this isn't the minimum
   *  (or maximum) value representable, but it does provide some 
   *  sort of anchor to base the system on.
   */
  int (*epoch)(struct timecalc_zone_struct *self,
	       timecalc_calendar_t *epoch);

} timecalc_zone_t;

/** Add two intervals */
int timecalc_interval_add(timecalc_interval_t *result,
			  const timecalc_interval_t *a,
			  const timecalc_interval_t *b);

/** Compute a-b */
int timecalc_interval_subtract(timecalc_interval_t *result,
			       const timecalc_interval_t *a,
			       const timecalc_interval_t *b);

/** Compare two dates numerically (note that this says, in general,
 *  nothing about which came first). 
 *
 * @return -1 if a < b, 0 if a==b, 1 if a > b 
 */
int timecalc_calendar_cmp(const timecalc_calendar_t *a, 
			  const timecalc_calendar_t *b);

/** Compare two intervals
 *
 * @return -1 if a < b, 0 if a == b, 1 if a > b 
 */
int timecalc_interval_cmp(const timecalc_interval_t *a,
			  const timecalc_interval_t *b);

/** Print an interval in %lld.%lld s format */
int timecalc_interval_sprintf(char *buf, 
			      int n,
			      const timecalc_interval_t *a);

/** Print a calendar time in ISO format */
int timecalc_calendar_sprintf(char *buf,
			      int n,
			      const timecalc_calendar_t *date);

/** Return the sign of an interval */
int timecalc_interval_sgn(const timecalc_interval_t *a);			      

/** Describe this system as a string, returned in a static buffer */
const char *timecalc_describe_system(const int system);


/** Add an interval to a calendar time and normalise. This is essentially
 *  a repeated add.
 */
int timecalc_zone_add(timecalc_zone_t *zone,
		      timecalc_calendar_t *out,
		      const timecalc_calendar_t *date,
		      const timecalc_interval_t *ival);

/** Retrieve a timezone for a given zone code 
 *
 * @return 0 on success, < 0 on failure, 0 with 
 *          result == NULL for 'couldn't find it' 
 */
int timecalc_zone_new(int system, 
		      timecalc_zone_t **out_zone,
		      void *arg);


/** Dispose of a zone */
int timecalc_zone_dispose(timecalc_zone_t **io_zone);

#endif

/* End File */

