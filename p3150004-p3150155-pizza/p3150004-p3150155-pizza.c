#include "p3150004-p3150155-pizza.h"

int seed = 0;
pthread_mutex_t outputlock;
int *pizzafororder;

int availabletels = NTEL;
pthread_mutex_t availabletelslock;
pthread_cond_t availabletelscond;

int cookcurrentorder = 0;
pthread_mutex_t cookcurrentorderlock;

pthread_mutex_t cooklock;
pthread_cond_t cookcond;

int availableovens = 0;
pthread_mutex_t ovenlock;
pthread_cond_t ovencond;
pthread_cond_t **ovenscondvar;
pthread_mutex_t **ovenscondvarlock;
int *ovenstatus;

int *readytopack;
pthread_mutex_t packerlock;
pthread_cond_t packercond;

int cooksfinished = 0;
int packeterfinished = 0;

pthread_mutex_t deliverlock;
pthread_cond_t delivercond;

int *ordertimes;
int *waittimes;
int *coldtimes;
int *servetimes;

int starttime = 0;

int sum = 0;
int main(int argc, char *argv[])
{
    int i;
    int ncust = 0;
    //-----Program Argument Parsing
    if (argc != 5)
    {
        printf("Usage: %s -ncust <ncust> -seed <seed>\n\n", argv[0]);
        return -1;
    }
    for (i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-ncust") == 0)
            ncust = atoi(argv[i + 1]);
        if (strcmp(argv[i], "-seed") == 0)
            seed = atoi(argv[i + 1]);
    }
    //-------END Program Argument Parsing

    //Invalid state
    if (NOVEN < NORDERHIGH)
    {
        printf("Number of ovens can't be less than the maximum allowed number of pizzas per order. Exiting...\n");
        return -2;
    }

    //Printing program parameters
    printf("---------------Parameters START---------------\nncust: %d\nseed: %d\n", ncust, seed);
    printf("NTEL: %d\n", NTEL);
    printf("NCOOK: %d\n", NCOOK);
    printf("NOVEN: %d\n", NOVEN);
    printf("NDELIVERER: %d\n", NDELIVERER);
    printf("TORDERLOW: %d\n", TORDERLOW);
    printf("TORDERHIGH: %d\n", TORDERHIGH);
    printf("NORDERLOW: %d\n", NORDERLOW);
    printf("NORDERHIGH: %d\n", NORDERHIGH);
    printf("TPAYMENTLOW: %d\n", TPAYMENTLOW);
    printf("TPAYMENTHIGH: %d\n", TPAYMENTHIGH);
    printf("CPIZZA: %d\n", CPIZZA);
    printf("PFAIL: %.2f\n", PFAIL);
    printf("TPREP: %d\n", TPREP);
    printf("TBAKE: %d\n", TBAKE);
    printf("TPACK: %d\n", TPACK);
    printf("TDELLOW: %d\n", TDELLOW);
    printf("TDELHIGH: %d\n", TDELHIGH);
    printf("---------------Parameters END---------------\n");
    printf("\n\n-------------------START--------------------\n");

    //INITIALIZE Mutexes and Condition Variables
    if (pthread_mutex_init(&outputlock, NULL) != 0)
    {
        printf("\n mutex outputlock init has failed\n");
        return 1;
    }
    if (pthread_mutex_init(&availabletelslock, NULL) != 0)
    {
        printf("\n mutex availabletelslock init has failed\n");
        return 1;
    }
    if (pthread_cond_init(&availabletelscond, NULL) != 0)
    {
        printf("\n cond availabletelscond init has failed\n");
        return 1;
    }
    if (pthread_mutex_init(&cooklock, NULL) != 0)
    {
        printf("\n mutex cooklock init has failed\n");
        return 1;
    }
    if (pthread_cond_init(&cookcond, NULL) != 0)
    {
        printf("\n cond cookcond init has failed\n");
        return 1;
    }
    if (pthread_mutex_init(&cookcurrentorderlock, NULL) != 0)
    {
        printf("\n mutex cookcurrentorderlock init has failed\n");
        return 1;
    }
    if (pthread_cond_init(&ovencond, NULL) != 0)
    {
        printf("\n cond ovencond init has failed\n");
        return 1;
    }
    if (pthread_mutex_init(&ovenlock, NULL) != 0)
    {
        printf("\n mutex ovenlock init has failed\n");
        return 1;
    }
    if (pthread_cond_init(&packercond, NULL) != 0)
    {
        printf("\n cond packercond init has failed\n");
        return 1;
    }
    if (pthread_mutex_init(&packerlock, NULL) != 0)
    {
        printf("\n mutex packerlock init has failed\n");
        return 1;
    }
    if (pthread_cond_init(&delivercond, NULL) != 0)
    {
        printf("\n cond delivercond init has failed\n");
        return 1;
    }
    if (pthread_mutex_init(&deliverlock, NULL) != 0)
    {
        printf("\n mutex deliverlock init has failed\n");
        return 1;
    }

    //Allocated space dynamically for variables
    pthread_t **orders = malloc(ncust * sizeof(pthread_t *));
    int *orderno = malloc(ncust * sizeof(int));
    pthread_t **ovens = malloc(NOVEN * sizeof(pthread_t *));
    ovenscondvar = malloc(NOVEN * sizeof(pthread_cond_t *));
    ovenscondvarlock = malloc(NOVEN * sizeof(pthread_mutex_t *));
    int *ovenno = malloc(NOVEN * sizeof(int));

    ovenstatus = malloc(NOVEN * sizeof(int));
    readytopack = malloc(ncust * sizeof(int));

    pthread_t **cooks = malloc(NCOOK * sizeof(pthread_t));
    pthread_t *packeter = malloc(sizeof(pthread_t));
    pthread_t **deliverers = malloc(NDELIVERER * sizeof(pthread_t *));

    pizzafororder = malloc(ncust * sizeof(int));
    ordertimes = malloc(ncust * sizeof(int));
    waittimes = malloc(ncust * sizeof(int));
    coldtimes = malloc(ncust * sizeof(int));
    servetimes = malloc(ncust * sizeof(int));

    //Initialize arrays
    for (i = 0; i < ncust; i++)
    {
        pizzafororder[i] = 0;
    }
    for (i = 0; i < NOVEN; i++)
    {
        ovenstatus[i] = -2;
    }
    for (i = 0; i < ncust; i++)
    {
        readytopack[i] = 0;
    }

    //Initalize Threads
    pthread_create(packeter, NULL, &packeterrun, &ncust); //INITIALIZE PACKETER

    for (i = 0; i < NCOOK; i++) //INITIALIZE COOKS
    {
        cooks[i] = malloc(sizeof(pthread_t));
        pthread_create(cooks[i], NULL, &cookrun, &ncust);
    }

    for (i = 0; i < NDELIVERER; i++) //INITIALIZE DELIVERERS
    {
        deliverers[i] = malloc(sizeof(pthread_t));
        pthread_create(deliverers[i], NULL, &delivererrun, &ncust);
    }

    for (i = 0; i < NOVEN; i++) //INITIALIZE OVENS
    {
        ovens[i] = malloc(sizeof(pthread_t));
        ovenno[i] = i;
        ovenscondvar[i] = malloc(sizeof(pthread_cond_t));      //Condition variables for individual oven
        ovenscondvarlock[i] = malloc(sizeof(pthread_mutex_t)); //Mutex for individual oven
        if (pthread_cond_init(ovenscondvar[i], NULL) != 0)
        {
            printf("\n mutex init has failed\n");
            return 1;
        }
        if (pthread_mutex_init(ovenscondvarlock[i], NULL) != 0)
        {
            printf("\n mutex init has failed\n");
            return 1;
        }
        pthread_create(ovens[i], NULL, &ovenrun, &ovenno[i]);
    }
    starttime = currenttimeseconds();
    for (i = 0; i < ncust; i++) //INITIALIZE CUSTOMERS, each after a random interval
    {
        orderno[i] = i;
        orders[i] = malloc(sizeof(pthread_t));
        pthread_create(orders[i], NULL, &orderrun, &orderno[i]);
        sleep(randomint(TORDERLOW, TORDERHIGH));
    }

    for (i = 0; i < ncust; i++) //Wait for customer threads to finish
    {
        pthread_join(*orders[i], NULL);
        free(orders[i]);
    }

    //----------Case when no orders were placed-------
    int flagnonerun = 1;
    for (i = 0; i < ncust; i++)
    {
        if (pizzafororder[i] > 0)
        {
            flagnonerun = 0;
            break;
        }
    }
    if (flagnonerun)
    {
        for (i = 0; i < NCOOK; i++)
        {
            pthread_cond_signal(&cookcond);
        }

        for (i = 0; i < NOVEN; i++)
        {
            pthread_cond_signal(ovenscondvar[i]);
        }
        pthread_cond_signal(&packercond);

        for (i = 0; i < NDELIVERER; i++)
        {
            pthread_cond_signal(&delivercond);
        }
    }
    //-----------END Case----------------

    for (i = 0; i < NCOOK; i++) //Wait for Cook threads to finish
    {
        pthread_join(*cooks[i], NULL);
        free(cooks[i]);
    }
    for (i = 0; i < NOVEN; i++) //Wait for oven threads to finish
    {
        pthread_join(*ovens[i], NULL);
        free(ovens[i]);
        free(ovenscondvar[i]);
        free(ovenscondvarlock[i]);
    }
    pthread_join(*packeter, NULL); //Wait for packetere thread to finish

    for (i = 0; i < NDELIVERER; i++) //Wait for Deliverer threads to finish
    {
        pthread_join(*deliverers[i], NULL);
        free(deliverers[i]);
    }
    printf("--------------------END---------------------\n");

    //-----------Generating statistics------
    int success = 0, fail = 0, maxwait = 0, maxserve = 0, maxcold = 0, pizzasum = 0;
    double averagewait = 0.0, averageserve = 0.0, averagecold = 0.0;
    for (i = 0; i < ncust; i++)
    {
        // if (pizzafororder[i] > 0)
        // printf("ORDER [%05d][YES] {%d} pizzas, Wait: %d, Serve: %d, Cold:%d\n", i + 1, pizzafororder[i], waittimes[i], servetimes[i], coldtimes[i]);
        // else
        // printf("ORDER [%05d][NO]  Wait: %d\n", i + 1, waittimes[i]);

        // printf("c %d w %d s %d\n", coldtimes[i], waittimes[i], servetimes[i]);
        if (readytopack[i] == -2)
            success++;
        else
            fail++;

        if (pizzafororder[i] > 0)
            pizzasum += pizzafororder[i];
        averagewait += waittimes[i];
        averagecold += coldtimes[i];
        averageserve += servetimes[i];

        if (waittimes[i] > maxwait)
            maxwait = waittimes[i];
        if (servetimes[i] > maxserve)
            maxserve = servetimes[i];
        if (coldtimes[i] > maxcold)
            maxcold = coldtimes[i];
    }
    averagewait = (double)averagewait / (double)ncust;
    averagecold = (double)averagecold / (double)success;
    averageserve = (double)averageserve / (double)success;

    if (fail == ncust)
    {
        averagewait = averagecold = averageserve = 0;
    }
    // -----------End Statistics generation---------

    //Print statistics
    printf("\n\n-----------------STATISTICS-----------------\n");
    printf("Elapsed time:    %d minutes\n", (currenttimeseconds() - starttime));
    printf("Total Earnings:  %d EUR\n\n", sum);

    printf("Successful: <%d> orders\n", success);
    printf("Failed:     <%d> orders\n", fail);
    printf("Produced:   <%d> pizzas\n\n", pizzasum);

    printf("Average waiting time: <%.1f> minutes\n", averagewait);
    printf("Maximum waiting time: <%d> minutes\n", maxwait);
    printf("Average serving time: <%.1f> minutes\n", averageserve);
    printf("Maximum serving time: <%d> minutes\n", maxserve);
    printf("Average cold time:    <%.1f> minutes\n", averagecold);
    printf("Maximum cold time:    <%d> minutes\n", maxcold);
    printf("---------------STATISTICS END---------------\n");

    //free up dyanmically allocated memory
    free(pizzafororder);
    free(deliverers);
    free(ovens);
    free(cooks);
    free(packeter);
    free(readytopack);
    free(ovenstatus);
    free(ovenscondvar);
    free(ovenscondvarlock);
    free(orders);
    free(orderno);
    free(ovenno);
    free(ordertimes);
    free(waittimes);
    free(coldtimes);
    free(servetimes);

    //Destroy mutexes and condition variables that were initialized in the beginning of the program
    if (pthread_mutex_destroy(&outputlock) != 0)
    {
        printf("\n mutex outputlock destroy has failed\n");
        return 1;
    }
    if (pthread_mutex_destroy(&availabletelslock) != 0)
    {
        printf("\n mutex availabletelslock destroy has failed\n");
        return 1;
    }
    if (pthread_cond_destroy(&availabletelscond) != 0)
    {
        printf("\n cond availabletelscond destroy has failed\n");
        return 1;
    }
    if (pthread_mutex_destroy(&cooklock) != 0)
    {
        printf("\n mutex cooklock destroy has failed\n");
        return 1;
    }
    if (pthread_cond_destroy(&cookcond) != 0)
    {
        printf("\n cond cookcond destroy has failed\n");
        return 1;
    }
    if (pthread_mutex_destroy(&cookcurrentorderlock) != 0)
    {
        printf("\n mutex cookcurrentorderlock destroy has failed\n");
        return 1;
    }
    if (pthread_cond_destroy(&ovencond) != 0)
    {
        printf("\n cond ovencond destroy has failed\n");
        return 1;
    }
    if (pthread_mutex_destroy(&ovenlock) != 0)
    {
        printf("\n mutex ovenlock destroy has failed\n");
        return 1;
    }
    if (pthread_cond_destroy(&packercond) != 0)
    {
        printf("\n cond packercond destroy has failed\n");
        return 1;
    }
    if (pthread_mutex_destroy(&packerlock) != 0)
    {
        printf("\n mutex packerlock destroy has failed\n");
        return 1;
    }
    if (pthread_cond_destroy(&delivercond) != 0)
    {
        printf("\n cond delivercond destroy has failed\n");
        return 1;
    }
    if (pthread_mutex_destroy(&deliverlock) != 0)
    {
        printf("\n mutex deliverlock destroy has failed\n");
        return 1;
    }

    return 0;
}

