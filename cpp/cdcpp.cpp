/* cdcpp.cpp */
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

#include "cdc/cdcpp.h"
#include "cdc/cdc.h"
#include <sstream>
#include <string>

#define ONE_MILLION 1000000
#define ONE_BILLION (1000 * ONE_MILLION)

using namespace cdc;

namespace
{
    struct cdc_zone_struct *Unwrap(ZoneHandleT* zh)
    {
        return (struct cdc_zone_struct *)(zh->mHandle);
    }

    const struct cdc_zone_struct *Unwrap(const ZoneHandleT* zh)
    {
        return (const struct cdc_zone_struct *)(zh->mHandle);
    }
    
    ZoneHandleT *Wrap(struct cdc_zone_struct *c, const bool owned = true)
    {
        ZoneHandleT *zh = new ZoneHandleT((void *)c, owned);
        return zh;
    }

    void intervalToCDC(cdc_interval_t &out, const cdc::IntervalT& in)
    {
        out.s = in.mS; out.ns = in.mNs;
    }
    
    void intervalFromCDC(cdc::IntervalT& out, const cdc_interval_t &in)
    {
        out.mS = in.s; out.mNs = in.ns;
    }

    void calendarToCDC(cdc_calendar_t& out, const cdc::CalendarTimeT& in)
    {
        out.year = in.mYear;
        out.month = in.mMonth;
        out.mday = in.mMDay;
        out.hour = in.mHour;
        out.minute = in.mMinute;
        out.second = in.mSecond;
        out.ns = in.mNs;
        out.system = in.mSystem;
        out.flags = in.mFlags;
    }

    void calendarFromCDC(cdc::CalendarTimeT& out, const cdc_calendar_t &in)
    {
        out.mYear = in.year;
        out.mMonth = in.month;
        out.mMDay = in.mday;
        out.mHour = in.hour;
        out.mMinute = in.minute;
        out.mSecond = in.second;
        out.mNs = in.ns;
        out.mSystem = in.system;
        out.mFlags = in.flags;
    }
}

namespace cdc
{
    namespace System
    {
        std::string ToString(const uint32_t inSystem)
        {
            return std::string(cdc_describe_system(inSystem));
        }
    }

    namespace Error
    {
        std::string ToString(const EnumT inError)
        {
            switch (inError)
            {
            case NoSuchSystem: return "No such system";
            case SystemsDoNotMatch: return "Systems do not match";
            case NotMySystem: return "Not my system";
            case UndefinedDate: return "Undefined date";
            case InitFailed: return "Initialisation failed";
            case BadSystem: return "Bad system";
            case InvalidArgument: return "Invalid argument";
            case InternalError: return "Internal Error";
            case CannotConvert: return "Cannot Convert";
            default:
            {
                std::ostringstream ss;
                ss << "Unknown CDC error " << (int)inError;
                return ss.str();
            }
            }
        }
    }

    namespace Month
    {
        std::string ToString(const EnumT inEnum)
        {
            switch (inEnum)
            {
            case January: return "January";
            case February: return "February";
            case March: return "March";
            case April: return "April";
            case May: return "May";
            case June: return "June";
            case July: return "July";
            case August: return "August";
            case September: return "September";
            case October: return "October";
            case November: return "November";
            case December: return "December";
            default: return "Unknown";
            }
        }
    }

    namespace Day
    {
        std::string ToString(const EnumT inEnum)
        {
            switch (inEnum)
            {
            case Sunday: return "Sunday";
            case Monday: return "Monday";
            case Tuesday: return "Tuesday";
            case Wednesday: return "Wednesday";
            case Thursday: return "Thursday";
            case Friday: return "Friday";
            case Saturday: return "Saturday";
            default: return "Unknown";
            }
        }
    }

    IntervalT::IntervalT() : mS(0), mNs(0) { }
    IntervalT::IntervalT(const std::string& inString) 
    {
        cdc_interval_t x;
        int rv;
        rv = cdc_interval_parse(&x, inString.c_str(), inString.length());
        if (rv) { throw ErrorExceptionT(rv); }
        mS = x.s; mNs = x.ns;
    }

    IntervalT IntervalT::FromMilliseconds(int64_t inMs)
    {
        return IntervalT(inMs / 1000, 
                         (long int)((inMs % 1000) * ONE_MILLION));
    }

    std::string IntervalT::ToString() const
    {
        cdc_interval_t x;
        intervalToCDC(x, *this);
        char buf[128];
        int rv;
        rv = cdc_interval_sprintf(buf, 128, &x);
        if (rv < 0 ) { throw ErrorExceptionT(rv); }
        return std::string(buf);
    }

    int IntervalT::Compare(const IntervalT& a, const IntervalT& b)
    {
        cdc_interval_t x, y;
        intervalToCDC(x, a); intervalToCDC(y,b);
        return cdc_interval_cmp(&x,&y);
    }

