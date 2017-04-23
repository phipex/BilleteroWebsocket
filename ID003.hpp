

#ifndef ID003_HPP
#define ID003_HPP

#include <Thread.hpp>

class ID003: public Thread
{
    Q_OBJECT
public:
    enum Command {None, StatusRequest, Ack, Reset, Stack1, Stack2, Return, Hold, Wait, EnableAceptor, DisableAceptor};

    ID003(const QString& sPortPath, QObject * pParent = 0);

    static QString billfoldPortPath();
    int billInProcess();
    bool enableBillfold() const;
    void idcSendComand(const Command& eCommand);
    void readExisting();
    void setEnableBillfold(const bool& bEnableBillfold);
    static QString deviceFile(const QString& sFilter);

signals:
    /**
     * Señal emitida cuando se detecta un billete.
     *
     * @param iBillValue Valor del billete detectado.
     *
     */
    void billDetected(const int& iBillValue);

protected:
    Command processMessage(char * rxData);
    void run();

private:
    bool _bTicketInProcess;
    bool _bEnableBillfold;
    /**
     * File Descriptor del puerto.
     */
    int _iFDPort;
    /**
     * Último billete detectado. Si es menor que cero, no se ha detectado ningún billete.
     */
    int _iLastBill;
    /**
     * Rutal al puerto del billetero.
     */
    QString _sPortPath;
};

inline bool ID003::enableBillfold() const
{
    return _bEnableBillfold;
}

inline void ID003::setEnableBillfold(const bool& bEnableBillfold)
{
    _bEnableBillfold = bEnableBillfold;
}

#endif
