#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h> /* time_t, time, ctime */

#include <pigpio.h>
#include <sqlite3.h>
#include "linkedList.h"


/*
vw.c
2015-06-29 - MGD
Public Domain

gcc -g -o RF_Receiver RF_Receiver.c -lpigpio -lpthread -lrt -lsqlite3 linkedList.c
gcc -g -o vw2 vw2.c -lpthread -lrt -lsqlite3 linkedList.c
sudo ./RF_Receiver > log.txt 2>&1 &
*/

/*
This module provides a 313MHz/434MHz radio interface compatible
with the Virtual Wire library used on Arduinos.

Untested.
*/

#define DEBUG 0

#define MAX_GROUP_LEN 20

#define MAX_MESSAGE_BYTES 77 // Maximum message payload.

#define MIN_BPS 50    // Minimum bits per second.
#define MAX_BPS 10000 // Maximum bits per second.

#define MAX_RX_MESSAGES 100 // How many received messagas to buffer.

uint8_t _HEADER[]={0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x2a, 0x38, 0x2c};

#define _CTL 3 // Number of control bytes, length + CRC.


uint8_t _SYMBOL[]={
    0x0d, 0x0e, 0x13, 0x15, 0x16, 0x19, 0x1a, 0x1c,
    0x23, 0x25, 0x26, 0x29, 0x2a, 0x2c, 0x32, 0x34};

/*
                8    3    B
preamble 48-bit   010101 010101 010101 010101 010101 010101 0001 1100 1101
length   12-bit   S1 S2
message  12*n bit S1 S2 ... S(2n-1) S(2n)
CRC      24-bit   S1 S2 S3 S4

S are the 6-bit symbols for each nibble.
*/

int _sym2nibble(int symbol)
{
    int nibble;

    for (nibble=0; nibble<16; nibble++)
    {
        if (symbol == _SYMBOL[nibble]) return nibble;
    }
    return 0;
}

int _crc_ccitt_update(int crc, int data)
{
    data = data ^ (crc & 0xFF);

    data = (data ^ (data << 4)) & 0xFF;

    return (
    (((data << 8) & 0xFFFF) | (crc >> 8)) ^
    ((data >> 4) & 0x00FF) ^ ((data << 3) & 0xFFFF)
    );
}


/*
class tx():
*/

typedef struct
{
    int txgpio;
    int bps;
    int txbit;
    int mics;
    int wave_id;
    gpioPulse_t pulse[1024];
    int npulses;
} vw_tx_t;

vw_tx_t *tx_init(int txgpio, int bps)
{
    /*
Instantiate a transmitter with the Pi, the transmit gpio,
and the bits per second (bps).  The bps defaults to 2000.
The bps is constrained to be within MIN_BPS to MAX_BPS.
*/

    vw_tx_t *self;

    self = calloc(1, sizeof(vw_tx_t));

    if (!self) return NULL;

    self->txbit = (1<<txgpio);

    if      (bps < MIN_BPS) bps = MIN_BPS;
    else if (bps > MAX_BPS) bps = MAX_BPS;

    self->bps = bps;

    self->mics = 1000000 / bps;

    self->wave_id = -1;

    gpioSetMode(txgpio, PI_OUTPUT);

    return self;
}

void tx_cancel(vw_tx_t *self)
{
    /*
Cancels the wireless transmitter, aborting any message
in progress.
*/
    if (self)
    {
        if (self->wave_id >= 0)
        {
            gpioWaveTxStop();
            gpioWaveDelete(self->wave_id);
            gpioWaveAddNew();

            self->wave_id = -1;
        }
        self->npulses = 0;
    }
}

void tx_nibble(vw_tx_t *self, int nibble)
{
    int i;

    for (i=0; i<6; i++)
    {
        if (nibble & (1<<i))
        {
            self->pulse[self->npulses].gpioOn = self->txbit;
            self->pulse[self->npulses].gpioOff = 0;
        }
        else
        {
            self->pulse[self->npulses].gpioOn = 0;
            self->pulse[self->npulses].gpioOff = self->txbit;
        }
        self->pulse[self->npulses++].usDelay = self->mics;
    }
}

int tx_byte(vw_tx_t *self, int crc, uint8_t byte)
{
    tx_nibble(self, _SYMBOL[byte>>4]);
    tx_nibble(self, _SYMBOL[byte&0x0F]);

    return _crc_ccitt_update(crc, byte);
}

