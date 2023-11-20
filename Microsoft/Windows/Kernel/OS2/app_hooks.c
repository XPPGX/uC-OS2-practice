/*
*********************************************************************************************************
*                                            EXAMPLE CODE
*
*               This file is provided as an example on how to use Micrium products.
*
*               Please feel free to use any application code labeled as 'EXAMPLE CODE' in
*               your application products.  Example code may be used as is, in whole or in
*               part, or may be used as a reference only. This file can be modified as
*               required to meet the end-product requirements.
*
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*
*               You can find our product's user manual, API reference, release notes and
*               more information at https://doc.micrium.com.
*               You can contact us at www.micrium.com.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                              uC/OS-II
*                                          Application Hooks
*
* Filename : app_hooks.c
* Version  : V2.92.13
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  <os.h>


/*
*********************************************************************************************************
*                                      EXTERN  GLOBAL VARIABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            LOCAL TABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/



/*
*********************************************************************************************************
*********************************************************************************************************
**                                         GLOBAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/
void OutFileInit() {
    /*Clear the file*/
    if ((Output_err = fopen_s(&Output_fp, OUTPUT_FILE_NAME, "w")) == 0) {
        fclose(Output_fp);
    }
    else {
        printf("Error to clear output file");
    }
}

void InputFile() {
    /*
    * Read File
    * Task Information
    * Task_ID ArriveTime Execution Periodic
    */

    errno_t err;
    if ((err = fopen_s(&fp, INPUT_FILE_NAME, "r")) == 0) {
        printf("The file 'Taskset.txt' was opened\n");
    }
    else {
        printf("The file 'TaskSet.txt' was not opened\n");
    }
    char str[MAX];
    char* ptr;
    char* pTmp = NULL;
    int TaskInfo[INFO], i, j = 0;
    
    TASK_NUMBER = 0;
    
    while (!feof(fp)) {
        i = 0;
        memset(str, 0, sizeof(str));

        for (int k = 0; k < INFO; k++) {
            TaskInfo[k] = -1;
        }

        fgets(str, sizeof(str) - 1, fp);
        ptr = strtok_s(str, " ", &pTmp);
        while (ptr != NULL) {
            TaskInfo[i] = atoi(ptr);
            ptr = strtok_s(NULL, " ", &pTmp);
            if (i == 0) {
                TASK_NUMBER++;
                TaskParameter[j].TaskID = TASK_NUMBER;
            }
            else if (i == 1) {
                TaskParameter[j].TaskArriveTime = TaskInfo[i];
            }
            else if (i == 2) {
                TaskParameter[j].TaskExecutionTime = TaskInfo[i];
            }
            else if (i == 3) {
                TaskParameter[j].TaskPeriodic = TaskInfo[i];
            }
            i++;
        }
        /*Initial Priority*/
        /*M11102136*/
        TaskParameter[j].TaskPriority = j + 1;
        /*M11102136*/
        j++;
    }

    fclose(fp);
    /*read file*/

    TASK_NUMBER--;  //真正的Periodic Task數量
    j--;            //取得TaskParameter中關於CUS的Index
    printf("Total Periodic Task Number = %2d\n", TASK_NUMBER);

    //初始化CUS_INFO
    CUS_INFO = (CUS*)malloc(sizeof(CUS));
    memset(CUS_INFO, 0, sizeof(CUS));
    CUS_INFO->TaskID = TaskParameter[j].TaskID;
    CUS_INFO->ServerSizeInversed = 100 / (TaskParameter[j].TaskArriveTime); //取得倒數的ServerSize，後續可直接乘Aperiodic Job的Execution Time
    printf("CUS = {ID = %2d, ServerSizeInversed = %2d}\n", CUS_INFO->TaskID, CUS_INFO->ServerSizeInversed);
}