    IntervalT& IntervalT::operator+=(const IntervalT& other) 
    {
        this->mS += other.mS;
        this->mNs += other.mNs;

        int64_t result = (this->mNs / ONE_BILLION);
        this->mS += result;
        this->mNs -= (ONE_BILLION * result);
        return *this;
    }

    const IntervalT IntervalT::operator+(const IntervalT& other) const
    {
        IntervalT result(*this);
        result += other;
        return result;
    }


    IntervalT& IntervalT::operator-=(const IntervalT& other)
    {
        this->mS -= other.mS;
        this->mNs -= other.mNs;

        int64_t result = (this->mNs / ONE_BILLION);
        this->mS += result;
        this->mNs -= (ONE_BILLION * result);
        return *this;
    }

    const IntervalT IntervalT::operator-(const IntervalT& other) const
    {
        IntervalT result(*this);
        result -= other;
        return result;
    }

    int IntervalT::Sgn() const
    {
        cdc_interval_t x;
        intervalToCDC(x, *this);
        return cdc_interval_sgn(&x);
    }

    CalendarTimeT::CalendarTimeT(const std::string& inString)
    {
        cdc_calendar_t x;
        int rv;
        rv = cdc_calendar_parse(&x, inString.c_str(), inString.length());
        if (rv) { throw ErrorExceptionT(rv); }
        calendarFromCDC(*this, x);
    }

    CalendarTimeT::CalendarTimeT(const IntervalT& inInterval, int inSystem) : 
        mYear(0), mMonth(0), mMDay(0), mHour(0), mMinute(0), 
        mSecond(0), mNs(0), mSystem(inSystem), mFlags(kFlagsAsIfNS)
    {
        int64_t remainS(inInterval.mS);

        mNs = inInterval.mNs;
        mMDay = (remainS / 86400);
        remainS -= (mMDay * 86400);

        mHour = (remainS / 3600);
        remainS -= (mHour * 3600);
        mMinute = (remainS / 60);
        remainS -= (mMinute * 60);
        mSecond = remainS;
    }


    std::string CalendarTimeT::ToString() const
    {
        char buf[128];
        cdc_calendar_t c;
        int rv;

        calendarToCDC(c, *this);
        rv = cdc_calendar_sprintf(buf, 128, &c);
        if (rv < 0 ) { throw ErrorExceptionT(rv); }
        return std::string(buf);
    }

    int CalendarTimeT::Compare(const CalendarTimeT& a, const CalendarTimeT& b)
    {
        if (a.mYear < b.mYear) { return -1; }
        if (a.mYear > b.mYear) { return 1; }
        
        if (a.mMonth < b.mMonth) { return -1; }
        if (a.mMonth > b.mMonth) { return 1; }

        if (a.mMDay < b.mMDay) { return  -1; }
        if (a.mMDay > b.mMDay) { return 1; }

        if (a.mHour < b.mHour) { return -1; }
        if (a.mHour > b.mHour) { return 1; }

        if (a.mMinute < b.mMinute) { return -1; }
        if (a.mMinute > b.mMinute) { return 1; }

        if (a.mSecond < b.mSecond) { return -1; }
        if (a.mSecond > b.mSecond) { return 1; }
        
        if (a.mNs < b.mNs) { return -1; }
        if (a.mNs > b.mNs) { return 1; }

        return 0;
    }

    CalendarAuxT::CalendarAuxT() : 
        mDay(Day::Sunday), mYday(0), mDST() { }


    ZoneHandleT::ZoneHandleT(void *inZonePtr, const bool owned)
    {
        mHandle = inZonePtr;
        mOwned = owned;
    }

    ZoneHandleT::~ZoneHandleT()
    {
        if (mOwned)
        {
            cdc_zone_t *zh = (cdc_zone_t *)mHandle;
            cdc_zone_dispose(&zh);
        }
        mHandle = NULL;
        mOwned = false;
    }

    uint32_t ZoneHandleT::GetSystem(void) const
    {
        const cdc_zone_t *zh = Unwrap(this);
        return zh->system;
    }

    std::auto_ptr<ZoneHandleT> ZoneHandleT::UTC()
    {
        cdc_zone_t *h(NULL);
        int rv;
        rv = cdc_utc_new(&h);
        if (rv) { throw ErrorExceptionT(rv); }
        return std::auto_ptr<ZoneHandleT>(Wrap(h));
    }

    std::auto_ptr<ZoneHandleT> ZoneHandleT::TAI()
    {
        cdc_zone_t *h(NULL);
        int rv;
        rv = cdc_tai_new(&h);
        if (rv) { throw ErrorExceptionT(rv); }
        return std::auto_ptr<ZoneHandleT>(Wrap(h));
    }