int currenttimeseconds() //used to get timestamp during the program
{
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }
    return ts.tv_sec;
}

int randomint(int min, int max) //returns a random number by using the seed given by the user
{
    return (rand_r(&seed) % max) + min;
}

void output(char *out) //Thread safe sequential printing to terminal
{
    pthread_mutex_lock(&outputlock);
    printf("%s", out);
    pthread_mutex_unlock(&outputlock);
}

void *orderrun(void *ordno)
{
    int orderid = *(int *)ordno;
    servetimes[orderid] = waittimes[orderid] = currenttimeseconds(); //get starting time of customer thread
    char out[100];
    // sprintf(out, "Order: %d %d\n", orderid,currenttimeseconds());
    // output(out);
    // sprintf(out, "Order: %d\n", orderid);
    // output(out);
    pthread_mutex_lock(&availabletelslock);
    // sprintf(out, "Order: %d available: %d\n", orderid,availabletels);
    // output(out);
    while (availabletels == 0) //waiting for available telephone person to order
    {
        //     sprintf(out, "Order: %d will wait availability\n", orderid);
        // output(out);
        pthread_cond_wait(&availabletelscond, &availabletelslock);
    }
    availabletels--;
    // sprintf(out, "Order: %d %d\n", orderid, currenttimeseconds());
    // output(out);
    waittimes[orderid] = currenttimeseconds() - waittimes[orderid];
    pthread_mutex_unlock(&availabletelslock);

    // sprintf(out, "Order with orderid [%05d] sleeping\n", orderid);
    // output(out);
    sleep(randomint(TPAYMENTLOW, TPAYMENTHIGH)); //waiting for payment to be completed
    // sprintf(out, "Order with orderid [%05d] wokeup\n", orderid);
    // output(out);
    if (randomint(1, 100) <= (PFAIL * 100)) //chance for transaction to fail
    {
        sprintf(out, "Order [%05d] NOT placed\n", orderid + 1); //if transaction fails
        servetimes[orderid] = 0;
        coldtimes[orderid] = 0;
        output(out);
        pizzafororder[orderid] = -1;
        pthread_mutex_lock(&availabletelslock);
        availabletels++;
        pthread_cond_signal(&availabletelscond);
        pthread_mutex_unlock(&availabletelslock);
        pthread_cond_signal(&cookcond);

        return NULL;
    }

    pizzafororder[orderid] = randomint(NORDERLOW, NORDERHIGH); //random number of pizzas

    // sprintf(out, "Order with orderid [%05d] was placed for %d pizzas\n", orderid, pizzafororder[orderid]);
    sprintf(out, "Order [%05d] placed for {%d} pizzas\n", orderid + 1, pizzafororder[orderid]); //successful order placement
    output(out);
    ordertimes[orderid] = currenttimeseconds();
    sum += (CPIZZA * pizzafororder[orderid]); //add profits
    pthread_mutex_lock(&availabletelslock);
    availabletels++; //telephone person is now available
    pthread_cond_signal(&availabletelscond);
    // sprintf(out, "Order: %d avail at end:%d\n", orderid, availabletels);
    // output(out);
    pthread_mutex_unlock(&availabletelslock);
    pthread_cond_signal(&cookcond);
}

