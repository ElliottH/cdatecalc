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

/** UTC */
#define TIMECALC_SYSTEM_UTC               2

/** BST */
#define TIMECALC_SYSTEM_BST               3

/** UTC plus an offset of 0 (-1200) to 1440 (+1200) */
#define TIMECALC_SYSTEM_UTCPLUS_BASE           0x1000
#define TIMECALC_SYSTEM_UTCPLUS_ZERO          (TIMECALC_SYSTEM_UTCPLUS_BASE + (60*12))

/** An offset */
#define TIMECALC_SYSTEM_OFFSET            4



//! You've tried to perform an operation on an unknown system.
#define TIMECALC_ERR_NO_SUCH_SYSTEM (-4000)

//! You've tried to operate on dates whose systems don't match.
#define TIMECALC_ERR_SYSTEMS_DO_NOT_MATCH (-3999)

//! Attempt to perform an operation on system A with a date of
//! system B
#define TIMECALC_ERR_NOT_MY_SYSTEM        (-3998)

//! Attempt to perform an operation on a date for which the given
//! timezone is not (well) defined.
#define TIMECALC_ERR_UNDEFINED_DATE       (-3997)

//! Initialisation or disposal failed.
#define TIMECALC_ERR_INIT_FAILED          (-3996)

//! Bad time system
#define TIMECALC_ERR_BAD_SYSTEM           (-3995)

//! Invalid argument
#define TIMECALC_ERR_INVALID_ARGUMENT     (-3994)

//! Internal error
#define TIMECALC_ERR_INTERNAL_ERROR       (-3993)


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


#define TIMECALC_JANUARY    0
#define TIMECALC_FEBRUARY   1
#define TIMECALC_MARCH      2
#define TIMECALC_APRIL      3
#define TIMECALC_MAY        4
#define TIMECALC_JUNE       5
#define TIMECALC_JULY       6
#define TIMECALC_AUGUST     7 
#define TIMECALC_SEPTEMBER  8
#define TIMECALC_OCTOBER    9
#define TIMECALC_NOVEMBER  10
#define TIMECALC_DECEMBER  11 


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
  /** Day of the week (Sunday = 0, Saturday = 6) */
  int wday;

#define TIMECALC_SUNDAY 0
#define TIMECALC_MONDAY 1
#define TIMECALC_TUESDAY 2
#define TIMECALC_WEDNESDAY 3
#define TIMECALC_THURSDAY 4
#define TIMECALC_FRIDAY 5
#define TIMECALC_SATURDAY 6

  /** Day of the year (0-365) */
  int yday;

  /** Are we in some form of DST? */
  int is_dst;

} timecalc_calendar_aux_t;

/** Represents a time zone 
 *
 *  Each time zone has a calendar and an offset.
 *
 *  The calendar is the underlying monotonic date system to which this time zone
 *  applies. It is required to be at least piecewise continuous and attempts to
 *  perform calculations with discontinuous portions of the calendar (e.g. the 
 *  'lost days' between Julian and Gregorian) are likely to return 
 *  TIMECALC_ERR_UNDEFINED_DATE.
 *
 *  The offset is the (typically discontinuous) offset from that monotonic calendar
 *  induced by the conversion to sidereal time,  your current longitude, DST, etc.
 *
 *  The distinction is that offsets to your current date and time are applied to
 *  the calendar and then corrected by the offset.
 */
