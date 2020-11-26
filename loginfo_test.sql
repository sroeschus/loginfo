CREATE EXTENSION loginfo;

-- Check views report contents.
-- Warning: This assumes the current postgres instance is using log files
--          and has not syslog configured.
SELECT COUNT(*) > 0 FROM log_info;
SELECT COUNT(*) > 0 FROM ls_log_files;
SELECT COUNT(*) > 0 FROM cat_log_file;

-- Test standard log file.
\set cwd `pwd`
SELECT COUNT(*)
 FROM li_test_cat_log_file(concat(:'cwd', '/sql/log1.log')::cstring, 'log'::cstring) v(line text);

-- Test different fields of a CSV file.
SELECT COUNT(*)
 FROM li_test_cat_log_file(concat(:'cwd', '/sql/log1.csv')::cstring, 'csv'::cstring)
      v(log_time               TEXT,
        user_name              TEXT,
	database_name          TEXT,
	process_id             TEXT,
	connection_from        TEXT,
	session_id             TEXT,
	session_line_num       TEXT,
	command_tag            TEXT,
	session_start_time     TEXT,
	virtual_transaction_id TEXT,
	transaction_id         TEXT,
	error_severity         TEXT,
	sql_state_code         TEXT,
	message                TEXT,
	detail                 TEXT,
	hint                   TEXT,
	internal_query         TEXT,
	internal_query_pos     TEXT,
	context                TEXT,
	query                  TEXT,
	query_pos              TEXT,
	location               TEXT,
	application_name       TEXT);

SELECT v.error_severity, COUNT(*)
 FROM li_test_cat_log_file(concat(:'cwd', '/sql/log1.csv')::cstring, 'csv'::cstring)
      v(log_time               TEXT,
        user_name              TEXT,
	database_name          TEXT,
	process_id             TEXT,
	connection_from        TEXT,
	session_id             TEXT,
	session_line_num       TEXT,
	command_tag            TEXT,
	session_start_time     TEXT,
	virtual_transaction_id TEXT,
	transaction_id         TEXT,
	error_severity         TEXT,
	sql_state_code         TEXT,
	message                TEXT,
	detail                 TEXT,
	hint                   TEXT,
	internal_query         TEXT,
	internal_query_pos     TEXT,
	context                TEXT,
	query                  TEXT,
	query_pos              TEXT,
	location               TEXT,
	application_name       TEXT)
  GROUP BY error_severity;

SELECT log_time, user_name, database_name, process_id, connection_from
 FROM li_test_cat_log_file(concat(:'cwd', '/sql/log1.csv')::cstring, 'csv'::cstring)
      v(log_time               TEXT,
        user_name              TEXT,
	database_name          TEXT,
	process_id             TEXT,
	connection_from        TEXT,
	session_id             TEXT,
	session_line_num       TEXT,
	command_tag            TEXT,
	session_start_time     TEXT,
	virtual_transaction_id TEXT,
	transaction_id         TEXT,
	error_severity         TEXT,
	sql_state_code         TEXT,
	message                TEXT,
	detail                 TEXT,
	hint                   TEXT,
	internal_query         TEXT,
	internal_query_pos     TEXT,
	context                TEXT,
	query                  TEXT,
	query_pos              TEXT,
	location               TEXT,
	application_name       TEXT);

SELECT session_id, session_line_num, command_tag, session_start_time, virtual_transaction_id
 FROM li_test_cat_log_file(concat(:'cwd', '/sql/log1.csv')::cstring, 'csv'::cstring)
      v(log_time               TEXT,
        user_name              TEXT,
	database_name          TEXT,
	process_id             TEXT,
	connection_from        TEXT,
	session_id             TEXT,
	session_line_num       TEXT,
	command_tag            TEXT,
	session_start_time     TEXT,
	virtual_transaction_id TEXT,
	transaction_id         TEXT,
	error_severity         TEXT,
	sql_state_code         TEXT,
	message                TEXT,
	detail                 TEXT,
	hint                   TEXT,
	internal_query         TEXT,
	internal_query_pos     TEXT,
	context                TEXT,
	query                  TEXT,
	query_pos              TEXT,
	location               TEXT,
	application_name       TEXT);