void *packeterrun(void *arg)
{
    int maxorders = *(int *)arg;
    // output("Packeter\n");
    int *packetqueue = malloc(maxorders * sizeof(int));
    int packetqueueindex = 0;
    char out[100];
    while (1)
    {
        // sprintf(out, "Packeter loop\n");
        // output(out);
        int i = 0, j = 0;
        packetqueueindex = 0;
        pthread_mutex_lock(&packerlock);
        for (i = 0; i < maxorders; i++) //get ready orders
        {

            if (readytopack[i] == pizzafororder[i] && readytopack[i] > 0)
            {
                packetqueue[packetqueueindex] = i;
                packetqueueindex++;
            }
        }
        if (packetqueueindex == 0) //no ready orders
        {
            int flag = 0;
            pthread_mutex_lock(&ovenlock);
            for (i = 0; i < NOVEN; i++)
            {
                if (ovenstatus[i] != -1)
                {
                    flag = 1;
                    break;
                }
            }
            pthread_mutex_unlock(&ovenlock);
            if (cooksfinished == NCOOK && flag == 0) //all orders finished
            {
                // for (i = 0; i < maxorders; i++)
                // {
                // sprintf(out, "Packeter order: %d \n", readytopack[i]);
                // output(out);
                // }
                // sprintf(out, "Packeter Exiting\n");
                // output(out);
                free(packetqueue);
                pthread_mutex_unlock(&packerlock);

                packeterfinished = 1;
                for (j = 0; j < NOVEN; j++)
                {
                    pthread_mutex_lock(ovenscondvarlock[j]);
                    pthread_cond_signal(ovenscondvar[j]);
                    pthread_mutex_unlock(ovenscondvarlock[j]);
                }
                break;
            }
            // sprintf(out, "Packeter will wait\n");
            // output(out);
            pthread_cond_wait(&packercond, &packerlock); //wait for orders to be ready for packing
            // sprintf(out, "Packeter resumed\n");
            // output(out);
            pthread_mutex_unlock(&packerlock);
            continue;
        }
        pthread_mutex_unlock(&packerlock);
        // sprintf(out, "Packeter acquired finished orders\n");
        // output(out);
        for (i = 0; i < packetqueueindex; i++) //handling packaging
        {
            // sprintf(out, "Packeter packing order [%05d]\n", packetqueue[i]);
            // output(out);
            sleep(TPACK); //packing time
            coldtimes[packetqueue[i]] = currenttimeseconds();
            sprintf(out, "Order [%05d] prepared in <%d> minutes\n", packetqueue[i] + 1, currenttimeseconds() - ordertimes[packetqueue[i]]);
            output(out);
            pthread_mutex_lock(&ovenlock);
            for (j = 0; j < NOVEN; j++) //take pizzas out of ovens and each oven becomes available
            {
                if (ovenstatus[j] == packetqueue[i])
                {
                    ovenstatus[j] = -1;
                    availableovens++;
                    readytopack[packetqueue[i]] = -1;
                }
            }
            pthread_cond_signal(&ovencond);
            pthread_mutex_unlock(&ovenlock);

            pthread_cond_signal(&delivercond);
        }
    }
}
void *ovenrun(void *arg)
{
    int localid = *(int *)arg;
    char out[100];
    // sprintf(out, "Oven %d\n", localid);
    // output(out);
    pthread_mutex_lock(&ovenlock);
    availableovens++;
    // sprintf(out,"Localid: %d, avail %d\n",localid,availableovens);
    // output(out);
    ovenstatus[localid] = -1;
    pthread_mutex_unlock(&ovenlock);
    while (1)
    {
        // sprintf(out, "Oven %d loop\n", localid);
        // output(out);

        pthread_mutex_lock(ovenscondvarlock[localid]);
        pthread_cond_wait(ovenscondvar[localid], ovenscondvarlock[localid]); //oven waits for pizzas to become available to bake
        pthread_mutex_unlock(ovenscondvarlock[localid]);

        pthread_mutex_lock(&ovenlock);
        if (ovenstatus[localid] < 0)
        {
            if (cooksfinished == NCOOK) //if no more pizzas are to be baked
            {
                // sprintf(out, "Oven %d EXITING\n", localid);
                // output(out);
                pthread_mutex_unlock(&ovenlock);
                break;
            }
            pthread_mutex_unlock(&ovenlock);
            continue;
        }
        pthread_mutex_unlock(&ovenlock);

        // sprintf(out, "Oven %d will bake order[%05d] 1 of %d pizzas \n", localid, ovenstatus[localid], pizzafororder[ovenstatus[localid]]);
        // output(out);
        sleep(TBAKE); //waiting to bake
        // sprintf(out, "Oven %d finished baking\n", localid);
        // output(out);
        pthread_mutex_lock(&packerlock);
        readytopack[ovenstatus[localid]] += 1;
        if (readytopack[ovenstatus[localid]] == pizzafororder[ovenstatus[localid]])
        {
            pthread_cond_signal(&packercond); //notify the packer to take pizzas out of oven
        }
        pthread_mutex_unlock(&packerlock);
    }
}
void *delivererrun(void *arg)
{
    // output("Deliverer\n");
    int maxorderno = *(int *)arg;
    int i = 0;
    char out[100];
    int deliverindex = -1;
    while (1)
    {
        deliverindex = -1;
        // sprintf(out, "Deliverer loop\n");
        // output(out);
        pthread_mutex_lock(&packerlock);
        for (i = 0; i < maxorderno; i++) //if there are available orders to be delivered, pick the first available
        {
            if (readytopack[i] == -1)
            {
                deliverindex = i;
                readytopack[i] = -2;
                break;
            }
        }
        pthread_mutex_unlock(&packerlock);
        if (deliverindex == -1)
        {
            if (packeterfinished) //no more orders to deliver
            {
                // sprintf(out, "Deliverer EXITING\n");
                // output(out);
                pthread_cond_signal(&delivercond);
                break;
            }
            pthread_mutex_lock(&deliverlock);
            pthread_cond_wait(&delivercond, &deliverlock);
            pthread_mutex_unlock(&deliverlock);
            continue;
        }
        // sprintf(out, "Deliverer delivering order [%05d] \n", deliverindex);
        // output(out);
        int slp = randomint(TDELLOW, TDELHIGH); //delivering order
        sleep(slp);
        sprintf(out, "Order [%05d] delivered in <%d> minutes\n", deliverindex + 1, currenttimeseconds() - ordertimes[deliverindex]);
        servetimes[deliverindex] = currenttimeseconds() - servetimes[deliverindex];
        coldtimes[deliverindex] = currenttimeseconds() - coldtimes[deliverindex];
        output(out);
        // sprintf(out, "Deliverer order [%05d]  delivered... Coming back\n", deliverindex);
        // output(out);
        sleep(slp); //returning to shop after delivery
        // sprintf(out, "Deliverer order [%05d]  returned to shop\n", deliverindex);
        // output(out);
    }
}