void InputFile_AperiodicTask() {
    errno_t err;
    if ((err = fopen_s(&fp, APERIODIC_FILE_NAME, "r")) == 0) {
        printf("The file 'Aperiodicjobs.txt' was opened\n");
    }
    else {
        printf("The file 'Aperiodicjobs.txt' was not opened\n");
    }
    
    char str[MAX];
    char* token;
    char* remainPart = NULL;
    
    int JobID           = -1;
    int ArriveTime      = -1;
    int ExecutionTime   = -1;
    int AbsDeadline     = -1;

    Aperiodic_TASK_HEAD = NULL;
    while (!feof(fp)) {
        memset(str, 0, sizeof(str));
        fgets(str, sizeof(str) - 1, fp);
     
        token = strtok_s(str , " ", &remainPart);       JobID           = atoi(token);
        token = strtok_s(NULL, " ", &remainPart);       ArriveTime      = atoi(token);
        token = strtok_s(NULL, " ", &remainPart);       ExecutionTime   = atoi(token);
        token = strtok_s(NULL, " ", &remainPart);       AbsDeadline     = atoi(token);
        

        //以ArriveTime去比較，決定Job先後順序，ArriveTime小的在前
        if (Aperiodic_TASK_HEAD == NULL) {
            Aperiodic_TASK_HEAD = (Aperiodic_Task_Info*)malloc(sizeof(Aperiodic_Task_Info));
            Aperiodic_TASK_HEAD->JobID              = JobID;
            Aperiodic_TASK_HEAD->ArriveTime         = ArriveTime;
            Aperiodic_TASK_HEAD->ExecutionTime      = ExecutionTime;
            Aperiodic_TASK_HEAD->AbsolutelyDeadline = AbsDeadline;
            Aperiodic_TASK_HEAD->Next               = NULL;
#ifdef InputFile_AperiodicTask_DEBUG
            printf("AperiodicTask.ID = %2d, is enqueued done [0]\n", Aperiodic_TASK_HEAD->JobID);
#endif
        }
        else {
            Aperiodic_Task_Info* New_AperiodicTask  = (Aperiodic_Task_Info*)malloc(sizeof(Aperiodic_Task_Info));
            New_AperiodicTask->JobID                = JobID;
            New_AperiodicTask->ArriveTime           = ArriveTime;
            New_AperiodicTask->ExecutionTime        = ExecutionTime;
            New_AperiodicTask->AbsolutelyDeadline   = AbsDeadline;
            New_AperiodicTask->Next                 = NULL;

            if (Aperiodic_TASK_HEAD->ArriveTime > New_AperiodicTask->ArriveTime) {          /*當Head->ArriveTime > New->ArriveTime*/
                New_AperiodicTask->Next = Aperiodic_TASK_HEAD;
                Aperiodic_TASK_HEAD = New_AperiodicTask;
#ifdef InputFile_AperiodicTask_DEBUG
                printf("AperiodicTask.ID = %2d, is enqueued done [1]\n", New_AperiodicTask->JobID);
#endif
            }
            else if (Aperiodic_TASK_HEAD->ArriveTime == New_AperiodicTask->ArriveTime) {    /*當Head->ArriveTime == New->ArriveTime*/
                if (Aperiodic_TASK_HEAD->JobID < New_AperiodicTask->JobID) {    //當Head->ID < New->ID
                    Aperiodic_Task_Info* IterPrev = Aperiodic_TASK_HEAD;
                    Aperiodic_Task_Info* Iter = Aperiodic_TASK_HEAD->Next;
                    //讓Iter走到New_Aperiodic該放入的地方
                    for (; (Iter->ArriveTime == New_AperiodicTask->ArriveTime) && (Iter->JobID < New_AperiodicTask->JobID) && (Iter != NULL); Iter = Iter->Next, IterPrev = IterPrev->Next);
                    New_AperiodicTask->Next = Iter;
                    IterPrev->Next = New_AperiodicTask;
#ifdef InputFile_AperiodicTask_DEBUG
                    printf("AperiodicTask.ID = %2d, is enqueued done [2]\n", New_AperiodicTask->JobID);
#endif
                }
                else {                                                          //當Head->ID > New->ID
                    New_AperiodicTask->Next = Aperiodic_TASK_HEAD;
                    Aperiodic_TASK_HEAD = New_AperiodicTask;
#ifdef InputFile_AperiodicTask_DEBUG
                    printf("AperiodicTask.ID = %2d, is enqueued done [3]\n", New_AperiodicTask->JobID);
#endif
                }
            }
            else {                                                                          /*當Head->ArriveTime < New->ArriveTime*/
                Aperiodic_Task_Info* IterPrev   = Aperiodic_TASK_HEAD;
                Aperiodic_Task_Info* Iter       = Aperiodic_TASK_HEAD->Next;
                for (; Iter != NULL; Iter = Iter->Next) {                                   
                    if (Iter->ArriveTime > New_AperiodicTask->ArriveTime) {     /*當Iter->ArriveTime > New->ArriveTime*/
                        New_AperiodicTask->Next = Iter;
                        IterPrev->Next          = New_AperiodicTask;
#ifdef InputFile_AperiodicTask_DEBUG
                        printf("AperiodicTask.ID = %2d, is enqueued done [4]\n", New_AperiodicTask->JobID);
#endif
                        break;
                    }
                    else if (Iter->ArriveTime == New_AperiodicTask) {           /*當Iter->ArriveTime == New->ArriveTime*/
                        if (Iter->JobID < New_AperiodicTask->JobID) {   //當Iter->ID < New->ID
                            New_AperiodicTask->Next = Iter->Next;
                            Iter->Next              = New_AperiodicTask;
#ifdef InputFile_AperiodicTask_DEBUG
                            printf("AperiodicTask.ID = %2d, is enqueued done [5]\n", New_AperiodicTask->JobID);
#endif
                            break;
                        }
                        else {                                          //當Iter->ID > New->ID
                            IterPrev->Next          = New_AperiodicTask;
                            New_AperiodicTask->Next = Iter;
#ifdef InputFile_AperiodicTask_DEBUG
                            printf("AperiodicTask.ID = %2d, is enqueued done [6]\n", New_AperiodicTask->JobID);
#endif
                            break;
                        }
                    }
                }
                //如果Iter走到NULL，New_AperiodicTask都沒有合適的位置可以放入，代表New_AperiodicTask要放在最後面
                if (Iter == NULL) {
                    IterPrev->Next = New_AperiodicTask;
#ifdef InputFile_AperiodicTask_DEBUG
                    printf("AperiodicTask.ID = %2d, is enqueued done [7]\n", New_AperiodicTask->JobID);
#endif
                }
            }
        }
#ifdef Aperiodic_ShowList_DEBUG
        for (Aperiodic_Task_Info* Iter = Aperiodic_TASK_HEAD; Iter != NULL; Iter = Iter->Next) {
            printf("Job[%2d] = {ArriveTime = %2d, ExecutionTime = %2d, AbsDeadline = %2d\n", Iter->JobID, Iter->ArriveTime, Iter->ExecutionTime, Iter->AbsolutelyDeadline);
        }
#endif
    }
}

