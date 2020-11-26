/* Copyright (c) 2020 S. Roesch. All rights reserved.*/

/*

   NAME
     loginfo.c - Postgres extension for additional log file functionality.

   DESCRIPTION
     This file implements additional log file functions to make the log
     files more accessible to administrators. In addition the functions
     also allow better tests and verify that the expected errors have
     been written to the log file.

   EXPORT FUNCTION(S)
   
   NOTES
*/

#include <glob.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "postgres.h"
#include "access/tupdesc.h"
#include "access/xact.h"
#include "executor/spi.h"
#include "fmgr.h"
#include "funcapi.h"
#include "miscadmin.h"
#include "nodes/execnodes.h"
#include "postmaster/postmaster.h"
#include "postmaster/syslogger.h"
#include "utils/builtins.h"
#include "utils/elog.h"
#include "utils/guc.h"
#include "utils/snapmgr.h"

/*---------------------------------------------------------------------------
                     PRIVATE TYPES AND CONSTANTS
  ---------------------------------------------------------------------------*/

/* Number of fields of the CSV database view. */
#define NUM_CSV_FIELDS 23

/* Number of fields of the log database view. */
#define NUM_LOGINFO_FIELDS 2

/* Number of fields for the log files view. */
#define NUM_LOGFILE_FIELDS 3

/* Number of fields of the database view to display the log file contents. */
#define NUM_LOGFILE_CONTENTS_FIELDS 1

/* Max line length of a logfile line. */
#define MAX_LOGFILE_LINE_LEN 1024

/* Max field length of a log file field. */
#define MAX_CSV_FIELD_LEN 1024

/* Maximum soft link name length. */
#define MAX_LINK_NAME_LEN 512

/* Length of timestamp field. */
#define TIMESTAMP_LEN 60

/* Template for log filename. */
static const char * const PROC_FILENAME_TEMPLATE = "/proc/%d/fd/2";

/* Query latest csv log file. */
static const char * const SQL_QUERY_CSV_FILES =
	"SELECT filename FROM ls_csv_files ORDER BY last_update DESC LIMIT 1";

/* Query latest log file. */
static const char * const SQL_QUERY_LOG_FILES = 
	"SELECT filename FROM ls_log_files ORDER BY last_update DESC LIMIT 1";

/* Definition of a result set. */
typedef struct ResultSet
{
    ReturnSetInfo     *rsInfo;                          /* Rowset definition */
    TupleDesc          tupleDesc;                        /* Tuple descriptor */
    Tuplestorestate   *tupleStore;                 /* Tuple store for rowset */
    MemoryContext      oldCtx;                  /* Saved context, to restore */
    MemoryContext      perQueryCtx;                  /* Query memory context */
} ResultSet;

/* Definition of the CSV parsing state machine.
 *
 * State transitions of the state machine:
 *
 *    Init |------------------------------------------------------------
 *         |                                                           |
 *    ^    ------------------> EmptyField --------------               |
 *    |    |                                           |               |
 *    |    |-----------------> DefaultField -----------+---------------|
 *    |    |                                           |               |
 *    |    |-----------------> StringStart -------------               |
 *    |                            |                   |               |
 *    |                            V                   V               |
 *    |                                                                |
 *    |                        StringField -------> TermField ---------|
 *    |                                                |               |
 *    |------------------------------------------------|               |
 *    |                                                |               |
 *    |                                                V               V
 *    |-------------------------------------------- TermRecord ----> Term --> Error
 *                                                                     |        |
 *                                                                     V        |
 *     																  End  <-----
 *
 */
typedef enum CSVState {
	STATE_INIT,
	STATE_EMPTY_FIELD,
	STATE_DEFAULT_FIELD,
	STATE_STRING_START,
	STATE_STRING_FIELD,
	STATE_TERM_FIELD,
	STATE_TERM_RECORD,
	STATE_TERM,
	STATE_ERROR,
	STATE_END,
} CSVState;

