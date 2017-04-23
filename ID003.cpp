

// Own
#include <ID003.hpp>
//#include <Global.hpp>

// old
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <QDir>
#include <QStringList>

int billValue[10] = { 1000, 2000, 5000, 10000, 20000, 50000, 0, 0, 0, 0 };                         // Valor del billete en pesos

ID003::ID003(const QString& sPortPath, QObject * pParent):
    Thread(pParent)
{
    _bTicketInProcess = false;
    _bEnableBillfold = true;
    _iFDPort = -1;
    _iLastBill = -1;
    _sPortPath = sPortPath;
}

QString ID003::billfoldPortPath()
{
    return deviceFile("ttyBill*");
}

int ID003::billInProcess()
{
    if (_iLastBill >= 0)
    {
        int iValorBillete = billValue[_iLastBill];

        // dinero += valor_billete;
        // sas_0x0c_credits += valor_billete;
        // credito = dinero/valor_credito;
        // residuo = dinero - (credito * valor_credito);
        // sas_0x40_bills_accepted[bill_code[ultimo_billete]-0x40]++;
        return iValorBillete;
    }
    else
        return 0;
}

void ID003::idcSendComand(const Command& eCommand)
{
    if (_iFDPort >= 0)
    {
        unsigned char out_buffer[8];
        unsigned char c = 5;
        out_buffer[0] = 0xfc;
        out_buffer[1] = 0x05;

        switch (eCommand)
        {
        case StatusRequest:
            out_buffer[2] = 0x11;
            out_buffer[3] = 0x27;
            out_buffer[4] = 0x56;
            break;
        case Ack:
            out_buffer[2] = 0x50;
            out_buffer[3] = 0xaa;
            out_buffer[4] = 0x05;
            break;
        case Reset:
            out_buffer[2] = 0x40;
            out_buffer[3] = 0x2b;
            out_buffer[4] = 0x15;
            break;
        case Stack1:
            out_buffer[2] = 0x41;
            out_buffer[3] = 0xa2;
            out_buffer[4] = 0x04;
            break;
        case Stack2:
            out_buffer[2] = 0x42;
            out_buffer[3] = 0x39;
            out_buffer[4] = 0x36;
            break;
        case Return:
            out_buffer[2] = 0x43;
            out_buffer[3] = 0xb0;
            out_buffer[4] = 0x27;
            break;
        case Hold:
            out_buffer[2] = 0x44;
            out_buffer[3] = 0x0f;
            out_buffer[4] = 0x53;
            break;
        case Wait:
            out_buffer[2] = 0x45;
            out_buffer[3] = 0x86;
            out_buffer[4] = 0x42;
            break;
        case EnableAceptor:
            out_buffer[1] = 0x06;
            out_buffer[2] = 0xc3;
            out_buffer[3] = 0x00;
            out_buffer[4] = 0x04;
            out_buffer[5] = 0xd6;
            c = 6;
            break;
        case DisableAceptor:
            out_buffer[1] = 0x06;
            out_buffer[2] = 0xc3;
            out_buffer[3] = 0x01;
            out_buffer[4] = 0x8d;
            out_buffer[5] = 0xc7;
            c = 6;
            break;
        default:
            break;
        }
        write(_iFDPort, out_buffer, c);
    }
}

void ID003::readExisting()
{
    char a[16];
    while (true)
    {
        if (read(_iFDPort, a, 16) < 0)
            return;
    }
}

