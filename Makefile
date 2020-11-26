# contrib/loginfo/Makefile

EXTENSION = loginfo              # Extension name
DATA      = loginfo--1.0.sql     # Script files to install
REGRESS   = loginfo_test         # Test script file (without extension)
MODULES   = loginfo              # C module name

# Postgres build pragmas
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
