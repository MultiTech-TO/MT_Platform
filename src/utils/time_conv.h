#ifndef _TIME_CONVERSION_
#define _TIME_CONVERSION_

#define hr_2_sec(X)         (X * 3600UL)
#define hr_2_min(X)         (X * 60UL)
#define min_2_sec(X)        (X * 60UL)
#define min_2_hr(X)         (X / 60UL)
#define sec_2_min(X)        (X / 60UL)
#define sec_2_hr(X)         (X / 36000UL)

#endif