    std::auto_ptr<ZoneHandleT> ZoneHandleT::UTCPlus(const int offset)
    {
        cdc_zone_t *h(NULL);
        int rv;
        rv = cdc_utcplus_new(&h, offset);
        if (rv) { throw ErrorExceptionT(rv); }
        return std::auto_ptr<ZoneHandleT>(Wrap(h));
    }

    std::auto_ptr<ZoneHandleT> ZoneHandleT::UKCT()
    {
        cdc_zone_t *h(NULL);
        int rv;
        rv = cdc_ukct_new(&h);
        if (rv) { throw ErrorExceptionT(rv); }
        return std::auto_ptr<ZoneHandleT>(Wrap(h));
    }

    std::auto_ptr<ZoneHandleT> ZoneHandleT::Rebased(ZoneHandleT *basedOn,
                                                    const CalendarTimeT& offset)
    {
        cdc_zone_t *h(NULL);
        int rv;
        cdc_calendar_t x;
        
        calendarToCDC(x, offset);

        rv = cdc_rebased_new(&h,&x, Unwrap(basedOn));
        if (rv) { throw ErrorExceptionT(rv); }
        return std::auto_ptr<ZoneHandleT>(Wrap(h));
    }

    std::auto_ptr<ZoneHandleT> ZoneHandleT::CreateRebasedTAI(ZoneHandleT *humanZone,
                                                            const CalendarTimeT& inHumanTime,
                                                            const CalendarTimeT& inMachineTime)
    {
        struct cdc_zone_struct *h(NULL);
        cdc_calendar_t x,y;
        int rv;

        calendarToCDC(x, inHumanTime); calendarToCDC(y, inMachineTime);
        rv = cdc_rebased_tai(&h, Unwrap(humanZone), &x, &y);
        if (rv) { throw ErrorExceptionT(rv); }
        
        ZoneHandleT *zh(Wrap(h));
        return std::auto_ptr<ZoneHandleT>(zh);
    }

    // WARNING: This code has a semi-clone in the form of
    // cdc_zone_from_system() in cdc.
    // If you change this, you must make a matching change in the other.
    std::auto_ptr<ZoneHandleT> ZoneHandleT::FromSystem(uint32_t inSystem)
    {

        if (inSystem > System::kUTCPlusBase)
        {
            int offset = (inSystem - System::kUTCPlusBase) - (12*60);
            if (offset < -720 || offset > 1440)
                throw ErrorExceptionT(Error::BadSystem);
            return UTCPlus(offset);
        }

        switch (inSystem)
        {
        case System::kGregorianTAI: return TAI(); 
        case System::kUTC: return UTC();
        case System::kUKCT: return UKCT();
        default:
            throw ErrorExceptionT(Error::BadSystem);
        }
    }

    std::string ErrorExceptionT::ToString() const
    {
        std::ostringstream ss;
        ss << Error::ToString(mErrorCode) <<  " (" << ((int)mErrorCode) << ")";
        return ss.str();
    }

    namespace Operation
    {
        std::string ToString(const EnumT inEnum)
        {
            switch (inEnum)
            {
            case SimpleAdd: return "SimpleAdd";
            case SimpleSubtract: return "SimpleSubtract";
            case ComplexAdd: return "ComplexAdd";
            case ZoneAdd: return "ZoneAdd";
            default:
                return "Unknown";
            }
        }
    }

    CalendarTimeT Op(ZoneHandleT *zoneHandle,
                 const CalendarTimeT& src,
                 const CalendarTimeT& offset,
                 const Operation::EnumT inOp)
    {
        int rv;
        cdc_calendar_t x,y,z;
        CalendarTimeT ret;

        calendarToCDC(x, src); calendarToCDC(y, offset);
        rv = cdc_op(Unwrap(zoneHandle), &z, &x, &y, (int)inOp);
        if (rv)
        {
            throw ErrorExceptionT(rv);
        }
        calendarFromCDC(ret, z);
        return ret;
    }

    CalendarTimeT Bounce(ZoneHandleT *sourceZone,
                     ZoneHandleT *dstZone,
                     const CalendarTimeT& src)
    {
        int rv;
        cdc_calendar_t x,y;
        CalendarTimeT ret;

        calendarToCDC(x, src);
        rv = cdc_bounce(Unwrap(sourceZone), Unwrap(dstZone), 
                        &y, &x);
        if (rv) 
        {
            throw ErrorExceptionT(rv);
        }
        calendarFromCDC(ret, y);
        return ret;
    }

    CalendarTimeT Raise(ZoneHandleT *targetZone,
                    const CalendarTimeT& inSrc)
    {
        int rv;
        cdc_calendar_t x,y;
        CalendarTimeT ret;

        calendarToCDC(x, inSrc);
        rv = cdc_zone_raise(Unwrap(targetZone), &y, &x);
        if (rv) { throw ErrorExceptionT(rv); }
        calendarFromCDC(ret, y);
        return ret;
    }

