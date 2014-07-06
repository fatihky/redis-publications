/*
    RANDKEYSFROMSETP key count prefix
     Time complexity: O(N) where N is the set cardinality.
    
    Examples:
    127.0.0.1:6377> SADD online a b c d e
    > (integer) 5
    127.0.0.1:6377> MSET d:a a d:b b d:c c d:d d d:e e
    > OK
    127.0.0.1:6377> RANDKEYSFROMSETP online 3 d:
    1) "c"
    2) "a"
    3) "b"


    Adding to redis -------------------------------------------------------------

    copy this to redisCommandTable variable in redis.c
        {"randkeysfromsetp",randkeysfromsetpCommand,4,"r",0,NULL,1,1,1,0,0},

    copy this line to util.h if not exists at util.h after the '#include "sds.h"' line
        int robjArrayContains(robj **arr, long length, robj *elem);

    copy this line to "Commands prototypes" section or anywhere in redis.h
        void randkeysfrompCommand(redisClient *c);

    copy robjArrayContains function to util.c if not exists at util.c after the '#include "util.h"' line

    copy randkeysfromsetpCommand function to t_set.c if not exists at t_set.c after the '#include "redis.h"' line
*/

int robjArrayContains(robj **arr, long length, robj *elem)
{
    for(int i = 0; i < length; i++)
    {
        if(arr[i] == elem)
            return 1;
    }

    return 0;
}

// randkeysfromsetp key count prefix
void randkeysfromsetpCommand(redisClient *c) {
    robj *o, **array;
    long count, set_size;
    size_t prefix_len;

    if (getLongFromObjectOrReply(c, c->argv[2], &count, NULL) != REDIS_OK) return;

    o = lookupKeyWriteOrReply(c,c->argv[1],shared.emptymultibulk);
    if (o == NULL || checkType(c,o,REDIS_SET)) return;

    set_size = setTypeSize(o);

    if(set_size <= count)
        count = set_size;

    srand(time(NULL));

    array = zcalloc(sizeof(long *) * count);

    for (int i = 0; i < count; i++)
    {
        robj *elem;
        do { 
            setTypeRandomElement(o, &elem, NULL);
        } while(robjArrayContains(array, count, elem));
        array[i] = elem;
    }

    addReplyMultiBulkLen(c,count);

    prefix_len = stringObjectLen(c->argv[3]);

    // directly copied from mgetCommand() and heavily modified
    for (int i = 0; i < count; i++)
    {
        size_t  arg_len = stringObjectLen(array[i]);
        size_t  len     = arg_len + prefix_len;
        char    *ptr    = zmalloc(len);
        memcpy(ptr, c->argv[3]->ptr, prefix_len);
        memcpy(ptr + prefix_len, array[i]->ptr, arg_len);

        robj    *tmp    = createStringObject(ptr, len);

        // we are no longer use "o" variable as set objects's pointer
        //  so we use it as second temporary pointer in here
        o = lookupKeyRead(c->db,tmp);
        if (o == NULL) {
            addReply(c,shared.nullbulk);
        } else {
            if (o->type != REDIS_STRING) {
                addReply(c,shared.nullbulk);
            } else {
                addReplyBulk(c,o);
            }
        }
        freeStringObject(tmp);
    }
}
