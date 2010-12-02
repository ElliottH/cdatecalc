/* cdcpp.h */
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

#ifndef CDCPP_H_INCLUDED
#define CDCPP_H_INCLUDED

#include <string>
#include <stdint.h>
#include <memory>

namespace cdc
{
    namespace System
    {
        static const int kInvalid = -1;
        static const int kGregorianTAI = 0;
        static const int kUTC = 2;
        static const int kBST = 3;
        static const int kOffset = 4;
        static const int kLowest = 5;

        //! Base is given with a fixed (60*12) offset. 
        static const int kUTCPlusBase = 0x1000;
        static const int kFlagTainted = (1<<30);

        static const int Rebased = (kFlagTainted | 6);

        std::string ToString(const int inSystem);
    };

    namespace Error
    {
        typedef enum 
        {
            NoSuchSystem = -4000,
            SystemsDoNotMatch = -3999,
            NotMySystem = -3998,
            UndefinedDate = -3997,
            InitFailed = -3996,
            BadSystem = -3995,
            InvalidArgument = -3994,
            InternalError = -3993,
            CannotConvert = -3992
        } EnumT;
        
        std::string ToString(const EnumT inEnum);
    }

    namespace Month
    {
        typedef enum
        {
            January = 0,
            February = 1,
            March = 2,
            April = 3,
            May = 4,
            June = 5,
            July = 6,
            August = 7,
            September = 8,
            October = 9,
            November = 10,
            December = 11
        } EnumT;

        std::string ToString(const EnumT inEnum);
    }

    namespace Day
    {
        typedef enum
        {
            Sunday = 0,
            Monday = 1,
            Tuesday = 2,
            Wednesday = 3,
            Thursday = 4,
            Friday = 5,
            Saturday = 6
        } EnumT;

        std::string ToString(const EnumT inEnum);
    };

    class IntervalT
    {
    public:
        int64_t mS;
        long int mNs;

        IntervalT();
        IntervalT(const std::string &inString);

        std::string ToString() const;

        /** Return -1 if this interval is -ve, 0 if it's 0, +1 if +ve
         */
        int Sgn() const;

        /** @return < 0 if a<b, 0 if a ==b , > 0 if a > b */
        static int Compare(const IntervalT& a, const IntervalT& b);
    };

    class CalendarTimeT
    {
    public:
        /** Any offset added to this calendar time will not suppress the
         *  carry-forward of DST offsets below that number (i.e.
         *  this time is treated as an offset in ns).
         */
        static const int kFlagsAsIfNS = (1<<0);

        int mYear;
        int mMonth;
        int mMDay;
        int mHour;
        int mMinute;
        int mSecond;
        long int mNs;
        uint32_t mSystem;
        uint32_t mFlags;

        CalendarTimeT(int yy = 0, int mm = 0, int dd = 0,
                      int hh = 0, int MM = 0, int ss = 0, 
                      long int ns = 0,
                      uint32_t inSystem = 0,
                      uint32_t inFlags = 0) : 
        mYear(yy), mMonth(mm), mMDay(dd), 
            mHour(hh), mMinute(MM), mSecond(ss),
            mNs(ns), mSystem(inSystem), mFlags(inFlags) { };

        void SetTime(int h, int m, int s, long int ns = 0)
        {
            mHour = h; mMinute =m; mSecond = s; 
            mNs = ns;
        }

        CalendarTimeT(const std::string& inString);
        
        std::string ToString() const;


    };

    class CalendarAuxT
    {
    public:
        //! Day of the week.
        Day::EnumT mDay;

        //! Day of the year (0.. 365)
        int mYday;
        
        //! Are we in some form of DST ?
        bool mDST;
        
        CalendarAuxT();
    };

    class ZoneHandleT 
    {
    public:
        ~ZoneHandleT();
        ZoneHandleT(void *zonePtr, const bool owned = true);

        void *mHandle;
        bool mOwned;

        /** Create a UTC zone */
        static std::auto_ptr<ZoneHandleT> UTC();

        /** Create a TAI zone */
        static std::auto_ptr<ZoneHandleT> TAI();

        /** Create a UTC -plus zone, offset in minutes. */
        static std::auto_ptr<ZoneHandleT> UTCPlus(const int offset);

        /** Create a new BST zone */
        static std::auto_ptr<ZoneHandleT> BST();

        /** Note that the rebased zone DOES NOT take ownership of the zone it's
         *  based on - you must delete both!
         */
        static std::auto_ptr<ZoneHandleT> Rebased(ZoneHandleT *basedOn,
                                                  const CalendarTimeT& offset);


