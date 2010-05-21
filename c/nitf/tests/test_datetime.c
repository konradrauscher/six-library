/* =========================================================================
 * This file is part of NITRO
 * =========================================================================
 *
 * (C) Copyright 2004 - 2008, General Dynamics - Advanced Information Systems
 *
 * NITRO is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, If not,
 * see <http://www.gnu.org/licenses/>.
 *
 */

#include <import/nitf.h>
#include "Test.h"

#define MAX_DATE_STRING    1024

NITFPRIV(void) printDate(nitf_DateTime *date)
{
    printf("Year: %d\n", date->year);
    printf("Month: %d\n", date->month);
    printf("Day Of Month: %d\n", date->dayOfMonth);
    printf("Day Of Week: %d\n", date->dayOfWeek);
    printf("Day Of Year: %d\n", date->dayOfYear);
    printf("Hour: %d\n", date->hour);
    printf("Minute: %d\n", date->minute);
    printf("Second: %f\n", date->second);
    printf("Millis: %f\n", date->timeInMillis);
}

TEST_CASE(testNow)
{
    nitf_DateTime *date = NULL;
    nitf_Error e;

    date = nitf_DateTime_now(&e);
    TEST_ASSERT(date);

    nitf_DateTime_destruct(&date);
    TEST_ASSERT_NULL(date);
}

TEST_CASE(testFromMillis)
{
    nitf_DateTime *date = NULL, *date2 = NULL;
    nitf_Error e;

    date = nitf_DateTime_now(&e);
    TEST_ASSERT(date);

    date2 = nitf_DateTime_fromMillis(date->timeInMillis, &e);
    TEST_ASSERT(date2);

    TEST_ASSERT((date->timeInMillis == date2->timeInMillis));

    nitf_DateTime_destruct(&date);
    nitf_DateTime_destruct(&date2);
    TEST_ASSERT_NULL(date);
    TEST_ASSERT_NULL(date2);
}

TEST_CASE(testRoundTrip)
{
    nitf_DateTime *date = NULL, *date2 = NULL;
    char buf[MAX_DATE_STRING], buf2[MAX_DATE_STRING];
    nitf_Error e;

    date = nitf_DateTime_now(&e);
    TEST_ASSERT(date);

    /*printDate(date);*/

    TEST_ASSERT((nitf_DateTime_format(date, NITF_DATE_FORMAT_21, buf,
                            NITF_FDT_SZ + 1, &e)));
    /*printf("Date: %s\n", buf);*/

    date2 = nitf_DateTime_fromString(buf, NITF_DATE_FORMAT_21, &e);
    TEST_ASSERT(date2);

    TEST_ASSERT((nitf_DateTime_format(date2, NITF_DATE_FORMAT_21, buf2,
                            NITF_FDT_SZ + 1, &e)));
    /*printf("Date: %s\n", buf2);*/
    /*printDate(date2);*/

    TEST_ASSERT_EQ_STR(buf, buf2);

    /* must subtract off the millis - since our format string doesn't include them */
    nitf_DateTime_setSecond(date, (int)date->second, &e);
    nitf_DateTime_setSecond(date2, (int)date2->second, &e);
    TEST_ASSERT_EQ_FLOAT(date->timeInMillis, date2->timeInMillis);

    nitf_DateTime_destruct(&date);
    nitf_DateTime_destruct(&date2);
    TEST_ASSERT_NULL(date);
    TEST_ASSERT_NULL(date2);
}

TEST_CASE(testSetIdentity)
{
    nitf_DateTime *date = NULL;
    char buf[MAX_DATE_STRING], buf2[MAX_DATE_STRING];
    nitf_Error e;

    date = nitf_DateTime_now(&e);
    TEST_ASSERT(date);

    TEST_ASSERT((nitf_DateTime_format(date, NITF_DATE_FORMAT_21, buf,
                            NITF_FDT_SZ + 1, &e)));

    /* set hour */
    TEST_ASSERT(nitf_DateTime_setHour(date, date->hour, &e));
    TEST_ASSERT((nitf_DateTime_format(date, NITF_DATE_FORMAT_21, buf2,
                            NITF_FDT_SZ + 1, &e)));
    TEST_ASSERT_EQ_STR(buf, buf2);

    /* set minute */
    TEST_ASSERT(nitf_DateTime_setMinute(date, date->minute, &e));
    TEST_ASSERT((nitf_DateTime_format(date, NITF_DATE_FORMAT_21, buf2,
                            NITF_FDT_SZ + 1, &e)));
    TEST_ASSERT_EQ_STR(buf, buf2);

    /* set second */
    TEST_ASSERT(nitf_DateTime_setSecond(date, date->second, &e));
    TEST_ASSERT((nitf_DateTime_format(date, NITF_DATE_FORMAT_21, buf2,
                            NITF_FDT_SZ + 1, &e)));
    TEST_ASSERT_EQ_STR(buf, buf2);

    /* set month */
    TEST_ASSERT(nitf_DateTime_setMonth(date, date->month, &e));
    TEST_ASSERT((nitf_DateTime_format(date, NITF_DATE_FORMAT_21, buf2,
                            NITF_FDT_SZ + 1, &e)));
    TEST_ASSERT_EQ_STR(buf, buf2);

    /* set month */
    TEST_ASSERT(nitf_DateTime_setYear(date, date->year, &e));
    TEST_ASSERT((nitf_DateTime_format(date, NITF_DATE_FORMAT_21, buf2,
                            NITF_FDT_SZ + 1, &e)));
    TEST_ASSERT_EQ_STR(buf, buf2);

    nitf_DateTime_destruct(&date);
    TEST_ASSERT_NULL(date);
}

TEST_CASE(testMillis)
{
    nitf_Error e;
    char buf[MAX_DATE_STRING];
    const char* timeStr = "2010-01-12T22:55:37.467000Z";
    nitf_DateTime *date = NULL;

    date = nitf_DateTime_fromString(timeStr, "%Y-%m-%dT%H:%M:%S000Z", &e);
    TEST_ASSERT(date);

    nitf_DateTime_format(date, "%Y-%m-%dT%H:%M:%.6SZ", buf, MAX_DATE_STRING, &e);
    TEST_ASSERT_EQ_STR(timeStr, buf);
}

int main(int argc, char **argv)
{
    CHECK(testNow);
    CHECK(testFromMillis);
    CHECK(testRoundTrip);
    CHECK(testSetIdentity);
    CHECK(testMillis);

    return 0;
}
