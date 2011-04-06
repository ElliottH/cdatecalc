/* cdcpptest.cpp */
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
 * A quick test for cdcpp, just to prove it works.
 *
 * @author Richard Watts <rrw@kynesim.co.uk>
 * @date   2010-11-15
 */

#include <stdint.h>
#include "cdc/cdcpp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

int main(int argn, char *args[])
{
    std::auto_ptr<cdc::ZoneHandleT> aZone;

    try
    {
        aZone = cdc::ZoneHandleT::UKCT();
        cdc::CalendarTimeT aCalTime
            (2001,8, 20, 18, 00, 00, 0, 
             cdc::System::kUKCT, 0), 
            anIncTime;
        
        anIncTime.SetTime(1, 0, 0, 0);
        cdc::CalendarTimeT after;
        
        after = cdc::Op(aZone.get(), aCalTime, anIncTime, cdc::Operation::ComplexAdd);

        std::cout << "Time after = " << after << std::endl;
        cdc::CalendarTimeT utc;

        std::auto_ptr<cdc::ZoneHandleT> tgtZone;
        cdc::ZoneHandleT *ptr(NULL);
        utc = cdc::LowerTo(&ptr, aZone.get(), after, cdc::System::kUTC);
        tgtZone.reset(ptr); ptr = NULL;
        std::cout << "In UTC = " << utc << std::endl;
        
        cdc::CalendarTimeT backInUKCT
            (cdc::Raise(aZone.get(), utc));
        std::cout << " Back in UKCT = " << backInUKCT << std::endl;
        
        
        

    }
    catch (cdc::ErrorExceptionT ee)
    {
        std::cout << "Error: " << ee << std::endl;
    }
}

/* End File */
