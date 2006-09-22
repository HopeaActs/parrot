/* $id $
 *  Copyright (C) 2005, The Perl Foundation.
 *
 * DO NOT EDIT THIS FILE DIRECTLY!
 * please update the charset/gen_tables.pl script instead.
 *
 * Created by gen_tables.pl jrieks 
 *  Overview:
 *     This file contains various charset tables.
 *  Data Structure and Algorithms:
 *  History:
 *  Notes:
 *  References:
 */

#if !defined(PARROT_CHARSET_TABLES_H_GUARD)
#define PARROT_CHARSET_TABLES_H_GUARD
#include "parrot/cclass.h"
#define WHITESPACE  enum_cclass_whitespace
#define WORDCHAR    enum_cclass_word
#define PUNCTUATION enum_cclass_punctuation
#define DIGIT       enum_cclass_numeric
extern const PARROT_CCLASS_FLAGS Parrot_ascii_typetable[256];
extern const PARROT_CCLASS_FLAGS Parrot_iso_8859_1_typetable[256];
#endif /* PARROT_CHARSET_TABLES_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