SELECT transaction_id, error_severity, sql_state_code
 FROM li_test_cat_log_file(concat(:'cwd', '/sql/log1.csv')::cstring, 'csv'::cstring)
      v(log_time               TEXT,
        user_name              TEXT,
	database_name          TEXT,
	process_id             TEXT,
	connection_from        TEXT,
	session_id             TEXT,
	session_line_num       TEXT,
	command_tag            TEXT,
	session_start_time     TEXT,
	virtual_transaction_id TEXT,
	transaction_id         TEXT,
	error_severity         TEXT,
	sql_state_code         TEXT,
	message                TEXT,
	detail                 TEXT,
	hint                   TEXT,
	internal_query         TEXT,
	internal_query_pos     TEXT,
	context                TEXT,
	query                  TEXT,
	query_pos              TEXT,
	location               TEXT,
	application_name       TEXT);

SELECT v.message
 FROM li_test_cat_log_file(concat(:'cwd', '/sql/log1.csv')::cstring, 'csv'::cstring)
      v(log_time               TEXT,
        user_name              TEXT,
	database_name          TEXT,
	process_id             TEXT,
	connection_from        TEXT,
	session_id             TEXT,
	session_line_num       TEXT,
	command_tag            TEXT,
	session_start_time     TEXT,
	virtual_transaction_id TEXT,
	transaction_id         TEXT,
	error_severity         TEXT,
	sql_state_code         TEXT,
	message                TEXT,
	detail                 TEXT,
	hint                   TEXT,
	internal_query         TEXT,
	internal_query_pos     TEXT,
	context                TEXT,
	query                  TEXT,
	query_pos              TEXT,
	location               TEXT,
	application_name       TEXT);

SELECT detail
 FROM li_test_cat_log_file(concat(:'cwd', '/sql/log1.csv')::cstring, 'csv'::cstring)
      v(log_time               TEXT,
        user_name              TEXT,
	database_name          TEXT,
	process_id             TEXT,
	connection_from        TEXT,
	session_id             TEXT,
	session_line_num       TEXT,
	command_tag            TEXT,
	session_start_time     TEXT,
	virtual_transaction_id TEXT,
	transaction_id         TEXT,
	error_severity         TEXT,
	sql_state_code         TEXT,
	message                TEXT,
	detail                 TEXT,
	hint                   TEXT,
	internal_query         TEXT,
	internal_query_pos     TEXT,
	context                TEXT,
	query                  TEXT,
	query_pos              TEXT,
	location               TEXT,
	application_name       TEXT);

SELECT hint
 FROM li_test_cat_log_file(concat(:'cwd', '/sql/log1.csv')::cstring, 'csv'::cstring)
      v(log_time               TEXT,
        user_name              TEXT,
	database_name          TEXT,
	process_id             TEXT,
	connection_from        TEXT,
	session_id             TEXT,
	session_line_num       TEXT,
	command_tag            TEXT,
	session_start_time     TEXT,
	virtual_transaction_id TEXT,
	transaction_id         TEXT,
	error_severity         TEXT,
	sql_state_code         TEXT,
	message                TEXT,
	detail                 TEXT,
	hint                   TEXT,
	internal_query         TEXT,
	internal_query_pos     TEXT,
	context                TEXT,
	query                  TEXT,
	query_pos              TEXT,
	location               TEXT,
	application_name       TEXT);

SELECT internal_query, internal_query_pos, context, query, query_pos
 FROM li_test_cat_log_file(concat(:'cwd', '/sql/log1.csv')::cstring, 'csv'::cstring)
      v(log_time               TEXT,
        user_name              TEXT,
	database_name          TEXT,
	process_id             TEXT,
	connection_from        TEXT,
	session_id             TEXT,
	session_line_num       TEXT,
	command_tag            TEXT,
	session_start_time     TEXT,
	virtual_transaction_id TEXT,
	transaction_id         TEXT,
	error_severity         TEXT,
	sql_state_code         TEXT,
	message                TEXT,
	detail                 TEXT,
	hint                   TEXT,
	internal_query         TEXT,
	internal_query_pos     TEXT,
	context                TEXT,
	query                  TEXT,
	query_pos              TEXT,
	location               TEXT,
	application_name       TEXT);

SELECT location, application_name
 FROM li_test_cat_log_file(concat(:'cwd', '/sql/log1.csv')::cstring, 'csv'::cstring)
      v(log_time               TEXT,
        user_name              TEXT,
	database_name          TEXT,
	process_id             TEXT,
	connection_from        TEXT,
	session_id             TEXT,
	session_line_num       TEXT,
	command_tag            TEXT,
	session_start_time     TEXT,
	virtual_transaction_id TEXT,
	transaction_id         TEXT,
	error_severity         TEXT,
	sql_state_code         TEXT,
	message                TEXT,
	detail                 TEXT,
	hint                   TEXT,
	internal_query         TEXT,
	internal_query_pos     TEXT,
	context                TEXT,
	query                  TEXT,
	query_pos              TEXT,
	location               TEXT,
	application_name       TEXT);

