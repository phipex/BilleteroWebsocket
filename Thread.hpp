

#ifndef THREAD_HPP
#define THREAD_HPP

// Qt
#include <QObject>

class Thread: public QObject
{
    Q_OBJECT
public:
    enum Status { Running, Stopped };

    Thread(QObject * parent = 0);
    ~Thread();

    void start();
    Status status() const;
    void stop();

protected:
    virtual void run() = 0;

private:
    Status _status;
    pthread_t * _pThread;

    static void* run(void* args);
    void threadStopped();
};

inline Thread::Status Thread::status() const
{
    return _status;
}

#endif
