/**
 * @file Hw_WatchDog.h
 * @brief CPU watchdog access used by the system lifecycle.
 */

#ifndef HW_WATCHDOG_H
#define HW_WATCHDOG_H

void Hw_EnableWatchDog(void);
void Hw_FeedWatchDog(void);

#endif /* HW_WATCHDOG_H */
