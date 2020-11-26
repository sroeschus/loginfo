# Summary
Postgres extension to introspect postgres log files.

This postgres extension allows to get information about log files and the log file configuration. Postgres supports different ways to configure logging. This is defined with the postgres parameter log_destination. This extension provides additional database function and views. If the log_destination parameter has been set to syslog only the log_info view can be used. For all other values the log files and their contents can be queried and displayed.

# Query the log configuration
The log configuration can be queried with the database view loginfo.

```
steve=# select * from log_info;
           name           |            setting             
--------------------------+--------------------------------
 real_logfile_name        | pipe:[377926]
 log_destination          | stderr,csvlog
 log_directory            | log
 log_duration             | off
 log_filename             | postgresql-%Y-%m-%d_%H%M%S.log
 log_file_mode            | 384
 log_hostname             | off
 log_line_prefix          | %m [%p] 
 log_min_error_statement  | error
 log_min_messages         | warning
 log_rotation_age         | 1440
 log_rotation_size        | 10240
 log_timezone             | America/Los_Angeles
 log_truncate_on_rotation | off
 logging_collector        | on
 syslog_facility          | local0
 syslog_ident             | postgres
 syslog_sequence_numbers  | on
 syslog_split_messages    | on
(19 rows)
```

The row with the name real_logfile_name shows the actual log filename. If the logging_collector postgres parameter is set to on, then this is a pipe. If the logging collector is not turned on, it shows the name of the log file. The other rows show the values of postgres parameters that are relevant for logging.

# List log files
With the ls_log_files view the current list of logfiles can be displayed. This view does not take the value of the postgres parameter log_fiename into consideration; it only determines which files with the file extension '.log' are stored in the directory specified by the postgres parameter log_directory. The log_directory parameter can be an absolute or a relative path. In case it is a relative path, the directory is a subdirectory of the data_directory.

```
steve=# select * from ls_log_files;
                            filename                            |     last_update     | file_size 
----------------------------------------------------------------+---------------------+-----------
 /home/steve/postgres/data/log/postgresql-2020-11-15_141126.log | 2020.11.15 14:13:19 |       319
 /home/steve/postgres/data/log/postgresql-2020-11-15_141930.log | 2020.11.15 23:02:28 |    409754
 /home/steve/postgres/data/log/postgresql-2020-11-16_000000.log | 2020.11.16 16:58:37 |       875
 /home/steve/postgres/data/log/postgresql-2020-11-16_165839.log | 2020.11.16 23:09:09 |  10489744
 /home/steve/postgres/data/log/postgresql-2020-11-16_230909.log | 2020.11.16 23:46:20 |   2321248
 /home/steve/postgres/data/log/postgresql-2020-11-17_000000.log | 2020.11.19 19:45:54 |    269834
 /home/steve/postgres/data/log/postgresql-2020-11-18_000000.log | 2020.11.21 20:00:56 |     93037
 /home/steve/postgres/data/log/postgresql-2020-11-20_000000.log | 2020.11.26 10:06:53 |     47994
 /home/steve/postgres/data/log/postgresql-2020-11-22_000000.log | 2020.11.26 11:13:13 |  10485774
 /home/steve/postgres/data/log/postgresql-2020-11-26_111313.log | 2020.11.26 11:37:14 |  10485763
 /home/steve/postgres/data/log/postgresql-2020-11-26_113714.log | 2020.11.26 11:45:14 |  10491687
 /home/steve/postgres/data/log/postgresql-2020-11-26_114514.log | 2020.11.26 12:36:58 |   7643253
(12 rows)
```
It shows the log filenames, the time of the last modification of the log file and the log file size.

# List log files in CSV format
With the ls_csv_files view the current list of logfiles in CSV format can be displayed . This requires that postgres has been configured to create log files in the CSV format. Thi s is achieved by specifying log_destination = 'csvlog'. More than one value can be specified for the log_destination. 
This view does not take the value of the postgres parameter log_filename into consideration; it only determines which files with the file extension '.csv' are stored in the directory specified by the postgres parameter log_directory. The log_directory parameter can be an absolute or a relative path. In case it is a relative path, the directory is a subdirectory of the data_directory.

```
steve=# select * from ls_csv_files;
                            filename                            |     last_update     | file_size 
----------------------------------------------------------------+---------------------+-----------
 /home/steve/postgres/data/log/postgresql-2020-11-15_141126.csv | 2020.11.15 14:13:19 |       510
 /home/steve/postgres/data/log/postgresql-2020-11-15_141930.csv | 2020.11.15 22:34:34 |     14414
 /home/steve/postgres/data/log/postgresql-2020-11-16_000000.csv | 2020.11.16 16:58:37 |       729
 /home/steve/postgres/data/log/postgresql-2020-11-16_165839.csv | 2020.11.16 18:30:32 |     11002
 /home/steve/postgres/data/log/postgresql-2020-11-17_000000.csv | 2020.11.19 19:45:54 |     19454
 /home/steve/postgres/data/log/postgresql-2020-11-18_000000.csv | 2020.11.21 20:00:56 |     11543
 /home/steve/postgres/data/log/postgresql-2020-11-20_000000.csv | 2020.11.21 22:21:50 |     52616
 /home/steve/postgres/data/log/postgresql-2020-11-22_000000.csv | 2020.11.26 12:36:58 |    145063
(8 rows)
```
It shows the log filenames, the time of the last modification of the log file and the log file size.