-- Error cases
--

-- Invalid filename for log file.
SELECT COUNT(*)
 FROM li_test_cat_log_file('/sql/log1.log'::cstring, 'log'::cstring) v(line text);

-- Invalid filename for csv file.
SELECT v.message
 FROM li_test_cat_log_file('/sql/log1.csv'::cstring, 'csv'::cstring)
      v(log_time               TEXT,
        user_name              TEXT,
	database_name          TEXT,
	process_id             TEXT,
	connection_from        TEXT,
	session_id             TEXT,
	session_line_num       TEXT,
	command_tag            TEXT,
	session_start_time     TEXT,
	virtual_transaction_id TEXT,
	transaction_id         TEXT,
	error_severity         TEXT,
	sql_state_code         TEXT,
	message                TEXT,
	detail                 TEXT,
	hint                   TEXT,
	internal_query         TEXT,
	internal_query_pos     TEXT,
	context                TEXT,
	query                  TEXT,
	query_pos              TEXT,
	location               TEXT,
	application_name       TEXT);

-- Try load log file as CSV file.
SELECT v.message
 FROM li_test_cat_log_file(concat(:'cwd', '/sql/log1.log')::cstring, 'csv'::cstring)
      v(log_time               TEXT,
        user_name              TEXT,
	database_name          TEXT,
	process_id             TEXT,
	connection_from        TEXT,
	session_id             TEXT,
	session_line_num       TEXT,
	command_tag            TEXT,
	session_start_time     TEXT,
	virtual_transaction_id TEXT,
	transaction_id         TEXT,
	error_severity         TEXT,
	sql_state_code         TEXT,
	message                TEXT,
	detail                 TEXT,
	hint                   TEXT,
	internal_query         TEXT,
	internal_query_pos     TEXT,
	context                TEXT,
	query                  TEXT,
	query_pos              TEXT,
	location               TEXT,
	application_name       TEXT);

-- Too many fields.
SELECT v.message
 FROM li_test_cat_log_file(concat(:'cwd', '/sql/too-many-fields.csv')::cstring, 'csv'::cstring)
      v(log_time               TEXT,
        user_name              TEXT,
	database_name          TEXT,
	process_id             TEXT,
	connection_from        TEXT,
	session_id             TEXT,
	session_line_num       TEXT,
	command_tag            TEXT,
	session_start_time     TEXT,
	virtual_transaction_id TEXT,
	transaction_id         TEXT,
	error_severity         TEXT,
	sql_state_code         TEXT,
	message                TEXT,
	detail                 TEXT,
	hint                   TEXT,
	internal_query         TEXT,
	internal_query_pos     TEXT,
	context                TEXT,
	query                  TEXT,
	query_pos              TEXT,
	location               TEXT,
	application_name       TEXT);

-- Not enough fields.
SELECT v.message
 FROM li_test_cat_log_file(concat(:'cwd', '/sql/not-enough-fields.csv')::cstring, 'csv'::cstring)
      v(log_time               TEXT,
        user_name              TEXT,
	database_name          TEXT,
	process_id             TEXT,
	connection_from        TEXT,
	session_id             TEXT,
	session_line_num       TEXT,
	command_tag            TEXT,
	session_start_time     TEXT,
	virtual_transaction_id TEXT,
	transaction_id         TEXT,
	error_severity         TEXT,
	sql_state_code         TEXT,
	message                TEXT,
	detail                 TEXT,
	hint                   TEXT,
	internal_query         TEXT,
	internal_query_pos     TEXT,
	context                TEXT,
	query                  TEXT,
	query_pos              TEXT,
	location               TEXT,
	application_name       TEXT);

-- Eof in record.
SELECT v.message
 FROM li_test_cat_log_file(concat(:'cwd', '/sql/eof-in-record.csv')::cstring, 'csv'::cstring)
      v(log_time               TEXT,
        user_name              TEXT,
	database_name          TEXT,
	process_id             TEXT,
	connection_from        TEXT,
	session_id             TEXT,
	session_line_num       TEXT,
	command_tag            TEXT,
	session_start_time     TEXT,
	virtual_transaction_id TEXT,
	transaction_id         TEXT,
	error_severity         TEXT,
	sql_state_code         TEXT,
	message                TEXT,
	detail                 TEXT,
	hint                   TEXT,
	internal_query         TEXT,
	internal_query_pos     TEXT,
	context                TEXT,
	query                  TEXT,
	query_pos              TEXT,
	location               TEXT,
	application_name       TEXT);
