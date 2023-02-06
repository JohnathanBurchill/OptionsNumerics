/*
    Options Numerics: on_status.h

    Copyright (C) 2023  Johnathan K Burchill

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef _ON_STATUS_H
#define _ON_STATUS_H

enum onStatusCodes
{
    ON_OK = 0,
    
    ON_NO_SCREEN,
    ON_NO_USERINPUT,
    ON_NO_WINDOW,

    ON_MISSING_ARG_POINTER,
    ON_HEAP_MEMORY_ERROR,
    
    ON_FILE_READ_ERROR,
    ON_FILE_WRITE_ERROR,

    ON_PARSE_INVALID_DATE_STRING,

    ON_MISSING_RETURN_POINTER,
    ON_OPTIONS_MODELS_MAX_ITERATIONS_REACHED,

    ON_REST_LIBCURL_ERROR,
    
    ON_QUESTRADE_INVALID_TOKEN,
    ON_QUESTRADE_INVALID_ACCOUNT,

    ON_FRED_OK = 0,
    ON_FRED_INVALID_JSON,
    ON_FRED_NO_DATA = 1,

    ON_PIO_INVALID_RESPONSE,
    ON_PIO_NO_STOCK_TICKER,
    ON_PIO_NO_STOCK_TICKER_NAME,
    ON_PIO_NO_STOCK_QUOTE,
    ON_PIO_NO_STOCK_DAY_INFO,
    ON_PIO_NO_STOCK_MINUTE_DATA,   
    ON_PIO_NO_STOCK_PREVIOUS_DAY_INFO,
    ON_PIO_NO_OPTIONS_RESULTS,
    ON_PIO_NO_OPTIONS_QUOTE,
    ON_PIO_NO_OPTIONS_DAY_INFO,
    ON_PIO_NO_OPTIONS_GREEKS,
    ON_PIO_NO_OPTIONS_DETAILS,
    ON_PIO_NO_TICKER_ARG,

    ON_PIO_REST_NO_JSON_ROOT,
    ON_PIO_REST_NO_JSON_RESULTS,
    ON_PIO_REST_JSON_NO_ARRAY_ENTRY,
    ON_PIO_REST_INVALID_JSON,

    ON_WSS_PROTOCOL_NOT_SUPPORTED,
    ON_PIO_WSS_NOT_ENABLED,
    ON_PIO_WSS_NOT_AUTHENTICATED,
    ON_PIO_WSS_NO_DATA,
    ON_PIO_WSS_NO_JSON_ROOT,
    ON_PIO_WSS_JSON_ROOT_NOT_ARRAY,
    ON_PIO_WSS_JSON_NO_ARRAY_ENTRY,
    ON_PIO_WSS_UNHANDLED_TEXT_FRAME,
    ON_PIO_WSS_NO_CHANNEL,
    ON_PIO_WSS_LIBCURL_ERROR
};

#endif // _ON_STATUS_H
