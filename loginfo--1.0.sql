/* contrib/loginfo/loginfo--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION loginfo" to load this file. \quit

-- Test function to display log files.
--
--  Parameters:
--    filename : Filename to show log file for
--    type     : Type of logfile: 'LOG' or 'CSV'
--
CREATE FUNCTION li_test_cat_log_file(filename CSTRING, type CSTRING)
       RETURNS SETOF RECORD
       AS 'MODULE_PATHNAME', 'li_test_cat_log_file'
       LANGUAGE C STRICT;

-- Return contents of log file as a set of records
--
--  Parameters:
--    type     : Type of logfile: 'LOG' or 'CSV'
--
CREATE FUNCTION li_cat_log_file(extension CSTRING)
       RETURNS SETOF RECORD
       AS 'MODULE_PATHNAME', 'li_cat_log_file'
       LANGUAGE C STRICT;

-- Return contents of the log files in the log file directory as a set of
-- records. Log files can be log or csv files.
--
--  Parameters:
--    type     : Type of logfile: 'LOG' or 'CSV'
--
CREATE FUNCTION li_ls_log_files(extension CSTRING)
       RETURNS SETOF RECORD
       AS 'MODULE_PATHNAME', 'li_ls_log_files'
       LANGUAGE C STRICT;

-- Return log information as a set of records.
--
--  Parameters:
--    None
--
CREATE FUNCTION li_log_info()
       RETURNS SETOF RECORD
       AS 'MODULE_PATHNAME', 'li_log_info'
       LANGUAGE C STRICT;

-- cat_log_file returns the contents of the current log file.
--
CREATE VIEW cat_log_file AS
SELECT v.*
  FROM li_cat_log_file('log') v(line text);

-- cat_csv_file returns the contents of the current csv log file.
--
CREATE VIEW cat_csv_file AS
SELECT v.*
  FROM li_cat_log_file('csv') v(log_time              TEXT,
                                user_name             TEXT,
				database_name         TEXT,
				process_id            TEXT,
				connection_from       TEXT,
				session_id            TEXT,
				session_line_num      TEXT,
				command_tag           TEXT,
				session_start_time    TEXT,
				virtual_transacton_id TEXT,
				transaction_id        TEXT,
				error_severity        TEXT,
				sql_state_code        TEXT,
				message               TEXT,
				detail                TEXT,
				hint                  TEXT,
				internal_query        TEXT,
				internal_query_pos    TEXT,
				context               TEXT,
				query                 TEXT,
				query_pos             TEXT,
				location              TEXT,
				application_name      TEXT);

-- ls_log_files returns the name of log files for this instance.
--
CREATE VIEW ls_log_files AS
SELECT v.*
  FROM li_ls_log_files('log') v(filename text, last_update text, file_size INT8);

-- ls_csv_files returns the name of csv log files for this instance.
--
CREATE VIEW ls_csv_files AS
SELECT v.*
  FROM li_ls_log_files('csv') v(filename text, last_update text, file_size INT8);

-- log_inf0 returns log infornation and log configuration for this instance.
--
CREATE VIEW log_info AS
SELECT v.*
  FROM li_log_info() v(name text, setting text);

