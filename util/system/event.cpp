#include "defaults.h"

#include "atomic.h"
#include "event.h"
#include "mutex.h"
#include "condvar.h"

#ifdef _win_
    #include "winint.h"
#endif

class TSystemEvent::TEvImpl: public TAtomicRefCount<TSystemEvent::TEvImpl> {
public:
#ifdef _win_
    inline TEvImpl(ResetMode rmode) {
        cond = CreateEvent(nullptr, rmode == rManual ? true : false, false, nullptr);
    }

    inline ~TEvImpl() {
        CloseHandle(cond);
    }

    inline void Reset() noexcept {
        ResetEvent(cond);
    }

    inline void Signal() noexcept {
        SetEvent(cond);
    }

    inline bool WaitD(TInstant deadLine) noexcept {
        if (deadLine == TInstant::Max()) {
            return WaitForSingleObject(cond, INFINITE) == WAIT_OBJECT_0;
        }

        const TInstant now = Now();

        if (now < deadLine) {
            //TODO
            return WaitForSingleObject(cond, (deadLine - now).MilliSeconds()) == WAIT_OBJECT_0;
        }

        return (WaitForSingleObject(cond, 0) == WAIT_OBJECT_0);
    }
#else
    inline TEvImpl(ResetMode rmode)
        : Manual(rmode == rManual ? true : false)
    {
    }

    inline void Signal() noexcept {
        if (Manual && AtomicGet(Signaled)) {
            return; // shortcut
        }

        with_lock (Mutex) {
            AtomicSet(Signaled, 1);
        }

        if (Manual) {
            Cond.BroadCast();
        } else {
            Cond.Signal();
        }
    }

    inline void Reset() noexcept {
        AtomicSet(Signaled, 0);
    }

    inline bool WaitD(TInstant deadLine) noexcept {
        if (Manual && AtomicGet(Signaled)) {
            return true; // shortcut
        }

        bool resSignaled = true;

        with_lock (Mutex) {
            while (!AtomicGet(Signaled)) {
                if (!Cond.WaitD(Mutex, deadLine)) {
                    resSignaled = AtomicGet(Signaled); // timed out, but Signaled could have been set

                    break;
                }
            }

            if (!Manual) {
                AtomicSet(Signaled, 0);
            }
        }

        return resSignaled;
    }
#endif

private:
#ifdef _win_
    HANDLE cond;
#else
    TCondVar Cond;
    TMutex Mutex;
    TAtomic Signaled = 0;
    bool Manual;
#endif
};

TSystemEvent::TSystemEvent(ResetMode rmode)
    : EvImpl_(new TEvImpl(rmode))
{
}

TSystemEvent::TSystemEvent(const TSystemEvent& other) noexcept
    : EvImpl_(other.EvImpl_)
{
}

TSystemEvent& TSystemEvent::operator=(const TSystemEvent& other) noexcept {
    EvImpl_ = other.EvImpl_;
    return *this;
}

TSystemEvent::~TSystemEvent() = default;

void TSystemEvent::Reset() noexcept {
    EvImpl_->Reset();
}

void TSystemEvent::Signal() noexcept {
    EvImpl_->Signal();
}

bool TSystemEvent::WaitD(TInstant deadLine) noexcept {
    return EvImpl_->WaitD(deadLine);
}
