#include <stdio.h>
#ifndef LLHTTP__TEST
# include "llhttp.h"
#else
# define llhttp_t llparse_t
#endif  /* */

int llhttp_message_needs_eof(const llhttp_t* parser);
int llhttp_should_keep_alive(const llhttp_t* parser);