int tx_put(vw_tx_t *self, uint8_t *data, int count)
{
    /*
Transmit a message.  If the message is more than
MAX_MESSAGE_BYTES in size it is discarded.  If a message
is currently being transmitted it is aborted and replaced
with the new message.  True is returned if message
transmission has successfully started.  False indicates
an error.
*/

    int i, crc;

    if (count > MAX_MESSAGE_BYTES) return 0;

    tx_cancel(self);

    self->npulses = 0;

    gpioWaveAddNew();

    for (i=0; i<sizeof(_HEADER); i++) tx_nibble(self, _HEADER[i]);

    crc = tx_byte(self, 0xFFFF, count+_CTL);

    for (i=0; i<count; i++) crc = tx_byte(self, crc, data[i]);

    crc = ~crc;

    tx_byte(self, 0, crc&0xFF);
    tx_byte(self, 0, crc>>8);

    gpioWaveAddGeneric(self->npulses, self->pulse);

    self->wave_id = gpioWaveCreate();

    if (self->wave_id >= 0)
    {
        gpioWaveTxSend(self->wave_id, PI_WAVE_MODE_ONE_SHOT);
        return 1;
    }
    else return 0;
}

int tx_ready(void)
{
    /*
Returns True if a new message may be transmitted.
*/
    return !gpioWaveTxBusy();
}

/*
class rx():
*/

typedef struct
{
    int inited;
    int rxgpio;
    int bps;
    int bad_CRC;
    int mics;
    int min_mics;
    int max_mics;
    int timeout;
    uint32_t last_tick;
    int last_level;
    int good;
    int token;
    int in_message;
    int msgLen;
    int bitCnt;
    int byteCnt;
    int msgCnt;
    int msgReadPos;
    int msgWritePos;
    char *msgBuf[MAX_RX_MESSAGES];
    char message[MAX_MESSAGE_BYTES+_CTL];
} vw_rx_t;

void _cb(int gpio, int level, uint32_t tick, vw_rx_t *self);

vw_rx_t *rx_init(int rxgpio, int bps)
{
    /*
Instantiate a receiver with the Pi, the receive gpio, and
the bits per second (bps).  The bps defaults to 2000.
The bps is constrained to be within MIN_BPS to MAX_BPS.
*/

    vw_rx_t *self;

    float slack;
    int slack_mics;

    self = calloc(1, sizeof(vw_rx_t));

    if (!self) return NULL;

    self->rxgpio = rxgpio;

    self->bad_CRC = 0;

    if      (bps < MIN_BPS) bps = MIN_BPS;
    else if (bps > MAX_BPS) bps = MAX_BPS;

    self->bps = bps;

    slack = 0.20;
    self->mics = 1000000 / bps;
    slack_mics = slack * self->mics;
    self->min_mics = self->mics - slack_mics;       // Shortest legal edge.
    self->max_mics = (self->mics + slack_mics) * 4; // Longest legal edge.

    self->timeout =  8 * self->mics / 1000; // 8 bits time in ms.
    if (self->timeout < 8) self->timeout = 8;

    self->inited = 0;
    self->last_tick = 0;
    self->good = 0;
    self->bitCnt = 0;
    self->token = 0;
    self->in_message = 0;
    self->msgLen = 0;
    self->byteCnt = 0;
    self->msgCnt = 0;
    self->msgWritePos = 0;
    self->msgReadPos = 0;

    gpioSetMode(rxgpio, PI_INPUT);

    gpioSetAlertFuncEx(rxgpio, (gpioAlertFuncEx_t)_cb, self);

    return self;
}

int rx_calc_crc(vw_rx_t *self)
{
    int i, crc;

    crc = 0xFFFF;

    for (i=0; i<self->msgLen; i++)
    crc = _crc_ccitt_update(crc, self->message[i]);
    return crc;
}

void rx_insert(vw_rx_t *self, int bits, int level)
{
    int i, byte, crc;
    char *mptr;

    for (i=0; i<bits; i++)
    {
        self->token >>= 1;

        if (level == 0) self->token |= 0x800;

        if (self->in_message)
        {
            self->bitCnt++;

            if (self->bitCnt >= 12) // Complete token.
            {
                byte = (
                _sym2nibble(self->token & 0x3f) << 4 |
                _sym2nibble(self->token >> 6));

                if (self->byteCnt == 0)
                {
                    self->msgLen = byte;

                    if (byte > (MAX_MESSAGE_BYTES+_CTL))
                    {
                        self->in_message = 0; // Abort message.
                        return;
                    }
                }

                self->message[self->byteCnt] = byte;

                self->byteCnt++;
                self->bitCnt = 0;

                if (self->byteCnt >= self->msgLen)
                {
                    self->in_message = 0;
                    gpioSetWatchdog(self->rxgpio, 0);

                    crc = rx_calc_crc(self);

                    if (crc == 0xF0B8) // Valid CRC.
                    {
                        /* discard new message if buffer full */
                        if (self->msgCnt < MAX_RX_MESSAGES)
                        {
                            /* allocate space for message */
                            mptr = malloc(self->msgLen-2);
                            if (mptr)
                            {
                                memcpy(mptr, self->message, self->msgLen-2);
                                self->msgBuf[self->msgWritePos] = mptr;
                                self->msgWritePos++;
                                if (self->msgWritePos >= MAX_RX_MESSAGES)
                                self->msgWritePos = 0;
                                self->msgCnt++;
                            }
                        }
                    }
                    else self->bad_CRC ++;
                }
            }
        }
        else
        {
            if (self->token == 0xB38) // Start message token.
            {
                self->in_message = 1;
                gpioSetWatchdog(self->rxgpio, self->timeout);
                self->bitCnt = 0;
                self->byteCnt = 0;
            }
        }
    }
}

