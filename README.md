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