/* Definition of the of the errors fo the CSV parsing state machine. */
typedef enum ParseError {
	PARSE_SUCCESS,
	PARSE_FIELD_TOO_LONG,
	PARSE_TOO_MANY_FIELDS,
	PARSE_NOT_ENOUGH_FIELDS,
	PARSE_EOF_IN_RECORD,
} ParseError;

/* Export functions. */
PG_MODULE_MAGIC;
PG_FUNCTION_INFO_V1(li_cat_log_file);
PG_FUNCTION_INFO_V1(li_ls_log_files);
PG_FUNCTION_INFO_V1(li_log_info);
PG_FUNCTION_INFO_V1(li_test_cat_log_file);

/*--------------------------- CreateResultSet -------------------------------*/
/*
  NAME:
    CreateResultSet - Build a new result set and initialize the tuple store.

  PARAMETERS:
    resultSet   - (INOUT) Ptr to result set
	fcInfo      - (IN)    Ptr to result set info structure

  DESCRIPTION:
    This function initializes a new result set structure to populate a
    database view. This function builds a materialized database view.

  NOTES:
    This function requires that no composite columns are defined in the view.

  RETURNS:
    void
*/
static
void CreateResultSet(ResultSet                *resultSet,
                     FunctionCallInfoBaseData *fcInfo)
{
    resultSet->rsInfo = (ReturnSetInfo *)fcInfo->resultinfo;
    if (!resultSet->rsInfo || !IsA(resultSet->rsInfo, ReturnSetInfo))
    {
        ereport(ERROR,
                (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
                 errmsg("Set-valued function call in context that cannot "
                        "except a set")));
    }

    if (!(resultSet->rsInfo->allowedModes & SFRM_Materialize))
    {
        ereport(ERROR,
                (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
                 errmsg("Materialize mode required , but is is not allowed "
                        "in this context")));
    }

    /* Build a tuple descriptor for our return type. */
    if (get_call_result_type(fcInfo, NULL, &resultSet->tupleDesc) != TYPEFUNC_COMPOSITE)
    {
        ereport(ERROR,
                (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("Return type must be a row type")));
    }

    /* Create the tuple store */
    resultSet->perQueryCtx = resultSet->rsInfo->econtext->ecxt_per_query_memory;
    resultSet->oldCtx      = MemoryContextSwitchTo(resultSet->perQueryCtx);
    resultSet->tupleStore  = tuplestore_begin_heap(true, false, work_mem);

    /* Build result set. */
    resultSet->rsInfo->returnMode = SFRM_Materialize;
    resultSet->rsInfo->setResult  = resultSet->tupleStore;
    resultSet->rsInfo->setDesc    = resultSet->tupleDesc;
}

/*------------------------------- AddLogInfo --------------------------------*/
/*
  NAME:
    AddLogInfo - Adds a new log info tuple to log info view.

  PARAMETERS:
    resultSet   - (INOUT) Ptr to result set
	name        - (IN)    Name of the log info setting
	value       - (IN)    Value of the log info setting.

  DESCRIPTION:
    This function adds a tuple to the log info view (log_info).

  NOTES:

  RETURNS:
    void
*/
static
void AddLogInfo(ResultSet *resultSet, const char *name, const char *value)
{
	Datum   values[NUM_LOGINFO_FIELDS];                      /* Values array */
	bool    nulls[NUM_LOGINFO_FIELDS];                        /* Nulls array */

	MemSet(values, 0, sizeof(values));
	MemSet(nulls, 0, sizeof(nulls));

	values[0] = CStringGetTextDatum(name);
	values[1] = CStringGetTextDatum(value);

	tuplestore_putvalues(resultSet->tupleStore,
						 resultSet->tupleDesc,
						 values,
						 nulls);
}

/*---------------------------- AddLogInfoByName -----------------------------*/
/*
  NAME:
    AddLogInfoByName - Adds a new log info tuple to the log info view by config
                       name.

  PARAMETERS:
    resultSet   - (INOUT) Ptr to result set
	name        - (IN)    Name of the log info setting

  DESCRIPTION:
    This function adds a tuple to the log info view. The value is determined
    from the postgres configuration (log_info).

  NOTES:

  RETURNS:
    void
*/
static
void AddLogInfoByName(ResultSet *resultSet, const char *name)
{
	AddLogInfo(resultSet, name, GetConfigOption(name, false, false));
}

