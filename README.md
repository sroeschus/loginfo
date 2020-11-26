# Summary
Postgres extension to introspect postgres log files.

This postgres extension allows to get information about log files and the log file configuration. It provides additional database function and views.

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