        /** Create a rebased TAI. Give it:
         *
         *   - A human time zone.
         *   - A human time.
         *   - A machine time
         *
         * And you will get back a timezone that maps that time in
         *  TAI to that time in the human zone.
         *
         * If the machine zone is a BST-style zone, we'll take account
         *  of this.
         */
        static std::auto_ptr<ZoneHandleT> CreateRebasedTAI(ZoneHandleT *humanZone,
                                                    const CalendarTimeT& inHumanTime,
                                                    const CalendarTimeT& inMachineTime);

        

        int GetSystem(void) const;

    private:
       // Intentionally unimplemented: this object may not be copied.
        ZoneHandleT(const ZoneHandleT &other);
        ZoneHandleT& operator=(const ZoneHandleT& other);


    };

    class ErrorExceptionT
    {
    public:
        Error::EnumT mErrorCode;

    ErrorExceptionT(int inErrorCode) : mErrorCode((Error::EnumT)inErrorCode) { };

        std::string ToString() const;

        Error::EnumT GetErrorCode() { return mErrorCode; }
    };
    
    namespace Operation
    {
        typedef enum
        {
            /** Add two times */
            SimpleAdd = 0,
            /** a-b */
            SimpleSubtract = 1,
            /** Complex addition - see cdc.h for details. This is normally what you want. */
            ComplexAdd = 2,
            /** Zone addition - addition with no consideration of time zone offsets at all;
             *  mainly for internal use 
             */
            ZoneAdd = 3
        } EnumT;
        
        std::string ToString(const EnumT inEnum);
    };



    /** Operate a zone */
    CalendarTimeT Op(ZoneHandleT* inZone, 
                 const CalendarTimeT& src,
                 const CalendarTimeT& offset,
                 const Operation::EnumT inOp);


    /** Bounce a time from one zone to another */
    CalendarTimeT Bounce(ZoneHandleT *source_zone,
                     ZoneHandleT *dst_zone,
                     const CalendarTimeT& src);
                     
    /** Raise a date from the underlying calendar type of a
     *  DST timezone to a full member of that timezone (so eg.
     *  Gregorian TAI to UTC).
     *
     * If the source requires several raises to get to the target
     *  zone type, we'll do that too.
     */
    CalendarTimeT Raise(ZoneHandleT *targetZone,
                    const CalendarTimeT& inSrc);

    /** Lower a date from a timezone to its immediate underlying
     *  calendar type.
     *
     *  Note that whilst raise will potentially raise many zones,
     *  lower lowers only one.
     *
     * @param[out] targetZone The zone into which you have landed: 
     *                          BE CAREFUL! You do not own this pointer.
     *                        It will go away when the zone that created
     *                          this calendar time goes away, and you must
     *                          not delete it. The ZoneHandle you should
     *                          delete - it knows it doesn't own the underlying
     *                          object.
     */
    CalendarTimeT Lower(ZoneHandleT **targetZone,
                    ZoneHandleT *srcZone,
                    const CalendarTimeT& inSrc);


    /** Lower to a given system, or if the system is -1, down to the
     *  lowest zone we can 
     *
     * @param[out] targetZone The zone into which you have landed: 
     *                          BE CAREFUL! You do not own this pointer.
     *                        It will go away when the zone that created
     *                          this calendar time goes away, and you must
     *                          not delete it. The ZoneHandle you should
     *                          delete - it knows it doesn't own the underlying
     *                          object.
     */
    CalendarTimeT LowerTo(ZoneHandleT **targetZone,
                          ZoneHandleT *srcZone,
                          const CalendarTimeT& inSrc,
                          int to_system);


    /** How many ns have elapsed between before and after? 
     */
    void Diff(ZoneHandleT *zone,
              IntervalT& outInterval,
              const CalendarTimeT& inBefore,
              const CalendarTimeT& inAfter);

    /** Parse a system indicator */
    unsigned int SystemFromString(const std::string& inSystem);
    
    /** Pring a system indicator */
    std::string PrintSystem(const int inSystem);
              
              
}

bool operator>(const cdc::IntervalT& a, const cdc::IntervalT& b);
bool operator<(const cdc::IntervalT& a, const cdc::IntervalT& b);
bool operator==(const cdc::IntervalT& a, const cdc::IntervalT& b);
static inline bool operator!=(const cdc::IntervalT& a, const cdc::IntervalT& b) { return !(a==b); }

std::ostream& operator<<(std::ostream& os, const cdc::IntervalT& ival);
std::ostream& operator<<(std::ostream& os, const cdc::CalendarTimeT& cal);

std::ostream& operator<<(std::ostream& os, const cdc::Error::EnumT inError);
std::ostream& operator<<(std::ostream& os, const cdc::Month::EnumT inMonth);
std::ostream& operator<<(std::ostream& os, const cdc::Day::EnumT inDay);
std::ostream& operator<<(std::ostream& os, const cdc::Operation::EnumT inOp);

std::ostream& operator<<(std::ostream& os, const cdc::ErrorExceptionT inEE);


#endif

/* End file */

