#include <k2systype.h>

BOOL
K2_IsLeapYear(
    UINT_PTR aYear
)
{
    if (0 != (aYear & 3))
        return FALSE;
    // divisibla by 4
    if (0 == (aYear % 400))
        return FALSE;
    // divisible by 4 and 400
    return TRUE;
}

BOOL
K2_IsOsTimeValid(
    K2_DATETIME const *apTime
)
{
    static UINT8 sMonthDays[12] = {
        31, 28, 31, 30,
        31, 30, 31, 31,
        30, 31, 30, 31
    };

    //
    // brunt checks
    //
    if ((apTime->mYear < 1980) ||
        (apTime->mYear > 2107) ||

        (apTime->mMonth == 0) ||
        (apTime->mMonth > 12) ||

        (apTime->mDay == 0) ||

        (apTime->mHour >= 24) ||
        (apTime->mMinute >= 60) ||
        (apTime->mSecond >= 60) ||
        (apTime->mMillisecond >= 1000))
    {
        return FALSE;
    }

    //
    // day checks
    //
    if ((2 == apTime->mMonth) && (K2_IsLeapYear(apTime->mYear)))
    {
        if (apTime->mDay > 29)
            return FALSE;
    }
    else
    {
        if (apTime->mDay > sMonthDays[apTime->mMonth - 1])
            return FALSE;
    }

    return TRUE;
}
