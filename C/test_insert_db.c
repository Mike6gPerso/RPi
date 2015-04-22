#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <sqlite3.h>

/*
        gcc -o test_str_sep test_str_sep.c
*/
char* create_insert_statement(char* input) {

        const char delimiters[]  =",:";
        char *token, *running;
        char *result;
        
        //running = (char *) malloc(strlen(input));
        result = (char *) malloc(sizeof(char*) * 30);
        strcat(result, "insert into data (");
        
        //token = (char *)malloc(sizeof(char*)/* * 50*/);
        running = strdup (input);                           /* Make writable copy.  */
        //strcpy(running, input);
        token = strsep (&running, delimiters);                  /* token = 1424034344 */
        strcat(result, token);
        strcat(result, ",");
        //printf("%s\n", token);
        token = strsep (&running, delimiters);                  /* token = 22 */
        //printf("%s\n", token);
        token = strsep (&running, delimiters);                  /* token = TS */
        //printf("%s\n", token);
        token = strsep (&running, delimiters);                  /* token = 11890 */
        //printf("%s\n", token);
        token = strsep (&running, delimiters);                  /* token = G */
        //printf("%s\n", token);
        token = strsep (&running, delimiters);                  /* token = 2 */
        //printf("%s\n", token);
        token = strsep (&running, delimiters);                  /* token = N */
        //printf("%s\n", token);
        token = strsep (&running, delimiters);                  /* token = 1 */
        strcat(result, token);
        strcat(result, ",");
        //printf("%s\n", token);
        token = strsep (&running, delimiters);                  /* token = H */
        //printf("%s\n", token);
        token = strsep (&running, delimiters);                  /* token = 6740 */
        strcat(result, token);
        strcat(result, ",");
        //printf("%s\n", token);
        token = strsep (&running, delimiters);                  /* token = T */
        //printf("%s\n", token);
        token = strsep (&running, delimiters);                  /* token = 1960 */
        strcat(result, token);
        strcat(result, ");");
        //printf("%s\n", token);
        token = strsep (&running, delimiters);                  /* token = NULL */
        
        free(running);
        free(token);
        
        return result;
}

char *create_insert_statement_v2(char *input){
        
        //fail safe:
        if(input == NULL){
                exit -2;
        }
        const char delimiters[]  =",:·";
        char *token, *result, *current;
        
        result = (char *) malloc(256);
        if( result == NULL){
                printf("Can not allocate memory...");
                exit -1;
        }
        //printf("size of input:%i\n", strlen(input));
        current = NULL;
        current = (char*) malloc(strlen(input)+1);
        if( current == NULL){
                printf("Can not allocate memory...");
                exit -1;
        }
        
        strcpy(current, input);
        strcat(result, "insert into data (");
        
        token = strtok(current, delimiters);
        
        strcat(result, token);
        strcat(result, ",");
        
        token = strtok (NULL, delimiters);                      /* token = 22 */
        token = strtok (NULL, delimiters);                      /* token = TS */
        token = strtok (NULL, delimiters);                      /* token = 11890 */
        token = strtok (NULL, delimiters);                      /* token = G */
        token = strtok (NULL, delimiters);                      /* token = 2 */
        token = strtok (NULL, delimiters);                      /* token = N */
        token = strtok (NULL, delimiters);                      /* token = 1 */
        strcat(result, token);
        strcat(result, ",");
        token = strtok (NULL, delimiters);                      /* token = H */
        token = strtok (NULL, delimiters);                      /* token = 6740 */
        strcat(result, token);
        strcat(result, ",");
        token = strtok (NULL, delimiters);                      /* token = T */
        token = strtok (NULL, delimiters);                      /* token = 1960 */
        
        strcat(result, token);
        strcat(result, ");");
        
//      free(token);
        free(current);
        return result;
}

char *create_insert_statement_v3(char *input){
        
        const char ins_base_statement[] ="insert into data (timestamp, sensor_ID, data, unit_ID) values (strftime('%s','now'),";
        //fail safe:
        if(input == NULL){
                exit -2;
        }
        const char delimiters[]  =",:·";
        char *token, *result, *current;
        char sensor_ID[4];
        result = (char *) malloc(sizeof(char*) * 500);
        if( result == NULL){
                printf("Can not allocate memory...");
                exit -1;
        }
        memset(result,'\0', sizeof(result));
        //printf("size of input:%i\n", strlen(input));
        current = NULL;
        current = (char*) malloc(strlen(input)+1);
        if( current == NULL){
                printf("Can not allocate memory...");
                exit -1;
        }
        
        strcpy(current, input);
        //printf("input: %s\n", input);
        //sprintf("insert into data(now(),");
        strcat(result, "BEGIN TRANSACTION;");
        strcat(result, ins_base_statement);
        
        token = strtok(current, delimiters);
        
        //token = strtok (NULL, delimiters);                    /* token = 22 */
        token = strtok (NULL, delimiters);                      /* token = TS */
        token = strtok (NULL, delimiters);                      /* token = 11890 */
        token = strtok (NULL, delimiters);                      /* token = G */
        token = strtok (NULL, delimiters);                      /* token = 2 */
        token = strtok (NULL, delimiters);                      /* token = N */
        token = strtok (NULL, delimiters);                      /* token = 1 */
        strcpy(sensor_ID, token);
        strcat(result, sensor_ID); // Sensor id
        strcat(result, ",");
        token = strtok (NULL, delimiters);                      /* token = H */
        token = strtok (NULL, delimiters);                      /* token = 6740 */
        strcat(result, token); //first data, Humidity ! unit_ID is number 2
        strcat(result, ", 2);");
        strcat(result, ins_base_statement);
        strcat(result, sensor_ID); // Sensor id
        strcat(result, ",");
        
        token = strtok (NULL, delimiters);                      /* token = T */
        token = strtok (NULL, delimiters);                      /* token = 1960 */
        
        strcat(result, token); //second data, Temperature ! unit_ID is number 1
        strcat(result, ", 1);");
        
        //if(token != NULL)
        //      free(token);
        strcat(result, "COMMIT;");

        free(current);
        return result;
}

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
   /*
   int i;
   for(i=0; i<argc; i++){
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   printf("\n");
   */
   return 0;
}

int insert_db(char *ins_query) {
        sqlite3 *db;
        char *zErrMsg = 0;
        int rc;
        
        
        rc = sqlite3_open("temp.db", &db);
        if( rc ){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return(1);
        }
        rc = sqlite3_exec(db, ins_query, callback, 0, &zErrMsg);
        if( rc!=SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
        }
        sqlite3_close(db);
        return 0;
}

int main(int argc, char *argv[])
{
        /*char line_example[] = "1424034344,22,TS:11890,G:2,N:1,H:6740,T:1960";*/
        char line_example[] = "3043,TS:4634405,G:2,N:1,H:5170,T:1840· ;";
        const char delimiters[]  =",:";
        char *statement;
        
        int iLoop = 0;
        
        //token = (char*) malloc(sizeof(char*));
        
        //running = strdup (line_example);  
        
        while(iLoop++ < 10) {
                statement = create_insert_statement_v3(line_example);
                //printf("%s\n", statement);
                insert_db(statement);
                free(statement);
        }
        
        /*
        BEGIN TRANSACTION;
        INSERT INTO DATA (timestamp, data, sensor_ID, unit_ID) VALUES (timestamp, data, sensor_ID, 1);
        INSERT INTO DATA (timestamp, data, sensor_ID, unit_ID) VALUES (timestamp, data, sensor_ID, 2);
        COMMIT;
        */      
}