    CalendarTimeT Lower(ZoneHandleT **targetZone,
                    ZoneHandleT *srcZone,
                    const CalendarTimeT& inSrc)
    {
        int rv;
        cdc_calendar_t x,y;
        CalendarTimeT ret;
        cdc_zone_t *h(NULL);
        
        calendarToCDC(x, inSrc);
        rv = cdc_zone_lower(Unwrap(srcZone), &y, &h, &x);
        if (rv) { throw ErrorExceptionT(rv); }
        (*targetZone) = Wrap(h, false);
        calendarFromCDC(ret, y);
        return ret;
    }

    CalendarTimeT LowerTo(ZoneHandleT **targetZone,
                      ZoneHandleT *srcZone,
                      const CalendarTimeT& inSrc,
                      int to_system)
    {
        int rv;
        cdc_calendar_t x,y;
        CalendarTimeT ret;
        cdc_zone_t *h(NULL);

        calendarToCDC(x, inSrc);
        rv = cdc_zone_lower_to(Unwrap(srcZone), &y, &h, &x, to_system);
        if (rv) { throw ErrorExceptionT(rv); }
        (*targetZone) = Wrap(h, false);
        calendarFromCDC(ret, y);
        return ret;
    }

    void Diff(ZoneHandleT *zone,
              IntervalT& outInterval,
              const CalendarTimeT& inBefore,
              const CalendarTimeT& inAfter)
    {
        cdc_calendar_t b, a;
        cdc_interval_t result;
        int rv;

        calendarToCDC(b, inBefore);
        calendarToCDC(a, inAfter);
        rv = cdc_diff(Unwrap(zone), &result, &b, &a);
        if (rv) { throw ErrorExceptionT(rv); }
        intervalFromCDC(outInterval, result);
    }

    unsigned int SystemFromString(const std::string& inSystem)
    {
        int rv;
        unsigned int outsys;
        rv = cdc_undescribe_system(&outsys, inSystem.c_str());
        if (rv < 0) { throw ErrorExceptionT(rv); }
        return outsys;
    }
    
    std::string SystemToString(const unsigned int inSys)
    {
        const char *b(cdc_describe_system(inSys));
        return std::string(b);
    }

}

bool operator>(const cdc::IntervalT& a, const cdc::IntervalT& b) 
{
    return cdc::IntervalT::Compare(a,b) > 0;
}

bool operator==(const cdc::IntervalT& a, const cdc::IntervalT& b)
{
    return cdc::IntervalT::Compare(a,b) == 0;
}


bool operator<(const cdc::IntervalT& a, const cdc::IntervalT& b)
{
    return cdc::IntervalT::Compare(a,b) < 0;
}

bool operator>(const cdc::CalendarTimeT& a, const cdc::CalendarTimeT& b)
{
    return cdc::CalendarTimeT::Compare(a,b) > 0;
}

bool operator==(const cdc::CalendarTimeT& a, const cdc::CalendarTimeT& b)
{
    return cdc::CalendarTimeT::Compare(a,b) == 0;
}

bool operator<(const cdc::CalendarTimeT& a, const cdc::CalendarTimeT& b)
{
    return cdc::CalendarTimeT::Compare(a,b) < 0;
}

std::ostream& operator<<(std::ostream& os, const cdc::IntervalT& ival)
{
    char buf[64];
    cdc_interval_t x;
    intervalToCDC(x, ival);
    cdc_interval_sprintf(buf, 64, &x);
    os << std::string(buf);
    return os;
}

std::ostream& operator<<(std::ostream& os, const cdc::CalendarTimeT& cal)
{
    char buf[128];
    cdc_calendar_t c;
    calendarToCDC(c, cal);
    cdc_calendar_sprintf(buf, 128, &c);
    os << std::string(buf);
    return os;
}

std::ostream& operator<<(std::ostream& os, const cdc::Error::EnumT inError)
{
    os << cdc::Error::ToString(inError);
    return os;
}

std::ostream& operator<<(std::ostream& os, const cdc::Month::EnumT inMonth)
{
    os << cdc::Month::ToString(inMonth);
    return os;
}

std::ostream& operator<<(std::ostream& os, const cdc::Day::EnumT inDay)
{
    os << cdc::Day::ToString(inDay);
    return os;
}

std::ostream& operator<<(std::ostream& os, const cdc::Operation::EnumT inOp)
{
    os << cdc::Operation::ToString(inOp);
    return os;
}

std::ostream& operator<<(std::ostream& os, const cdc::ErrorExceptionT inEE)
{
    os  << inEE.ToString();
    return os;
}

  



/* End file */