void *cookrun(void *arg)
{

    char out[100];

    int maxorderno = *(int *)arg;
    maxorderno--;
    // output("Cook\n");
    // sprintf(out, "Cook max order no %d\n", maxorderno);
    // output(out);

    int handlingorder;
    while (1)
    {
        pthread_mutex_lock(&cooklock);
        // sprintf(out, "Cook current order [%05d], %d\n", cookcurrentorder, maxorderno);
        // output(out);

        if (cookcurrentorder > maxorderno) //if orders are available to handle
        {
            pthread_cond_signal(&cookcond);
            // sprintf(out, "Cook EXITING\n");
            // output(out);
            cooksfinished++;
            pthread_mutex_unlock(&cooklock);
            break;
        }

        if (pizzafororder[cookcurrentorder] == 0) //order has not yet been placed
        {
            // sprintf(out, "Cook will sleep order [%05d], %d and pizza %d\n", cookcurrentorder, maxorderno, pizzafororder[cookcurrentorder]);
            // output(out);
            pthread_cond_wait(&cookcond, &cooklock);
            // sprintf(out, "Cook woke up order [%05d], %d and pizza %d\n", cookcurrentorder, maxorderno, pizzafororder[cookcurrentorder]);
            // output(out);
            pthread_mutex_unlock(&cooklock);

            continue;
        }

        if (pizzafororder[cookcurrentorder] == -1) //order was a declined transaction (payment)
        {
            cookcurrentorder++;
            pthread_mutex_unlock(&cooklock);
            continue;
        }
        handlingorder = cookcurrentorder;
        cookcurrentorder++;

        pthread_mutex_unlock(&cooklock);
        // sprintf(out, "Cook will sleep for %d from %d \n", TPREP * pizzafororder[handlingorder], handlingorder);
        // output(out);
        sleep(TPREP * pizzafororder[handlingorder]); //cook preparing the pizzas
        // sprintf(out, "Cook handling order [%05d]\n", handlingorder);
        // output(out);
        pthread_mutex_lock(&ovenlock);
        while (availableovens < pizzafororder[handlingorder]) //cook waiting for ovens to become available so that the pizzas will for each order will be baked at the same time
            pthread_cond_wait(&ovencond, &ovenlock);
        availableovens -= pizzafororder[handlingorder];
        int i = 0;
        int neededovens = pizzafororder[handlingorder];
        for (i = 0; i < NOVEN; i++) //allocated ovens for baking
        {
            if (ovenstatus[i] == -1)
            {
                ovenstatus[i] = handlingorder;
                neededovens--;
                pthread_cond_signal(ovenscondvar[i]); //signal each oven to start baking
                // sprintf(out, "Cook handling order [%05d] by giving it oven %d\n", handlingorder, i);
                // output(out);
            }
            if (neededovens == 0)
                break;
        }
        pthread_mutex_unlock(&ovenlock);
        //handle
    }
}