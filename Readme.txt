#Run sqlite:
#sqlite3 [DBFileName]

#transform existing data and import into t1
#Run script in Toolbox.txt

#create tables
sqlite3 temp.db < 01_create_tables/sql_tables.txt

#init tables
sqlite3 temp.db < 02_init_tables/init.txt

#import all data !
sqlite3 temp.db < import_all_data.txt

#connect to DB
sqlite3 temp.db
