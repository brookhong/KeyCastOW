/*
Based on "Simple C++ Timer Wrapper"
http://www.codeproject.com/Articles/146617/Simple-C-Timer-Wrapper
by ken.loveday

v1.0 2013 ArmyOfPirates
*/
static void CALLBACK TimerProc(void*, BOOLEAN);
static void CALLBACK TimerProcOnce(void* param, BOOLEAN timerCalled);

///////////////////////////////////////////////////////////////////////////////
//
// class CTimer
//
class CTimer
{
public:
    CTimer()
    {
        m_hTimer = NULL;
        m_mutexCount = 0;
    }

    virtual ~CTimer()
    {
        Stop();
    }

    bool Start(unsigned int interval,   // interval in ms
               bool immediately = false,// true to call first event immediately
               bool once = false)       // true to call timed event only once
    {
        if( m_hTimer )
        {
            Stop();
        }

        SetCount(0);

        BOOL success = CreateTimerQueueTimer( &m_hTimer,
                                              NULL,
                                              once ? TimerProcOnce : TimerProc,
                                              this,
                                              immediately ? 0 : interval,
                                              once ? 0 : interval,
                                              WT_EXECUTEINTIMERTHREAD);

        return( success != 0 );
    }

    void Stop()
    {
        DeleteTimerQueueTimer( NULL, m_hTimer, NULL );
        m_hTimer = NULL ;
    }

    void (*OnTimedEvent)();

    void SetCount(int value)
    {
        InterlockedExchange( &m_mutexCount, value );
    }

    int GetCount()
    {
        return InterlockedExchangeAdd( &m_mutexCount, 0 );
    }

    bool Enabled()
    {
        return m_hTimer != NULL;
    }

private:
    HANDLE m_hTimer;
    long m_mutexCount;
};

///////////////////////////////////////////////////////////////////////////////
//
// TimerProc
//
static void CALLBACK TimerProc(void* param, BOOLEAN timerCalled)
{
    CTimer* timer = static_cast<CTimer*>(param);
    timer->SetCount( timer->GetCount()+1 );
    timer->OnTimedEvent();
};


static void CALLBACK TimerProcOnce(void* param, BOOLEAN timerCalled)
{
    CTimer* timer = static_cast<CTimer*>(param);
    timer->SetCount( timer->GetCount()+1 );
    timer->OnTimedEvent();
    if( timer->Enabled() )
        timer->Stop();
};