/*------------------------------- AddLogFile --------------------------------*/
/*
  NAME:
    AddLogFile - Adds a new log file tuple to the log files view.

  PARAMETERS:
    resultSet   - (INOUT) Ptr to result set
	name        - (IN)    Name of the log info setting
	timestamp   - (IN)    Timestamp of the last file modification
	size        - (IN)    Size of the log file in bytes

  DESCRIPTION:
    This function adds a tuple to the log files view (ls_log_files,
    ls_csv_files).

  NOTES:

  RETURNS:
    void
*/
static
void AddLogFile(ResultSet *resultSet,
				const char *name,
				const char *timestamp,
	            long int    size)
{
	Datum   values[NUM_LOGFILE_FIELDS];                      /* Values array */
	bool    nulls[NUM_LOGFILE_FIELDS];                        /* Nulls array */

	MemSet(values, 0, sizeof(values));
	MemSet(nulls, 0, sizeof(nulls));

	values[0] = CStringGetTextDatum(name);
	values[1] = CStringGetTextDatum(timestamp);
	values[2] = Int64GetDatum(size);

	tuplestore_putvalues(resultSet->tupleStore,
						 resultSet->tupleDesc,
						 values,
						 nulls);
}

/*------------------------- AddLogFileContents ------------------------------*/
/*
  NAME:
    AddLogFileContents - Adds a new tuple to the log file contents view.

  PARAMETERS:
    resultSet   - (INOUT) Ptr to result set
	line        - (IN)    Line from the log file

  DESCRIPTION:
    This function adds a tuple to the log file contents view (cat_log_file).

  NOTES:

  RETURNS:
    void
*/
static
void AddLogFileContents(ResultSet *resultSet, const char *line)
{
	Datum   values[NUM_LOGFILE_CONTENTS_FIELDS];             /* Values array */
	bool    nulls[NUM_LOGFILE_CONTENTS_FIELDS];               /* Nulls array */
	
	MemSet(values, 0, sizeof(values));
	MemSet(nulls, 0, sizeof(nulls));
	
	values[0] = CStringGetTextDatum(line);
		
	tuplestore_putvalues(resultSet->tupleStore,
						 resultSet->tupleDesc,
						 values,
						 nulls);
}

/*------------------------- AddCSVFileContents ------------------------------*/
/*
  NAME:
    AddLogFileContents - Adds a new tuple to the csv file contents view.

  PARAMETERS:
    resultSet   - (INOUT) Ptr to result set
	fields      - (IN)    Ptr to array of fields

  DESCRIPTION:
    This function adds a tuple to the csv file contents view (cat_csv_file).
	The fields is an array of strings. It must have NUM_CSV_FIELDS.

  NOTES:

  RETURNS:
    void
*/
static
void AddCSVFileContents(ResultSet *resultSet, char **fields)
{
	Datum   values[NUM_CSV_FIELDS];                          /* Values array */
	bool    nulls[NUM_CSV_FIELDS];                            /* Nulls array */
				
	MemSet(values, 0, sizeof(values));
	MemSet(nulls, 0, sizeof(nulls));
				
	for (int i = 0; i < NUM_CSV_FIELDS; i++)
	{
		values[i] = CStringGetTextDatum(fields[i]);
	}
				
	tuplestore_putvalues(resultSet->tupleStore,
						 resultSet->tupleDesc,
						 values,
						 nulls);
}