/*
*********************************************************************************************************
*********************************************************************************************************
**                                        uC/OS-II APP HOOKS
*********************************************************************************************************
*********************************************************************************************************
*/

#if (OS_APP_HOOKS_EN > 0)

/*
*********************************************************************************************************
*                                  TASK CREATION HOOK (APPLICATION)
*
* Description : This function is called when a task is created.
*
* Argument(s) : ptcb   is a pointer to the task control block of the task being created.
*
* Note(s)     : (1) Interrupts are disabled during this call.
*********************************************************************************************************
*/

void  App_TaskCreateHook (OS_TCB *ptcb)
{
#if (APP_CFG_PROBE_OS_PLUGIN_EN == DEF_ENABLED) && (OS_PROBE_HOOKS_EN > 0)
    OSProbe_TaskCreateHook(ptcb);
#endif
}


/*
*********************************************************************************************************
*                                  TASK DELETION HOOK (APPLICATION)
*
* Description : This function is called when a task is deleted.
*
* Argument(s) : ptcb   is a pointer to the task control block of the task being deleted.
*
* Note(s)     : (1) Interrupts are disabled during this call.
*********************************************************************************************************
*/

void  App_TaskDelHook (OS_TCB *ptcb)
{
    (void)ptcb;
}


/*
*********************************************************************************************************
*                                    IDLE TASK HOOK (APPLICATION)
*
* Description : This function is called by OSTaskIdleHook(), which is called by the idle task.  This hook
*               has been added to allow you to do such things as STOP the CPU to conserve power.
*
* Argument(s) : none.
*
* Note(s)     : (1) Interrupts are enabled during this call.
*********************************************************************************************************
*/

