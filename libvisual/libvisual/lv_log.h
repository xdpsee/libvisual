/* Libvisual - The audio visualisation framework.
 *
 * Copyright (C) 2012      Libvisual team
 *               2004-2006 Dennis Smit
 *
 * Authors: Chong Kai Xiong <kaixiong@codeleft.sg>
 *          Dennis Smit <ds@nerds-incorporated.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef _LV_LOG_H
#define _LV_LOG_H

#include <libvisual/lvconfig.h>
#include <libvisual/lv_defines.h>

/**
 * @defgroup VisLog VisLog
 * @{
 */

LV_BEGIN_DECLS

/**
 * Used to determine the severity of the log message using the visual_log
 * define.
 *
 * @see visual_log
 */
typedef enum {
	VISUAL_LOG_MIN,      /**< this should always remain the FIRST entry */
	VISUAL_LOG_DEBUG,    /**< Debug message, to use for debug messages. */
	VISUAL_LOG_INFO,     /**< Informative message, can be used for general info. */
	VISUAL_LOG_WARNING,  /**< Warning message, use to warn the user. */
	VISUAL_LOG_ERROR,    /**< Error message, use to notify the user of fatals. */
	VISUAL_LOG_CRITICAL, /**< Critical message, when a critical situation happens. */
	VISUAL_LOG_MAX,      /**< this should always remain the LAST entry */
} VisLogSeverity;

typedef struct {
	const char   *file;
	const char   *func;
	unsigned int  line;
} VisLogSource;

/**
 * Functions that want to handle messages must match this signature.
 *
 * @arg message The message that will be shown, exactly the same as that was passed
 * to visual_log(), but after formatting.
 *
 * @arg funcname The name of the function that invokes visual_log(). On non-GNU systems
 * this will probably be NULL.
 *
 * @arg priv Private field to be used by the client. The library will never touch this.
 */
typedef void (*VisLogHandlerFunc) (VisLogSeverity severity, const char *message,
	const VisLogSource *source, void *priv);

/**
 * Set the log verbosity level. Any message of a lower severity then
 * the given level will be dropped.
 *
 * @param level The verbosity level
 */
LV_API void visual_log_set_verbosity (VisLogSeverity level);

/**
 * Get the current library it's verbosity level.
 *
 * @return The verboseness level as a VisLogVerboseness enumerate value.
 */
LV_API VisLogSeverity visual_log_get_verbosity (void);

/**
 * Set the callback function that handles info messages.
 *
 * @param handler The custom message handler callback.
 * @param priv Optional private data to pass on to the handler.
 */
LV_API void visual_log_set_handler (VisLogSeverity severity, VisLogHandlerFunc handler, void *priv);

/**
 * Logs a message
 *
 * @param severity Determines the severity of the message using VisLogSeverity.
 * @param format The format string of the log message.
 *
 * @see VisLogSeverity
 */
#define visual_log(severity,...)      \
	_lv_log (severity,            \
	         __FILE__,            \
	         __LINE__,            \
	         __PRETTY_FUNCTION__, \
	         __VA_ARGS__)

LV_API void _lv_log (VisLogSeverity severity, const char *file, int line, const char *funcname,
	const char *fmt, ...) LV_CHECK_PRINTF_FORMAT(5, 6);

LV_END_DECLS

/**
 * @}
 */

#endif /* _LV_LOG_H */