/*-------------------------- GetLinkedFilename ------------------------------*/
/*
  NAME:
    GetLinkedFilename - Return the filename of a soft link

  PARAMETERS:
    filename   - (IN) Filename

  DESCRIPTION:
    This function returns the filename of a soft link. The function is linux
    specific. It doesn't use the lstat function to determine the link name
    size as the proc filesystem is only a virtual filesystem and the reported
    file name size can only be an estimate.

  NOTES:
    The returned filename must be de-allocated from the caller.

  RETURNS:
    Name of the linked filename or NULL.
*/
static
char *GetLinkedFilename(const char *filename)
{
	size_t    linklen    = MAX_LINK_NAME_LEN;         /* Soft link name size */
    char     *linkname   = NULL;                    /* Filename of soft link */

	while (1)
	{
		linkname = malloc(linklen);
		if (linkname)
		{
			ssize_t   r;                              /* Actual size of link */
			
			r = readlink(filename, linkname, linklen);
			if (r < 0)
			{
				free(linkname);
				linkname = NULL;

				ereport(ERROR,
						(errcode(ERRCODE_INTERNAL_ERROR),
						 errmsg("readlink failed")));
			}
			else if (r > linklen - 1)
			{
				pfree(linkname);
				linklen *= 2;

				if (linklen > PATH_MAX)
				{
					ereport(ERROR,
							(errcode(ERRCODE_INTERNAL_ERROR),
							 errmsg("readlink returned > 16384")));
				}
			}
			else
			{
				linkname[r] = '\0';
				break;
			}
		}
		else
		{
			ereport(ERROR,
					(errcode(ERRCODE_OUT_OF_MEMORY),
					 errmsg("cannot alloc os link information")));
		}
	}
	
	return linkname;
}

/*----------------------------- QueryDirectory ------------------------------*/
/*
  NAME:
    QueryDirectory - Filename list that follow a wildcard pattern

  PARAMETERS:
    pattern     - (IN)    Wildcard pattern for directory search
    resultSet   - (INOUT) Ptr to result set

  DESCRIPTION:
    This function queries the directory for matching files and adds them to
    the result set. The parameter pattern consists of the directory name and
    the pattern for the filename search.

  NOTES:

  RETURNS:
    void
*/
static
void QueryDirectory(const char *pattern, ResultSet *resultSet)
{
	glob_t   gbuffer;                                        /* Match buffer */

	if (glob(pattern, 0, NULL, &gbuffer) == 0)
	{
		char   **names = gbuffer.gl_pathv;             /* List of file names */
		
		for ( ; *names; names++)
		{
			struct stat   sb;                               /* File metadata */
			
			if (lstat(*names, &sb) != -1)
			{
				char   timestamp[TIMESTAMP_LEN];   /* Last file modification */
				
				strftime(timestamp,
						 sizeof(timestamp),
						 "%Y.%m.%d %H:%M:%S",
						 localtime(&sb.st_ctime));
				AddLogFile(resultSet, *names, timestamp, sb.st_size);
			}
		}
	}
}

/*----------------------------- GetCurrentLogFilename -----------------------*/
/*
  NAME:
    GetCurrentLogFilename - Returns the filename of the active log file

  PARAMETERS:
    type    - (IN) Type of log file : 'log' or 'csv'

  DESCRIPTION:
    This function executes a SQL query to determine the current log file.
    Depending on the value of the parameter it searches for log or csv files.

  NOTES:

  RETURNS:
    Name of log file or NULL.
*/
static
const char *GetCurrentLogFilename(const char *type)
{
	int          err;                                           /* SQL error */
	int          nbrows;                     /* Number of rows in result set */
	char        *name        = NULL;                        /* Log file name */
	const char  *sql         = (strcmp(type, "csv") == 0        /* SQL query */
				         		   ? SQL_QUERY_CSV_FILES
				         		   : SQL_QUERY_LOG_FILES);

	/* Connect to postgres. */
	err = SPI_connect();
	if (err != SPI_OK_CONNECT)
	{
		elog(ERROR, "SPI_connect: %s", SPI_result_code_string(err));
	}

	/* Query current log or csv file. */
	err = SPI_execute(sql, true, 0);
	if (err != SPI_OK_SELECT)
		elog(ERROR, "SPI_execute: %s", SPI_result_code_string(err));

	nbrows = SPI_processed;
	if (nbrows > 0 && SPI_tuptable)
	{
		TupleDesc        tupdesc  = SPI_tuptable->tupdesc;
			                                             /* Tuple descriptor */
		SPITupleTable   *tuptable = SPI_tuptable;             /* Tuple table */
		HeapTuple        tuple    = tuptable->vals[0];      /* Tuple storage */

		name = SPI_getvalue(tuple, tupdesc, 1);
	}

	/* Cleanup connection. */
	err = SPI_finish();
	if (err != SPI_OK_FINISH)
		elog(ERROR, "SPI_finish: %s", SPI_result_code_string(err));

	return name;
}

