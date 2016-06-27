/*
 *  NAME:
 *      usleep     -- This is the precision timer for Test Set
 *                    Automation. It uses the select(2) system
 *                    call to delay for the desired number of
 *                    micro-seconds. This call returns ZERO
 *                    (which is usually ignored) on successful
 *                    completion, -1 otherwise.
 *
 *  ALGORITHM:
 *      1) We range check the passed in microseconds and log a
 *         warning message if appropriate. We then return without
 *         delay, flagging an error.
 *      2) Load the Seconds and micro-seconds portion of the
 *         interval timer structure.
 *      3) Call select(2) with no file descriptors set, just the
 *         timer, this results in either delaying the proper
 *         ammount of time or being interupted early by a signal.
 *
 *  HISTORY:
 *      Added when the need for a subsecond timer was evident.
 *
 *  AUTHOR:
 *      Michael J. Dyer                   Telephone:   AT&T 414.647.4044
 *      General Electric Medical Systems        GE DialComm  8 *767.4044
 *      P.O. Box 414  Mail Stop 12-27         Sect'y   AT&T 414.647.4584
 *      Milwaukee, Wisconsin  USA 53201                      8 *767.4584
 *      internet:  mike@sherlock.med.ge.com     GEMS WIZARD e-mail: DYER
 */

int usleep(unsigned long int microSeconds);
