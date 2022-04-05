Durla Cosmin 333CC

Ficare functie data(in stdio.h) implementata este descrisa in stdio.c 
in comentarii

Ca functii adaugate de mine:
    1.GetDescriptor -> Returneaza file descriptorul in functie de ce operatie 
se doreste a fi executata
    2.InitializeB_Flags -> Intializeaza flag-urile din buffer
    3.EmptyBuffer -> Goleste buffer-ul



Structura SO_FILE are ca elemente: 
    
    -pid-ul parintelui stream-ului deschis
    -parintele stream-ului deschis
    -file descriptorul fisierului deschis
    -dimensiunea buffer-ului
    -indexul curent din buffer
    -tipul de eroare
    -flag pentru eof
    -un char pentru tipul ultimei operatii
    -un char de tip pointer ce contine pathname-ul fisierului
    -un char de tip pointer ce contine modul de deschidere al fisierului
    -buffer de lungime fixa(folosesc size in aceasta structura,chair daca bufferul are lungime fixa,
     fiindca actiunea de citire poate returna un numar de caractere diferit de IO_BUFFER_SIZE) 