/*----------------------------- FreeFieldsMemory ----------------------------*/
/*
  NAME:
    FreeFieldsMemory - Deallocates the memory of the CSV fields 

  PARAMETERS:
    fields     - (INOUT) Ptr to array of string fields
	numFields  - (IN)    Number of fields to deallocate

  DESCRIPTION:
    This function the CSV fields array.

  NOTES:

  RETURNS:
    void
*/
static
void FreeFieldsMemory(char **fields, int numFields)
{
	for (int i = 0; i < numFields; i++)
	{
		pfree(fields[i]);
	}
}

/*----------------------------- ProcessLogFile ------------------------------*/
/*
  NAME:
    ProcessLogFile - Read log file and create view with its contents

  PARAMETERS:
    f           - (INOUT) Ptr to file handle 
    resultSet   - (INOUT) Ptr to result set

  DESCRIPTION:
    This function processes a log file and creates a result set with its
    contents.

  NOTES:

  RETURNS:
    void
*/
static
void ProcessLogFile(FILE *f, ResultSet *resultSet)
{
	char   buffer[MAX_LOGFILE_LINE_LEN];         /* Buffer for next log line */

	while (fgets(buffer, sizeof(buffer), f))
	{
		int   len = strlen(buffer);                           /* Line length */

		buffer[len > 0 ? len - 1 : 0] = '\0';
		AddLogFileContents(resultSet, buffer);
	}
}