ID003::Command ID003::processMessage(char * rxData)
{
    Command response = None;

    switch (rxData[0])
    {
    case 0x11:                                              // Enabled idling
        if (_bEnableBillfold == false)
            idcSendComand(response = DisableAceptor);       // Envia comando deshabilitar billetero
        break;

    case 0x12:                                              // Accepting
        break;

    case 0x13:                                              // Escrow
        if (rxData[1] > 0x60 && rxData[1] < 0x69)
        {
            _iLastBill = rxData[1] - 0x61;
            _bTicketInProcess = true;
            idcSendComand(response = Stack1);               // Envia comando STACK 1
        }
        else
        {
            _bTicketInProcess = false;
            idcSendComand(response = Return);               // Envia comando RETURN
        }
        break;

    case 0x14:                                              // Stacking
        break;

    case 0x15:                                              // Vend Valid
        if (_bTicketInProcess == true)
            idcSendComand(response = Ack);                  // Envia comando ACK
        break;

    case 0x16:                                              // STACKED
        if (_bTicketInProcess == true)
        {
            int iBill = billInProcess();
            _iLastBill = -1;
            _bTicketInProcess = false;

            if (iBill > 0)
                emit billDetected(iBill);
            // std::cout << "Billete recibido con codigo: " << ultimo_billete << "\nCreditos: " << sas_0x0c_credits << "\nDinero: " << dinero << std::endl;
        }
        break;

    case 0x1a:                                              // DISABLE
        if (_bEnableBillfold == true)
            idcSendComand(response = EnableAceptor);        // Envia comando de habilitar billetero
        break;

    case 0x40:                                              // POWER UP
    case 0x41:                                              // POWER UP BILL ACCEPT
    case 0x42:                                              // POWER UP BILL STACK
        idcSendComand(response = Reset);                    // Envia comando de reset
        break;
    }

    return response;
}

void ID003::run()
{
    // Abre el puerto
    _iFDPort = open(_sPortPath.toStdString().c_str(), O_RDWR | O_NOCTTY);

    if (_iFDPort < 0)
    {
        //std::perror("Open port: unable open serial port.");
        stop();
    }
    else
    {
        struct termios options;
        char szInBuffer[32];

        fcntl(_iFDPort, F_SETFL, FNDELAY);

        // Lee la configuracion actual
        bzero(&options, sizeof(options));
        tcgetattr(_iFDPort, &options);

        options.c_cflag = B9600 | CS8 | CLOCAL | CREAD | PARENB ;
        options.c_cflag &= ~PARODD;
        options.c_iflag = IGNPAR | IGNBRK;
        options.c_oflag = 0;

        // Deshabilta Flow Control
        options.c_cflag &= ~ CRTSCTS;
        // RAW input
        options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        // RAW output
        // ts_port_options.c_oflag &= ~OPOST;

        tcsetattr(_iFDPort, TCSANOW, &options);
        fcntl(_iFDPort, F_SETFL, O_NONBLOCK);

        unsigned char iRxAux = 0;
        unsigned char iRxEstado = 0;
        char szRxData[16];
        unsigned char iRxNumbytes = 0;

        while (status() == Running)
        {
            unsigned char x = 0, y;
            int readedBytes = read(_iFDPort, szInBuffer, 32);

            if (readedBytes <= 0)
                idcSendComand(StatusRequest);
            else
            {
                while (readedBytes > 0)
                {
                    y = szInBuffer[x];

                    switch (iRxEstado)
                    {
                    case 0:     // Esperando Sync 0x0f
                        if (y == 0xfc)
                            iRxEstado = 5;
                        else
                        {
                            readExisting();
                            readedBytes = 0;
                        }
                        break;
                    case 5:     // Toma la cantidad de datos del mensaje
                        iRxNumbytes = y;
                        iRxNumbytes -= 2;
                        iRxAux = 0;
                        iRxEstado = 10;
                        break;
                    case 10:    // Lee el mensaje
                        szRxData[iRxAux] = y;
                        iRxAux++;
                        iRxNumbytes--;

                        if (iRxNumbytes == 0)
                        {
                            Command command = processMessage(szRxData);

                            if (command == None)
                                idcSendComand(StatusRequest);

                            iRxEstado = 0;
                        }
                        break;
                    }
                    x++;
                    readedBytes--;
                }
            }

            usleep(200000);     // Realiza el ciclo cada 200ms.
        }

        idcSendComand(DisableAceptor);
        close(_iFDPort);
    }
}

QString ID003::deviceFile(const QString& sFilter) 
{ 
   QString DEV_DIR_PATH = "/dev/ies";

   QDir dir(DEV_DIR_PATH); 

   if (dir.exists()) 
   {    
       dir.setFilter(QDir::Files); 
       dir.setNameFilters(QStringList() << sFilter); 

       QStringList entries = dir.entryList(); 

       return (entries.size() > 0)?QString(DEV_DIR_PATH).append(entries.first()):QString(); 
   }    
   else 
       return QString(); 
}
