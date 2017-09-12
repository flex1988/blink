#ifndef __STATUS_H__
#define __STATUS_H__
class Status {
public:
    Status() {}
    ~Status() {}
    enum Code {
        kOk = 0,
        kNotFound = 1,
        kCorruption = 2,
        kNotSupported = 3,
        kInvalidArgument = 4,
        kIOError = 5,
        kMergeInProgress = 6,
        kIncomplete = 7,
        kShutdownInProgress = 8,
        kTimedOut = 9,
        kAborted = 10,
        kBusy = 11,
        kExpired = 12,
        kTryAgain = 13
    };

    Code code() const { return _code; }
private:
    Code _code;
}
#endif
