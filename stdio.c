#include                    <string.h>
#include                    <unistd.h>
#include                    <fcntl.h>
#include                    <sys/types.h>
#include                    <sys/wait.h>
#include                    "so_stdio.h"

#define IO_BUFFER_SIZE      4096

//Structura SO_FILE
typedef struct _so_file
{

    int Pid_parent;
    int f_Descriptor;
    int bufferSize;
    int bufferIndex;
    int errorCode;
    int is_EOF;

    char lastOperation;
    const char *pathName;
    const char *mode;
    char buffer[IO_BUFFER_SIZE];

} SO_FILE;

//Returneaza descriptor-ul de fisier in functie de pathname-ul si mode-ul primite
int GetDescriptor(const char *pathname, const char *mode)
{
    if(strcmp(mode, "r+") == 0)
        return open(pathname, O_RDWR, 0644);

    if(strcmp(mode, "w+") == 0)
        return open(pathname, O_RDWR | O_CREAT | O_TRUNC, 0644);

    if(strcmp(mode, "a+") == 0)
        return open(pathname, O_RDWR | O_CREAT | O_APPEND, 0644);

    if(strcmp(mode, "r") == 0)
        return open(pathname, O_RDONLY, 0644);

    if(strcmp(mode, "w") == 0)
        return open(pathname, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if(strcmp(mode, "a") == 0)
        return open(pathname, O_WRONLY | O_CREAT | O_APPEND, 0644);

    return SO_EOF;
}

void InitializeB_Flags(SO_FILE *stream)
{
    stream->lastOperation = 'n';
    stream->errorCode = 0;
    stream->is_EOF = 0;
}

void EmptyBuffer(SO_FILE *stream)
{   
    stream->bufferSize = IO_BUFFER_SIZE;
    stream->bufferIndex = 0;
    memset(stream->buffer, 0, stream->bufferSize);
}

//Creeaza un SO_FILE nou un pathname si mode corespunzator
FUNC_DECL_PREFIX SO_FILE *so_fopen(const char *pathname, const char *mode)
{
    SO_FILE *stream = (SO_FILE*)malloc(1 * sizeof(SO_FILE));
    stream->f_Descriptor = GetDescriptor(pathname, mode);

    if(stream->f_Descriptor < 0)
    {
        free(stream);    
        return NULL;
    }
    
    stream->mode = mode;
    stream->pathName = pathname;
    EmptyBuffer(stream);
    InitializeB_Flags(stream);
    return stream;

}

//Returneaza file descriptor-ul corespunzator cu fisierul deschis
FUNC_DECL_PREFIX int so_fileno(SO_FILE *stream)
{
    return stream->f_Descriptor;
}

//Returneaza pozitia curenta a cursorului in functie de ultima operatie 
FUNC_DECL_PREFIX long so_ftell(SO_FILE *stream)
{
    if(stream->lastOperation == 'n')
        return lseek(stream->f_Descriptor, 0, SEEK_CUR) + stream->bufferIndex;
    if(stream->lastOperation == 'r')
        return lseek(stream->f_Descriptor, 0, SEEK_CUR) - stream->bufferSize + stream->bufferIndex;
    if(stream->lastOperation == 'w')
        return lseek(stream->f_Descriptor, 0, SEEK_CUR) + stream->bufferIndex;    

    return SO_EOF;
}

// Scrierestul  buffer-ului in fisier si il goleste, si inchide fisierul
FUNC_DECL_PREFIX int so_fclose(SO_FILE *stream)
{
    
    char lastOperation = stream->lastOperation;
    int flushErrorCode = so_fflush(stream);
    int closeErrorCode = close(stream->f_Descriptor);

    free(stream);
    return lastOperation == 'w' ? flushErrorCode : closeErrorCode;
}

//Scrie buffer-ul in fisier si il goleste, dar doar dupa o operatie de scriere
FUNC_DECL_PREFIX int so_fflush(SO_FILE *stream)
{
    int current_bytes_written = 0;
    int total_bytes_written = 0;

    if(stream->lastOperation == 'w')
    {
        while(total_bytes_written < stream->bufferIndex)
        {
            current_bytes_written = write
            (
                stream->f_Descriptor, 
                stream->buffer + total_bytes_written, 
                stream->bufferIndex - total_bytes_written
            );

            if(current_bytes_written >= 0)
            {
                total_bytes_written += current_bytes_written;
            }
            else
            {
                stream->errorCode = SO_EOF;
                return SO_EOF;
            }
        }

        EmptyBuffer(stream);
        return 0;
    }

    return SO_EOF;
}


//Citeste un numar "nmemb * size" de bytes din buffer folosind functia so_fgetc
FUNC_DECL_PREFIX size_t so_fread(void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
    char *derefrencedPtr = (char*)ptr;
    int currentReadChar;

    int i = 0;

    while (i < nmemb * size)
    {
        currentReadChar = so_fgetc(stream);
        
        if(currentReadChar != SO_EOF)
            derefrencedPtr[i] = currentReadChar;
        else
            break;
        i++;    
    }

    return i / size;
}

// Golesete/scrie si apoi goleste buffer-ul.
FUNC_DECL_PREFIX int so_fseek(SO_FILE *stream, long offset, int whence)
{
    if(stream->lastOperation == 'w'){

        so_fflush(stream);

        }
    if(stream->lastOperation == 'r'){
        EmptyBuffer(stream);

        }
    return lseek(stream->f_Descriptor, offset, whence) >= 0 ? 0 : SO_EOF;    

}

// Scrie un numar "nmemb * size" de bytes in buffer folosind functia so_fputc
FUNC_DECL_PREFIX size_t so_fwrite(const void *ptr, size_t size, size_t nmemb, SO_FILE *stream)
{
    char *derefrencedPtr;
    
    derefrencedPtr = (char*)ptr;
    int i = 0;

    while (i < nmemb * size)
    {
        if(so_fputc(derefrencedPtr[i], stream) == SO_EOF)
            break;
        i++;    
    }

    return nmemb;
}

//Citeste un numar IO_BUFFER_SIZE de bytes in buffer ina celasi timp si
//returneaza byte-ul disponibil la bufferIndex
FUNC_DECL_PREFIX int so_fgetc(SO_FILE *stream)
{
    stream->lastOperation = 'r';

    if(stream->bufferIndex == stream->bufferSize || stream->bufferIndex == 0)
    {
        stream->bufferSize = read(stream->f_Descriptor, stream->buffer, IO_BUFFER_SIZE);
        
        if(stream->bufferSize > 0)
        {
            stream->bufferIndex = 0; 
        }
        else
        {
            if(stream->bufferSize == 0)
                stream->is_EOF = 1;
        
            stream->errorCode = SO_EOF;
            return SO_EOF;
        }
    }

    return (int)((unsigned char)(stream->buffer[(stream->bufferIndex)++]));
}

// Returneaza 0 daca nu a aparut nici-o eroare pana la acest apel, sau ultimul code de erare in ca contrar
FUNC_DECL_PREFIX int so_ferror(SO_FILE *stream)
{
    return stream->errorCode;
}

// Returneaza 1 daca s-a ajuns la EoF si 0 daca nu
FUNC_DECL_PREFIX int so_feof(SO_FILE *stream)
{
    return stream->is_EOF;
}

// Scrie byte-ul primit de bufferIndex,
// scrie si golesete bufferul dupa un numar IO_BUFFER_SIZE de bytes 
FUNC_DECL_PREFIX int so_fputc(int c, SO_FILE *stream)
{
    stream->lastOperation = 'w';

    if(stream->bufferIndex == stream->bufferSize)
    {
        if(so_fflush(stream) != SO_EOF)
            EmptyBuffer(stream);
        else    
            return SO_EOF;
        
    }

    stream->buffer[(stream->bufferIndex)++] = c;
    return (int)((unsigned char)(c));
}




// Aceasta fuctie creaza un pipe anonim bidirectional
// In copil, inchide sfarsitul nedorit al pipe-ului si 
// muta pointerul STDIN/STDOUT in sfarsitul dorit al pipe-ului.
// In parinte, inchide sfarsitul nedorit al pipe-ului si
// seteaza sfarsitul dorit al pipe-ului ca f_Descriptor pentru structura
FUNC_DECL_PREFIX SO_FILE *so_popen(const char *command, const char *type)
{
    int anonPipe[2];
    SO_FILE *stream;
    const char *argv[] = {"sh", "-c", command, NULL};
    pid_t pid;

    

    if (pipe(anonPipe) == SO_EOF)
        return NULL;
    
    pid = fork();
    
    //Parinte
    if(pid > 0)
    {
        stream = (SO_FILE*)malloc(1 * sizeof(SO_FILE));

        EmptyBuffer(stream);
        InitializeB_Flags(stream);
        stream->Pid_parent = pid;
        stream->f_Descriptor = anonPipe[!strcmp(type, "w")];
        close(anonPipe[!strcmp(type, "r")]);
        return stream;
    }

    // Copil
    if(pid == 0)
    {
        int x = 1, y = 0;

        if(strcmp(type, "w") == 0)
        {
            close(anonPipe[x]);
            dup2(anonPipe[y], STDIN_FILENO);
        }
        else if(strcmp(type, "r") == 0)
        {
            close(anonPipe[y]);
            dup2(anonPipe[x], STDOUT_FILENO);
        }

        execvp("sh", (char *const *) argv);
        exit(SO_EOF);
    }

    return NULL;
}

//Asteapta ca procesul copil sa se incheie cu succes
FUNC_DECL_PREFIX int so_pclose(SO_FILE *stream)
{
    int Pid_parent = stream->Pid_parent;
    int pCloseStatus;
    so_fclose(stream);
 
    return waitpid(Pid_parent, &pCloseStatus, 0) != SO_EOF ? pCloseStatus : SO_EOF;
}