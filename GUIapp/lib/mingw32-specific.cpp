#ifdef __MINGW32__
﻿#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char *strdup( const char *s ) {
    char* d = (char*)malloc( strlen( s ) + 1 );
    if( !d ) return NULL;
    return strcpy (d,s);
}


int strcasecmp( const char *s1, const char *s2 ) {
    while (true) {
        int c1 = tolower( (unsigned char) *s1++ );
        int c2 = tolower( (unsigned char) *s2++ );
        if (c1 == 0 || c1 != c2)
          return c1 - c2;
    }
}

#endif