void _cb(int gpio, int level, uint32_t tick, vw_rx_t *self)
{
    int edge, bitlen, bits;

    if (self->inited)
    {
        if (level == PI_TIMEOUT)
        {
            gpioSetWatchdog(self->rxgpio, 0); // Switch watchdog off.

            if (self->in_message) rx_insert(self, 4, !self->last_level);

            self->good = 0;
            self->in_message = 0;
        }
        else
        {

            edge = tick - self->last_tick;

            if (edge < self->min_mics)
            {
                self->good = 0;
                self->in_message = 0;
            }
            else if (edge > self->max_mics)
            {
                if (self->in_message) rx_insert(self, 4, level);

                self->good = 0;
                self->in_message = 0;
            }
            else
            {
                self->good += 1;

                if (self->good > 8)
                {
                    bitlen = (100 * edge) / self->mics;

                    if      (bitlen < 140) bits = 1;
                    else if (bitlen < 240) bits = 2;
                    else if (bitlen < 340) bits = 3;
                    else                   bits = 4;

                    rx_insert(self, bits, level);
                }
            }
        }
    }

    self->last_tick = tick;
    self->last_level = level;
    self->inited = 1;
}


int rx_get(vw_rx_t *self, uint8_t *buf)
{
    /*
Returns the next unread message.

The next message is copied to buf and its length is
returned as the function value (0 if no message is
available.

buf should be large enough to hold the longest message
of MAX_MESSAGE_BYTES.
*/

    int msgLen;

    if (self->msgCnt)
    {
        msgLen = self->msgBuf[self->msgReadPos][0];
        if (buf) memcpy(buf, self->msgBuf[self->msgReadPos]+1, msgLen);

        /* free memory used for message */
        free(self->msgBuf[self->msgReadPos]);
        self->msgBuf[self->msgReadPos] = NULL;

        self->msgCnt--;
        self->msgReadPos++;
        if (self->msgReadPos >= MAX_RX_MESSAGES) self->msgReadPos = 0;
        return msgLen;
    }
    else return 0;
}

int rx_ready(vw_rx_t *self)
{
    /*
Returns count of messages available to be read.
*/
    return self->msgCnt;
}

void rx_pause(vw_rx_t *self)
{
    /*
Pauses the wireless receiver.
*/
    gpioSetAlertFuncEx(self->rxgpio, NULL, self);
    gpioSetWatchdog(self->rxgpio, 0);
}

void rx_resume(vw_rx_t *self)
{
    /*
Resumes the wireless receiver.
*/
    self->in_message = 0;
    gpioSetAlertFuncEx(self->rxgpio, (gpioAlertFuncEx_t)_cb, self);
}

void rx_cancel(vw_rx_t *self)
{
    /*
Cancels the wireless receiver.
*/
    gpioSetAlertFuncEx(self->rxgpio, NULL, self);
    gpioSetWatchdog(self->rxgpio, 0);
}
//////////////////////////////////////
char * DATA_DIR="data";
char * DB_FILENAME="temp.db";

pthread_mutex_t mutexDB = PTHREAD_MUTEX_INITIALIZER;

void touchFile(char* groupID, char* sensorID) {
 
    FILE *f = NULL;
    char *filename, *dirname;
    if(NULL == (dirname = (char *) malloc(sizeof(char *)*MAX_GROUP_LEN))){
        exit(1);
    }
    if(NULL == (filename = (char *) malloc(sizeof(char *)* (MAX_GROUP_LEN *2 + 1 + 4)))){ //dirname can be MAX_GROUP_LEN + / + filename can be MAX_GROUP_LEN + .txt
        exit(1);
    }
    sprintf(dirname, "%s/%s", DATA_DIR, groupID);
    sprintf(filename, "%s/%s/%s.txt", DATA_DIR, groupID, sensorID);
 
    mkdir(dirname); //create dir if does not exist
    f = fopen(filename, "w"); // update File
    fclose(f);
    f = NULL;
 
    free(filename);
    free(dirname);
}