/*----------------------------- ProcessCSVFile ------------------------------*/
/*
  NAME:
    ProcessCSVFile - Read csv file and create view with its contents

  PARAMETERS:
    f           - (INOUT) Ptr to file handle 
    resultSet   - (INOUT) Ptr to result set

  DESCRIPTION:
    This function processes a CSV file and creates a result set with its
    contents. In case the fails violates the expected contents, errors are
    raised. The function does not try to continue parsing the file in that
    case.

  NOTES:
    The state machine is described in the definition of the state enum type.

  RETURNS:
    void
*/
static
void ProcessCSVFile(FILE *f, ResultSet *resultSet)
{
	int         pos      = 0;               /* Current buffer write position */
	int         field    = 0;                                /* Field number */
	int         quotes   = 0;                  /* In string quote processing */

	CSVState    state    = STATE_INIT;                /* State machine state */
	ParseError  error    = PARSE_SUCCESS;                    /* Parse errors */
	bool        isString = false;                       /* Processing string */

	char       *buffer   = NULL;              /* Write buffer for next field */
    char       *fields[NUM_CSV_FIELDS];   /* Cache to store processed fields */

	while (state != STATE_END)
	{
		char   ch;                                         /* Next character */

		switch (state)
		{
		case STATE_INIT:
			/* Start of a field. */
			buffer = palloc(MAX_CSV_FIELD_LEN);
			
			ch = fgetc(f);
			if (feof(f))
			{
				state = STATE_TERM;
				break;
			}

			/* Trim leading spaces. */
			if (!isspace(ch))
			{
				if (ch == '"')
					state = STATE_STRING_START;
				else if (ch == ',')
					state = STATE_EMPTY_FIELD;
				else
				{
					state = STATE_DEFAULT_FIELD;
					buffer[pos++] = ch;
				}
			}
			break;

		case STATE_EMPTY_FIELD:
			/* Field has no contents. */
			state = STATE_TERM_FIELD;
			break;
			
		case STATE_DEFAULT_FIELD:
			/* This is a default field - not a string field. */
			ch = fgetc(f);
			if (feof(f))
				state = STATE_TERM;
			else if (ch == ',' || ch == '\n')
				state = STATE_TERM_FIELD;
			else
				buffer[pos++] = ch;
			break;

		case STATE_STRING_START:
			/* Start of a string field. */
			isString = true;
			state    = STATE_STRING_FIELD;
			break;
			
		case STATE_STRING_FIELD:
			/* This is a string field. */
			ch = fgetc(f);
			if (feof(f))
			{
				state = STATE_TERM; 
			}
			else if (!isString && (ch == ',' || ch == '\n'))
			{
				state = STATE_TERM_FIELD;
			}
		    else if (ch != '"')
			{
				buffer[pos++] = ch;
			}
			else
			{
				if (!isString)
				{
					buffer[pos++] = ch;
					isString      = true;
					quotes        = 2;
				}
				else if (quotes)
				{
					--quotes;
					if (quotes == 0)
						buffer[pos++] = ch;
				}	
				else
					isString = false;
			}
			break;

		case STATE_TERM_FIELD:
			/* Field is terminated. */
			buffer[pos]     = '\0';
			fields[field++] = buffer;
			buffer          = NULL;

			if (ch == '\n')
			{
				state = STATE_TERM_RECORD;
			}
			else
			{
				if (field > NUM_CSV_FIELDS)
				{
					error = PARSE_TOO_MANY_FIELDS;
					state = STATE_TERM;
				} else {
					state = STATE_INIT;
				}
			}

			pos = 0;
			break;

		case STATE_TERM_RECORD:
			/* Record is terminated. */
			if (field != NUM_CSV_FIELDS)
			{
				error = PARSE_NOT_ENOUGH_FIELDS;
				state = STATE_TERM;
			}
			else
			{
				AddCSVFileContents(resultSet, fields);
				FreeFieldsMemory(fields, field);

				field = 0;
				state = STATE_INIT;
			}
			break;

		case STATE_TERM:
			/* Cleanup already allocated fields. */
			if (buffer)
				pfree(buffer);
			FreeFieldsMemory(fields, field);

			if (error == PARSE_SUCCESS && field != 0)
				error = PARSE_EOF_IN_RECORD;

			if (error != PARSE_SUCCESS)
				state = STATE_ERROR;
			else
				state = STATE_END;
			break;

		case STATE_ERROR:
			/* Close file handle before raising error. */
			fclose(f);
			
			switch (error)
			{
			case PARSE_FIELD_TOO_LONG:
				ereport(ERROR,
						(errcode(ERRCODE_INTERNAL_ERROR),
						 errmsg("invalid CSV field length")));
				break;

			case PARSE_NOT_ENOUGH_FIELDS:
				ereport(ERROR,
				 		(errcode(ERRCODE_INTERNAL_ERROR),
				 		 errmsg("invalid CSV format, not enough fields")));
				break;

			case PARSE_TOO_MANY_FIELDS:
				ereport(ERROR,
				 		(errcode(ERRCODE_INTERNAL_ERROR),
				 		 errmsg("invalid CSV format, too many fields")));
				break;

			case PARSE_EOF_IN_RECORD:
				ereport(ERROR,
				 		(errcode(ERRCODE_INTERNAL_ERROR),
				 		 errmsg("eof while parsing record")));
				break;

			case PARSE_SUCCESS:
				/* Fallthrough */
				break;
			}
			break;

		case STATE_END:
			/* Silence compiler about unhandled enum. */
			break;
		}

		/* Check for valid field length. */
		if (pos >= MAX_CSV_FIELD_LEN)
		{
			error = PARSE_FIELD_TOO_LONG;
			state = STATE_TERM;
		}
	}
}

/*----------------------------- LogFileContents ------------------------------*/
/*
  NAME:
    LogFileContents - Read a log file and return a view with its contents

  PARAMETERS:
    filename    - (IN)    Log filename
    type        - (IN)    Type of the log file: 'log' or 'csv'
    resultSet   - (INOUT) Ptr to result set

  DESCRIPTION:
    This function opens the log file, parses the file and returns the contents
    as a database view. Depending on the type of the log file different
    parsing routines are used.

  NOTES:

  RETURNS:
    void
*/
static
void LogFileContents(const char *filename,
					 const char *extension,
					 ResultSet  *resultSet)
{
	FILE   *f = fopen(filename, "r");                     /* Log file handle */
	if (f)
	{
		if (strcmp(extension, "log") == 0)
			ProcessLogFile(f, resultSet);
		else
			ProcessCSVFile(f, resultSet);
	}
	else
	{
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("invalid file name")));
	}
	
	fclose(f);
}