#if OS_VERSION >= 251
void  App_TaskIdleHook (void)
{
}
#endif


/*
*********************************************************************************************************
*                                  STATISTIC TASK HOOK (APPLICATION)
*
* Description : This function is called by OSTaskStatHook(), which is called every second by uC/OS-II's
*               statistics task.  This allows your application to add functionality to the statistics task.
*
* Argument(s) : none.
*********************************************************************************************************
*/

void  App_TaskStatHook (void)
{
}


/*
*********************************************************************************************************
*                                   TASK RETURN HOOK (APPLICATION)
*
* Description: This function is called if a task accidentally returns.  In other words, a task should
*              either be an infinite loop or delete itself when done.
*
* Arguments  : ptcb      is a pointer to the task control block of the task that is returning.
*
* Note(s)    : none
*********************************************************************************************************
*/


#if OS_VERSION >= 289
void  App_TaskReturnHook (OS_TCB  *ptcb)
{
    (void)ptcb;
}
#endif


/*
*********************************************************************************************************
*                                   TASK SWITCH HOOK (APPLICATION)
*
* Description : This function is called when a task switch is performed.  This allows you to perform other
*               operations during a context switch.
*
* Argument(s) : none.
*
* Note(s)     : (1) Interrupts are disabled during this call.
*
*               (2) It is assumed that the global pointer 'OSTCBHighRdy' points to the TCB of the task that
*                   will be 'switched in' (i.e. the highest priority task) and, 'OSTCBCur' points to the
*                  task being switched out (i.e. the preempted task).
*********************************************************************************************************
*/

#if OS_TASK_SW_HOOK_EN > 0
void  App_TaskSwHook (void)
{
#if (APP_CFG_PROBE_OS_PLUGIN_EN > 0) && (OS_PROBE_HOOKS_EN > 0)
    OSProbe_TaskSwHook();
#endif
}
#endif


/*
*********************************************************************************************************
*                                   OS_TCBInit() HOOK (APPLICATION)
*
* Description : This function is called by OSTCBInitHook(), which is called by OS_TCBInit() after setting
*               up most of the TCB.
*
* Argument(s) : ptcb    is a pointer to the TCB of the task being created.
*
* Note(s)     : (1) Interrupts may or may not be ENABLED during this call.
*********************************************************************************************************
*/

#if OS_VERSION >= 204
void  App_TCBInitHook (OS_TCB *ptcb)
{
    (void)ptcb;
}
#endif


/*
*********************************************************************************************************
*                                       TICK HOOK (APPLICATION)
*
* Description : This function is called every tick.
*
* Argument(s) : none.
*
* Note(s)     : (1) Interrupts may or may not be ENABLED during this call.
*********************************************************************************************************
*/

#if OS_TIME_TICK_HOOK_EN > 0
void  App_TimeTickHook (void)
{
#if (APP_CFG_PROBE_OS_PLUGIN_EN == DEF_ENABLED) && (OS_PROBE_HOOKS_EN > 0)
    OSProbe_TickHook();
#endif
}
#endif
#endif
