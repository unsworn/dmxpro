/*
 * serial.c 
 *
 * serial port routines
 *
 * Adapted from sio.c
 * (C)1999 Stefano Busti
 * (C)2012 Nicklas Marelis
 */
 
#include <stdio.h>
#include <string.h>   
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <fcntl.h>
#include <dirent.h>
#include "serial.h"

struct serialhandle_s {
    int fd;
};

int 
serial_is_port(const char* p)
{
    if (p == NULL)
        return 0;
    return (p[0] == 'c' && p[1] == 'u' && p[2] == '.');
}

int 
serial_count()
{
    int c=0;
    DIR* dir = NULL;
    struct dirent* entry;
    
    if ((dir = opendir("/dev")) == NULL)
        return 0;
            
    while((entry = readdir(dir)) != NULL)
        if (serial_is_port(entry->d_name))
            c++;

    closedir(dir);
    return c;
        
}

int
serial_list(serialinfo_list_s* list)
{
    int i=0, numDevs = serial_count();
    DIR* dir = NULL;
    struct dirent* entry;
    
    list->size = numDevs;
    
    if ((dir = opendir("/dev")) == NULL)
        return -1;
    
    list->info = (serialinfo_s*) malloc (list->size * sizeof(serialinfo_s));
    
    while ((entry = readdir(dir)) != NULL) {
        if (serial_is_port(entry->d_name) && i < numDevs) {
            list->info[i].name = strdup(entry->d_name);;
            list->info[i].port = 0;
            list->info[i].baud = 230400;
            list->info[i].parity = SERIAL_PARITY_NONE;
            list->info[i].stopbits = 1;
            list->info[i].databits = 8;
            i++;
        }
    }
    
    closedir(dir);
    return i;
}

void
serial_list_free(serialinfo_list_s* list) 
{
    int i;
    if (!list->info)
        return ;
    for (i=0 ; i < list->size ; i++)
        free(list->info[i].name);
    free(list->info);
}

int
serial_init(serial_s* serial)
{
    serial->port = (struct serialhandle_s*) malloc(sizeof(struct serialhandle_s));
    serial->port->fd = SERIAL_INVALID_FD;
    serial->info.baud = 9600;
    serial->info.parity = SERIAL_PARITY_NONE;
    serial->info.stopbits = 1;
    serial->info.databits = 8;
    return 0;
}

void 
serial_cleanup(serial_s* serial)
{
    if (serial_isopen(serial))
        serial_close(serial);
    free(serial->port);
}

int 
serial_open(serial_s* serial, const char* port)
{
    struct termios t;
    int fd, c_stop, c_data, i_parity, c_parity;
    speed_t speed;
    
    if (serial_isopen(serial))
        return -1;
                     
    fd = open(port, O_RDWR);

    if (fd == -1) {
        fprintf(stderr, "serial_open() invalid fd, open fails\n");
        return -1;
    }
    
    if (tcgetattr(fd, &t))
    {
        close(fd);
        return -1;
    }
    
    switch(serial->info.baud)
    {
        case 0:      speed = B0; break;
        case 50:     speed = B50; break;
        case 75:     speed = B75; break;
        case 110:    speed = B110; break;
        case 134:    speed = B134; break;
        case 150:    speed = B150; break;
        case 300:    speed = B300; break;
        case 600:    speed = B600; break;
        case 1200:   speed = B1200; break;
        case 1800:   speed = B1800; break;
        case 2400:   speed = B2400; break;
        case 4800:   speed = B4800; break;
        case 9600:   speed = B9600; break;
        case 19200:  speed = B19200; break;
        case 38400:  speed = B38400; break;
        case 57600:  speed = B57600; break;
        case 115200: speed = B115200; break;
        case 230400: speed = B230400; break;
        default:     speed = B0; break;
    }

    if (speed == B0)
    {
        close(fd);
        return -1;
    }
    
    if (cfsetospeed(&t, speed))
    {
        close(fd);
        return -1;
    }

    if (cfsetispeed(&t, speed))
    {
        close(fd);
        return -1;
    }
    
    switch(serial->info.stopbits)
    {
    case 1: c_stop = 0; break;
    case 2: c_stop = CSTOPB; break;
    default: close(fd); return -1;
    }

    switch(serial->info.databits)
    {
    case 5: c_data = CS5; break;
    case 6: c_data = CS6; break;
    case 7: c_data = CS7; break;
    case 8: c_data = CS8; break;
    default: close(fd); return -1;
    }

    switch(serial->info.parity)
    {
    case SERIAL_PARITY_NONE:
        i_parity = IGNPAR;
        c_parity = 0;
        break;
        
    case SERIAL_PARITY_EVEN:
        i_parity = INPCK;
        c_parity = PARENB;
        break;

    case SERIAL_PARITY_ODD:
        i_parity = INPCK;
        c_parity = PARENB | PARODD;
        break;

    default:
        close(fd);
        return -1;
    }

    if (tcsetattr(fd, TCSANOW, &t))
    {
        close(fd);
        return -1;
    }
    
    serial->port->fd = fd;
    
    return 0;
    
}

void 
serial_close(serial_s *serial)
{
    close(serial->port->fd);
    serial->port->fd = SERIAL_INVALID_FD;
}

int 
serial_read(serial_s *serial, void *buf, size_t count)
{
    return read(serial->port->fd, buf, count);
}

int 
serial_write(serial_s *serial, void *buf, size_t count)
{
    return write(serial->port->fd, buf, count);
}

int
serial_recv(serial_s* serial, int to_sec, void* buf, size_t bufsz)
{
    fd_set         ports;
    struct timeval timeout;
    int            err;

    timeout.tv_sec = to_sec;
    timeout.tv_usec = 0;
    
    FD_ZERO(&ports);

    FD_SET(serial->port->fd, &ports);

    err = select(serial->port->fd+1, &ports, NULL, NULL, &timeout);

    if (err > 0)
        return read(serial->port->fd, buf, bufsz);

    return 0;
}

int 
serial_isopen(serial_s *serial)
{
    return (serial->port->fd != SERIAL_INVALID_FD);
}

int 
serial_setinfo(serial_s *serial, serialinfo_s *info)
{
    serial->info = *info;
    return 0;
}
    
void 
serial_flush(serial_s *serial, int dir)
{
    if (serial_isopen(serial))
    {
        switch(dir)
        {
        case SERIAL_IN: tcflush(serial->port->fd, TCIFLUSH); break;
        case SERIAL_OUT: tcflush(serial->port->fd, TCOFLUSH); break;
        case SERIAL_IN | SERIAL_OUT: tcflush(serial->port->fd, TCIOFLUSH); break;
        }
    }
}

void 
serial_drain(serial_s *serial)
{
    if (serial_isopen(serial))
    {
        tcdrain(serial->port->fd);
    }
}

void 
serial_debug(serial_s *serial, FILE *f)
{
    
    fprintf(f, "serial {\n");
    fprintf(f, "\tfd = %d\n", serial_isopen(serial) ? serial->port->fd : 0);
    fprintf(f, "\tbaud = %ld\n", serial->info.baud);    
    fprintf(f, "\tparity = %d\n", serial->info.parity); 
    fprintf(f, "\tstopbits = %d\n", serial->info.stopbits);
    fprintf(f, "\tdatabits = %d\n", serial->info.databits); 
    fprintf(f, "\topen = %d\n", serial_isopen(serial)); 
    fprintf(f, "}\n\n");
}