/*------------------------------- li_log_info -------------------------------*/
/*
  NAME:
    li_log_info - Callback function for log_info view

  PARAMETERS:
    -

  DESCRIPTION:
    This function creates a view display log information.

  NOTES:

  RETURNS:
    void
*/
Datum li_log_info(PG_FUNCTION_ARGS)
{
	pid_t        pid         = getpid();       /* Process id of this process */
    char        *linkname    = NULL;     /* Filename the soft link points to */
	char         fileName[PATH_MAX];                         /* Log filename */
    ResultSet    resultSet;                     /* Result set for this query */

    /* Build a new result set. */
    CreateResultSet(&resultSet, fcinfo);	

	sprintf(fileName, PROC_FILENAME_TEMPLATE, pid);
	linkname = GetLinkedFilename(fileName);
	if (linkname)
		AddLogInfo(&resultSet, "real_logfile_name", linkname);

	AddLogInfoByName(&resultSet, "log_destination");
	AddLogInfoByName(&resultSet, "log_directory");
	AddLogInfoByName(&resultSet, "log_duration");
	AddLogInfoByName(&resultSet, "log_filename");
	AddLogInfoByName(&resultSet, "log_file_mode");
	AddLogInfoByName(&resultSet, "log_hostname");
	AddLogInfoByName(&resultSet, "log_line_prefix");
	AddLogInfoByName(&resultSet, "log_min_error_statement");
	AddLogInfoByName(&resultSet, "log_min_messages");
	AddLogInfoByName(&resultSet, "log_rotation_age");
	AddLogInfoByName(&resultSet, "log_rotation_size");
	AddLogInfoByName(&resultSet, "log_timezone");
	AddLogInfoByName(&resultSet, "log_truncate_on_rotation");
	AddLogInfoByName(&resultSet, "logging_collector");
	AddLogInfoByName(&resultSet, "syslog_facility");
	AddLogInfoByName(&resultSet, "syslog_ident");
	AddLogInfoByName(&resultSet, "syslog_sequence_numbers");
	AddLogInfoByName(&resultSet, "syslog_split_messages");

	/* Clean up and return the result set. */
    tuplestore_donestoring(resultSet.tupleStore);

	if (linkname)
		free(linkname);

    MemoryContextSwitchTo(resultSet.oldCtx);
    PG_RETURN_VOID();
}

/*------------------------------ li_ls_log_files -----------------------------*/
/*
  NAME:
    li_ls_log_files - Callback function for ls_log_files, ls_csv_files view

  PARAMETERS:
    type   - (IN) Type of log file: 'log' or 'csv'

  DESCRIPTION:
    This function creates a view to list the log files.

  NOTES:

  RETURNS:
    void
*/
Datum li_ls_log_files(PG_FUNCTION_ARGS)
{
	pid_t   pid        = getpid();             /* Process id of this process */
    char   *linkname   = NULL;           /* Filename the soft link points to */
	char   *type       = NULL;               /* Type of log file: log or csv */

	char        filename[PATH_MAX];                          /* Log filename */
    ResultSet   resultSet;                      /* Result set for this query */

	type = PG_GETARG_CSTRING(0);
    if (!type)
	{
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("type of log file must be specified")));
	}
	
    /* Build a new result set. */
    CreateResultSet(&resultSet, fcinfo);	

	sprintf(filename, PROC_FILENAME_TEMPLATE, pid);
	linkname = GetLinkedFilename(filename);

	if (linkname)
	{
		/* The link can point to either a log file or a pipe. If its is a log
         * log file, we can directly open it. In case of a pipe we need to find
         * out which log file it is. A pipe is used when logger process is
         * configured. Unfortunately postgres does not expose its pid, so we
         * we query the log file directory and search for the latest log file.
         */
		if (strncmp(linkname, "pipe:", 5) == 0)
		{
			/* Get log directory. */
			if (strlen(Log_directory) && Log_directory[0] == '/')
			{
				strcpy(filename, Log_directory);
			}
			else
			{
				sprintf(filename,
						"%s/%s",
						GetConfigOption("data_directory", false, false),
						Log_directory);
			}
			
			/* Open directory and read log files. */
			sprintf(filename, "%s/*.%s", filename, type);
			QueryDirectory(filename, &resultSet);
		} else {
			struct stat sb;                                 /* File metadata */

			if (lstat(linkname, &sb) != -1)
			{
				AddLogFile(&resultSet,
						   linkname,
						   ctime(&sb.st_ctime),
						   sb.st_size);
			}
		}

		free(linkname);
	}

	/* Clean up and return the result set. */
    tuplestore_donestoring(resultSet.tupleStore);

    MemoryContextSwitchTo(resultSet.oldCtx);
    PG_RETURN_VOID();
}