# Display contents of current log file
With the view the contents of the current log file can be displayed.
```
steve=# select * from cat_log_file;
                                                line                                                 
-----------------------------------------------------------------------------------------------------
 2020-11-26 14:07:26.416 PST [160858] LOG:  database system was shut down at 2020-11-26 14:07:21 PST
 2020-11-26 14:07:26.424 PST [160856] LOG:  database system is ready to accept connections
(2 rows)
```
# Display contents of current log file in CSV format
With the view the contents of the current log file can be displayed in CSV format (this requires that logs in CSV format have been configured).
```
steve=# select log_time, message from cat_csv_file;
          log_time           |                         message                          
-----------------------------+----------------------------------------------------------
 2020-11-26 14:07:26.416 PST | database system was shut down at 2020-11-26 14:07:21 PST
 2020-11-26 14:07:26.424 PST | database system is ready to accept connections
(2 rows)
```
The above is only an example and selects from two fields of the view. The nice thing about the CSV format is that the individual fields are mapped to columns in the above view. The view consists of 23 fields.
```
steve=# \d cat_csv_file
                  View "public.cat_csv_file"
        Column         | Type | Collation | Nullable | Default 
-----------------------+------+-----------+----------+---------
 log_time              | text |           |          | 
 user_name             | text |           |          | 
 database_name         | text |           |          | 
 process_id            | text |           |          | 
 connection_from       | text |           |          | 
 session_id            | text |           |          | 
 session_line_num      | text |           |          | 
 command_tag           | text |           |          | 
 session_start_time    | text |           |          | 
 virtual_transacton_id | text |           |          | 
 transaction_id        | text |           |          | 
 error_severity        | text |           |          | 
 sql_state_code        | text |           |          | 
 message               | text |           |          | 
 detail                | text |           |          | 
 hint                  | text |           |          | 
 internal_query        | text |           |          | 
 internal_query_pos    | text |           |          | 
 context               | text |           |          | 
 query                 | text |           |          | 
 query_pos             | text |           |          | 
 location              | text |           |          | 
 application_name      | text |           |          | 
```
The individual columns are text fields. This avoids data conversion problems. The field values can be easily converted to postgres data types if required.

# Monitoring the postgres log
The view to display the contents of the CSV logs are very useful in monitoring the health of the system. The following SQL query only scratches the surface:
```
SELECT v.error_severity, COUNT(*)
  FROM cat_csv_log 
 GROUP BY error_severity;
 error_severity | count
----------------+-------
 WARNING        |     8
 LOG            |    38
 ERROR          |     5
(3 rows)
```

# Building the postgres extension
The easiest way to build the extension is to have a postgres source distribution. Go to the contrib directory and clone the repository there. Then switch to loginfo directory and build the extension. Afterwards it can be installed and tested. Obviously you don't want to do this on your production machine.

Building the postgres extension has the same requirements as building postgres itself.

```
cd postgres/contrib
git clone https://github.com/sroeschus/loginfo.git
cd loginfo
make clean
make
make install
make installcheck
```

# Installing the postgres extension
To install use the following commands:
```
psql
create extension loginfo;
```

# Validating the installation
After the postgres extension has been installed, the following views should be defined:
```steve=# \dv
          List of relations
 Schema |     Name     | Type | Owner 
--------+--------------+------+-------
 public | cat_csv_file | view | steve
 public | cat_log_file | view | steve
 public | log_info     | view | steve
 public | ls_csv_files | view | steve
 public | ls_log_files | view | steve
(5 rows)
```
In addition the following functions should be defined:
```
steve=# \df
                                    List of functions
 Schema |         Name         | Result data type |      Argument data types       | Type 
--------+----------------------+------------------+--------------------------------+------
 public | li_cat_log_file      | SETOF record     | extension cstring              | func
 public | li_log_info          | SETOF record     |                                | func
 public | li_ls_log_files      | SETOF record     | extension cstring              | func
 public | li_test_cat_log_file | SETOF record     | filename cstring, type cstring | func
(4 rows)
```

# Uninstalling the postgres extension
If the extension is no longer needed it can be de-installed with:
```
psql
drop extension loginfo;
```

# Configuring postgres to use CSV logs
If you want to configure postgres to write csv logs, the following change is required:
```
log_destination = 'csvlog'
```
After that the postgres instance needs to be restarted. Reloading the postgres configuration is not enough. The log_destination can have several values; so different log configurations can be combined:
```
log_destination = 'stderr,csvlog'
```
