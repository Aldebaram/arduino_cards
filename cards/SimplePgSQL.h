/*
 * SimplePgSQL.h - Lightweight PostgreSQL connector for Arduino
 * Copyright (C) Bohdan R. Rau 2016 <ethanak@polip.com>
 *
 * SimplePgSQL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * SimplePgSQL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with SimplePgSQL.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */
#ifndef _SIMPLEPGSQL
#define _SIMPLEPGSQL 1


#ifndef ESP8266
// You need MD5 library from https://github.com/tzikis/ArduinoMD5
// for Arduino boards. Uncomment only if you have this library and
// you must use md5 passwords.
// Do not use it on 32 kB ATMega processors!

// #define PG_USE_MD5 1

#else
// ESP8266 has MD5 code in ROM so there is no need to comment
#define PG_USE_MD5 1
#endif

#include <Arduino.h>
#include <Client.h>

typedef enum
{
	CONNECTION_OK,
	CONNECTION_BAD,
	CONNECTION_NEEDED,			/* setDbLogin() needed */
	/* Internal states here */
	CONNECTION_AWAITING_RESPONSE,		/* Waiting for a response from the
                                                 * postmaster.        */
	CONNECTION_AUTH_OK			/* Received authentication; waiting for
								 * backend startup. */
} ConnStatusType;

#ifdef ESP8266
#define PG_BUFFER_SIZE 2048
#else
#define PG_BUFFER_SIZE 256
#endif

// maximum number of fields in backend response
// must not exceed number of bits in _formats and _null
#define PG_MAX_FIELDS 32

// ignore notices and notifications
#define PG_FLAG_IGNORE_NOTICES 1
// do not store column names
#define PG_FLAG_IGNORE_COLUMNS 2
// never set this flag manually!
# define PG_FLAG_STATIC_BUFFER 4

// ready for next query
#define PG_RSTAT_READY 1
// command sent
#define PG_RSTAT_COMMAND_SENT 2
// column names in buffer
#define PG_RSTAT_HAVE_COLUMNS 4
// row values in buffer
#define PG_RSTAT_HAVE_ROW 8
// summary (number of tuples/affected rows) received
#define PG_RSTAT_HAVE_SUMMARY 16
// error message in buffer
#define PG_RSTAT_HAVE_ERROR 32
// notice/notification in buffer
#define PG_RSTAT_HAVE_NOTICE 64

#define PG_RSTAT_HAVE_MASK (PG_RSTAT_HAVE_COLUMNS | \
    PG_RSTAT_HAVE_ROW | \
    PG_RSTAT_HAVE_SUMMARY | \
    PG_RSTAT_HAVE_ERROR | \
    PG_RSTAT_HAVE_NOTICE)

#define PG_RSTAT_HAVE_MESSAGE (PG_RSTAT_HAVE_ERROR | PG_RSTAT_HAVE_NOTICE)

class PGconnection {
    public:
        PGconnection(Client *c,
                int flags = 0,
                int memory = 0,
                char *foreignBuffer = NULL);
        /*
         * returns connection status.
         * passwd may be null in case of 'trust' authorization.
         * only 'trust', 'password' and 'md5' (if compiled in)
         * authorization modes are implemented.
         * ssl mode is not implemented.
         * database name defaults to user name         *
         */
        int setDbLogin(IPAddress server,
            const char *user,
            const char *passwd = NULL,
            const char *db = NULL,
            const char *charset = NULL,
            int port = 5432);
        /*
         * performs authorization tasks if needed
         * returns current connection status
         * must be called periodically until OK, BAD or NEEDED
         */
        int status(void);
        /*
         * sends termination command if possible
         * closes client connection and frees internal buffer
         */
        void close(void);
        /*
         * sends query to backend
         * returns negative value on error
         * or zero on success
         */
        int execute(const char *query, int progmem = 0);

        /* should be called periodically in idle state
         * if notifications are enabled
         * returns:
         * - negative value on error
         * - zero if no interesting data arrived
         * - current data status if some data arrived
         */
        int getData(void);
        /*
         * returns pointer to n-th column name in internal buffer
         * if available or null if column number out of range
         * will be invalidated on next getData call
         */
        char *getColumn(int n);
        /*
         * returns pointer to n-th column value in internal buffer
         * if available or null if column number out of range
         * or value is NULL
         * will be invalidated on next getData call
         */
        char *getValue(int);
        /*
         * returns pointer to message (error or notice)
         * if available or NULL
         * will be invalidated on next getData call
         */
        char *getMessage(void);
        int dataStatus(void) {
            return result_status;
        };
        int nfields(void) {
            return _nfields;
        };
        int ntuples(void) {
            return _ntuples;
        };
        /*
         * returns length of escaped string
         * single quotes and E prefix (if needed)
         * will be added.
         */
        int escapeString(const char *inbuf, char *outbuf);
        /*
         * returns length of escaped string
         * double quotes will be added.
         */
        int escapeName(const char *inbuf, char *outbuf);
        /*
         * sends formatted query to backend
         * returns negative value on error
         * or zero on success
         * Formatting sequences:
         * %s - string literal (will be escaped with escapeString)
         * %n - name (will be escaped with escapeName)
         * %d - int (single quotes will be added)
         * %l - long int (single quotes will be added)
         * %% - % character
         */
        int executeFormat(int progmem, const char *format, ...);


    private:
        Client *client;
        int pqPacketSend(char pack_type, const char *buf, int buf_len, int progmem = 0);
        int pqGetc(char *);
        int pqGetInt4(int32_t *result);
        int pqGetInt2(int16_t *result);
        int pqGetnchar(char *s, int len);
        int pqSkipnchar(int len);
        int pqGets(char *s, int maxlen);
        int pqGetRowDescriptions(void);
        int pqGetRow(void);
        void setMsg(const char *, int);
        void setMsg_P(const char *, int);
        int pqGetNotice(int);
        int pqGetNotify(int32_t);
        char *_user;
        char *_passwd;
        char *Buffer;
        int bufSize;
        int bufPos;
        int writeMsgPart(const char *s, int len, int fine);
        int writeMsgPart_P(const char *s, int len, int fine);
        int32_t writeFormattedQuery(int32_t length, int progmem, const char *format, va_list va);

        int build_startup_packet(char *packet, const char *db, const char *charset);
        byte conn_status;
        byte attempts;
/*
        int32_t be_pid;
        int32_t be_key;
*/
        int16_t _nfields;
        int16_t _ntuples;
        uint32_t _formats;
        uint32_t _null;
        byte _binary;
        byte _flags;
        int result_status;
};

#endif