/*------------------------------ li_cat_log_file -----------------------------*/
/*
  NAME:
    li_cat_log_file - Callback function for cat_log_file, cat_csv_file view

  PARAMETERS:
    type   - (IN) Type of log file: 'log' or 'csv'

  DESCRIPTION:
    This function creates a view to list the contents of a log file.

  NOTES:

  RETURNS:
    void
*/
Datum li_cat_log_file(PG_FUNCTION_ARGS)
{
	pid_t        pid            = getpid();           /* Pid of this process */
    char        *linkname       = NULL;  /* Filename the soft link points to */
	char        *type           = NULL;      /* Type of log file: log or csv */
	char         filename[PATH_MAX];                         /* Log filename */
    ResultSet    resultSet;                     /* Result set for this query */

    /* Build a new result set. */
	type = PG_GETARG_CSTRING(0);
    if (!type)
	{
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("log file type must be specified")));
	}
	
    CreateResultSet(&resultSet, fcinfo);	

	/* Open log file and create records. */
	sprintf(filename, PROC_FILENAME_TEMPLATE, pid);

	linkname = GetLinkedFilename(filename);
	if (linkname)
	{
		if (strncmp(linkname, "pipe:", 5) == 0)
		{
			strcpy(filename, GetCurrentLogFilename(PG_GETARG_CSTRING(0)));
			LogFileContents(filename, type, &resultSet);
		}
		else
		{
			LogFileContents(filename, type, &resultSet);
		}

		free(linkname);
	}
	
	/* Clean up and return the result set. */
    tuplestore_donestoring(resultSet.tupleStore);

    MemoryContextSwitchTo(resultSet.oldCtx);
    PG_RETURN_VOID();
}

/*--------------------------- li_test_cat_log_file ---------------------------*/
/*
  NAME:
    li_test_cat_log_file - Test function for cat_log_file, cat_csv_file view

  PARAMETERS:
    filename   - (IN) Log filename
    type       - (IN) Type of log file: 'log' or 'csv'

  DESCRIPTION:
    This function creates a view to list the contents of a log file. It allows
    to define which log file to load.

  NOTES:

  RETURNS:
    void
*/
Datum li_test_cat_log_file(PG_FUNCTION_ARGS)
{
	char        *type           = NULL;      /* Type of log file: log or csv */
	char        *filename       = NULL;                      /* log filename */
    ResultSet    resultSet;                     /* Result set for this query */

	filename = PG_GETARG_CSTRING(0);
    if (!filename)
	{
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("log filename must be specified")));
	}

	type = PG_GETARG_CSTRING(1);
    if (!type)
	{
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("log file type must be specified")));
	}
	
    CreateResultSet(&resultSet, fcinfo);	
	LogFileContents(filename, type, &resultSet);

	/* Clean up and return the result set. */
    tuplestore_donestoring(resultSet.tupleStore);

    MemoryContextSwitchTo(resultSet.oldCtx);
    PG_RETURN_VOID();
}