typedef struct timecalc_zone_struct
{
  //! 'user' handle.
  void *handle;
  
  //! The system this zone handles.
  int system;

  /** Initialize this zone structure; mainly used internally */
  int (*init)(struct timecalc_zone_struct *self, int arg_i, void *arg_n);

  /** Kill this zone structure (but don't free() it - just any 
   *   internal data) - mainly used by timecalc_zone_dispose() 
   */
  int (*dispose)(struct timecalc_zone_struct *self);


#if 0

  /** How much time has elapsed between 'before' and 'after' ? 
   *
   * @return 0 on success , < 0 on failure.
   */
  int (*diff)(struct timecalc_zone_struct *self,
	      timecalc_interval_t *ival,
	      const timecalc_calendar_t *before,
	      const timecalc_calendar_t *after);
#endif

  /** Obtain the calendar_t required to be added to the underlying
   *  calendar to get it into this time zone.
   *
   *  Note that the quantities in this calendar_t are not normalised;
   *   they are in 'natural' form, because timecalc_add_offset() needs
   *   to know which additional offsets to knock out when running -
   *   see the documentation for that function for details.
   *
   *  Though it makes no difference to the actual offset, (*leap_second)
   *   will be set to 1 when a leap second follows the time in src.
   *
   * cal_offset() should normally be able to take a src of either 
   *  the native system for this zone or that of the lower zone.
   */
  int (*offset)(struct timecalc_zone_struct *self,
		timecalc_calendar_t *offset,
		const timecalc_calendar_t *src);

  /** Fieldwise addition.
   */
  int (*op)(struct timecalc_zone_struct *self,
	    timecalc_calendar_t *dest,
	    const timecalc_calendar_t *src,
	    const timecalc_calendar_t *offset,
	    int op);		    

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

  /** Obtain a pointer to 'the next zone down' in the heirarchy,
   *   if there is one 
   */
  int (*lower_zone)(struct timecalc_zone_struct *self,
		    struct timecalc_zone_struct **next);

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

/** Add two calendar times fieldwise */
int timecalc_simple_op(timecalc_calendar_t *result,
		       const timecalc_calendar_t *a,
			const timecalc_calendar_t *b, 
			int op);
			


/** Add an interval to a calendar time and normalise. This is essentially
 *  a repeated add.
 */
int timecalc_zone_add(timecalc_zone_t *zone,
		      timecalc_calendar_t *out,
		      const timecalc_calendar_t *date,
		      const timecalc_interval_t *ival);


// dst = a + b
#define TIMECALC_OP_SIMPLE_ADD 0

// dst = a - b 
#define TIMECALC_OP_SUBTRACT 1

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
#define TIMECALC_OP_COMPLEX_ADD 2

/* A timezone addition performs addition with no consideration of time zone
 *  offsets at all - it's used when raising and lowering calendar_ts 
 */
#define TIMECALC_OP_ZONE_ADD 3


int timecalc_op(struct timecalc_zone_struct *zone,
		timecalc_calendar_t *dst,
		const timecalc_calendar_t *src,
		const timecalc_calendar_t *offset,
		int op);


/** "Raise" a date from the underlying calendar type of a DST 
 *  timezone to a full member of that timezone (so e.g. a Gregorian
 *  TAI to UTC)
 *
 *  If the source requires several raises to get to the target type
 *  of zone, we'll do that too.
 */
int timecalc_zone_raise(timecalc_zone_t *zone,
			timecalc_calendar_t *dest,
			const timecalc_calendar_t *src);

/** "Lower" a date from a timezone to its immediate underlying
 *  calendar type.
 *
 *  Note that raise will raise to the current zone. Lower will lower
 *   one zone.
 */
int timecalc_zone_lower(timecalc_zone_t *zone,
			timecalc_calendar_t *dest,
			timecalc_zone_t **lzone,
			const timecalc_calendar_t *src);


/** Convert one date to another with the aid of an epoch; the actual
 *  epoch doesn't really matter except that most timezones use some
 *  kind of loop so keeping the epoch close to the dates will 
 *  make your calculation faster.
 */
//int timecalc_zone_convert(timecalc_calendar_t *dest,
//			  timecalc_zone_t *from,
//			  timecalc_zone_t *to,
//			  const timecalc_calendar_t *src,
//			  const timecalc_calendar_t *epoch);
			  

/** Retrieve a timezone for a given zone code 
 *
 * @return 0 on success, < 0 on failure, 0 with 
 *          result == NULL for 'couldn't find it' 
 */
int timecalc_zone_new(int system, 
		      timecalc_zone_t **out_zone,
		      int i,
		      void *n);


/** Dispose of a zone */
int timecalc_zone_dispose(timecalc_zone_t **io_zone);

#endif

/* End File */