char *create_insert_statement(char *input, char *group_ID, char *sensor_ID){
       
        const char ins_base_statement[] ="insert into data (timestamp, sensor_ID, group_ID, data, unit_ID) values (strftime('%s','now'),";
        //fail safe:
        if(input == NULL){
                exit -2;
        }
        const char delimiters[]  ="*#";
        char *token, *result, *current;
        int len;
        
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
        strcat(result, "BEGIN TRANSACTION;");
        strcat(result, ins_base_statement);
       
        //str: 9006;2;1;1960;6740
       
        token = strtok(current, delimiters);                    /* token = 9006 */
        token = strtok (NULL, delimiters);                      /* token = 2 */
        
        len = strlen(token);
        if(len > MAX_GROUP_LEN) {
            len = MAX_GROUP_LEN;
        }
        strncpy(group_ID, token, len);
        group_ID[len] = '\0';
        token = strtok (NULL, delimiters);                      /* token = 1 */
        
        len = strlen(token);
        if(len > MAX_GROUP_LEN) {
            len = MAX_GROUP_LEN;
        }
        strncpy(sensor_ID, token, len);
        sensor_ID[len] = '\0';
        
        strcat(result, sensor_ID); // Sensor id
        strcat(result, ",");
        strcat(result, group_ID); // Group id
        strcat(result, ",");
        
        token = strtok (NULL, delimiters);                      /* token = 1960 */
        strcat(result, token); //first data, Temperature ! unit_ID is number 1
        strcat(result, ", 1);");
        
        //second record
        strcat(result, ins_base_statement);
        strcat(result, sensor_ID); // Sensor id
        strcat(result, ",");
        strcat(result, group_ID); // Group id
        strcat(result, ",");

        token = strtok (NULL, delimiters);                      /* token = 6740 */
        strcat(result, token); //second data, Humidity ! unit_ID is number 2
        strcat(result, ", 2);");
       
	    token = strtok (NULL, delimiters);
        //if(token != NULL){
		//	free(token);
		//}
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
    //First acquire lock
    pthread_mutex_lock( &mutexDB );
    
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
   
    rc = sqlite3_open(DB_FILENAME, &db);
    if( rc ){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        //on error, release lock
        pthread_mutex_unlock( &mutexDB );
        return(1);
    }
    rc = sqlite3_exec(db, ins_query, callback, 0, &zErrMsg);
    if( rc!=SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    sqlite3_close(db);
    
    //Finally, release lock
    pthread_mutex_unlock( &mutexDB );
    
    return 0;
}
void * manageDataReceived (void *arg){
 
    char sensor_ID[MAX_GROUP_LEN];
    char group_ID[MAX_GROUP_LEN];
    char * str = (char *) arg;
    char * ins_st = create_insert_statement(str, &group_ID[0], &sensor_ID[0]);
    if (0 == insert_db(ins_st)){
        //Success
        touchFile(group_ID, sensor_ID);
    }
    if(DEBUG){
        printf("%s\n", str);
    }
    free(ins_st);
	pthread_exit(NULL);
}


#define RX 11
#define TX 25

#define BPS 2000

LinkedList head;

int main(int argc, char *argv[])
{
    vw_rx_t *rx;
    vw_tx_t *tx;

    char buf[128];
    char str[128];
    char fileName[30];
    int len, err = -1;
    pthread_t tid;
    
    int speed = BPS;
    if(argc > 1){
        speed = atoi(argv[1]);
    }
   
    time_t rawtime;

    time (&rawtime);
    if(DEBUG) {
        printf ("Starting at: %s", ctime (&rawtime));
    }

    if (gpioInitialise() < 0) return 1;

    rx = rx_init(RX, speed); // Specify rx gpio, and baud.
    tx = tx_init(TX, speed); // Specify tx gpio, and baud.
   
    if(DEBUG) {
        printf("Speed:%i\n", speed);
    }
   
    while(1){
        
        while (!rx_ready(rx)) time_sleep(0.05);
        while (rx_ready(rx))
        {
			//init buf
			memset(&buf, '\0', sizeof(buf));
            //Retrive message in buf
            len = rx_get(rx, buf);
            
            //Copy to str
			memset(&str, '\0', sizeof(str));
            strncpy(str, ((char *) &buf) + 4, len - 7);
            //str[len -3] = '\0';
            
			if(DEBUG) {
				printf("buf:%s\nstr:%s\nlen:%i\nstrlen:%i\n", ((char *) &buf) + 4, str, len, strlen(str));
			}
            //create a thread to manage the data
            err = pthread_create(&tid, NULL, manageDataReceived, str);
            if(err != 0){
                printf("Could not create thread: %i\n", err);
                printf("Data lost: %s\n", str);
            }
			pthread_detach(tid);
        }
    }
    rx_cancel(rx);
    tx_cancel(tx);

    gpioTerminate();
}


