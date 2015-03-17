cdatecalc is a C99 library for manipulating dates and times. At present it supports date calculations in TAI, UTC, UTC with arbitrary offsets and BST (British Summer Time).

Operations supported include translation between time zones, discovering the number of seconds between two calendar times and adding or subtracting discrete units (seconds, months, days, ..) from a calendar time to form another calendar time.

cdatecalc resides in a single (rather large) compilation unit for easy linking with your own programs and comes with a set of unit tests you can use to check that it more or less does what it's supposed to.

Please contribute and make datecalc better! In particular, we don't currently support the Gregorian/Julian transition.

datecalc was developed with funding from the UK Metropolitan Police, who also hold the copyright in the initial version. It is licenced under the MPL 1.1.

Please ignore any previous licences in source control; they are an artefact of an import from the original git repository.