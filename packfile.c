/*
** packfile.c
**
** The PackFile API.
**
** Copyright (C) 2001-2002 Gregor N. Purdy. All rights reserved.
** This program is free software. It is subject to the same
** license as Parrot itself.
**
** $Id$
*/

#include "parrot/parrot.h"
#include "parrot/packfile.h"


#define TRACE_PACKFILE 0


/******************************************************************************

=head1 PackFile Manipulation Functions

This file, C<packfile.c> contains all the functions required
for the processing of the structure of a PackFile. It is not
intended to understand the byte code stream itself, but merely
to dissect and reconstruct data from the various segments.
See L<parrotbyte> for information about the structure of the
frozen bycode.

=cut

******************************************************************************/


/******************************************************************************

=head2 PackFile Structure Functions

=over 4

=cut

******************************************************************************/


/***************************************

=item new

Allocate a new empty PackFile.

NOTE: The PackFile's magic is automatically set to PARROT_MAGIC.

=cut

***************************************/

struct PackFile *
PackFile_new(void)
{
    struct PackFile *self = mem_sys_allocate((UINTVAL)sizeof(struct PackFile));

    if (!self) {
        fprintf(stderr, "PackFile_new: Unable to allocate!\n");
        return NULL;
    }

    self->magic = PARROT_MAGIC;
    self->fixup_table = PackFile_FixupTable_new();

    if (!self->fixup_table) {
        fprintf(stderr, "PackFile_new: Unable to allocate fixup table!\n");
        mem_sys_free(self);
        return NULL;
    }

    self->const_table = PackFile_ConstTable_new();

    if (!self->const_table) {
        fprintf(stderr, "PackFile_new: Unable to allocate constant table!\n");
        PackFile_FixupTable_DELETE(self->fixup_table);
        self->fixup_table = NULL;
        mem_sys_free(self);
        return NULL;
    }

    self->byte_code = NULL;
    self->byte_code_size = 0;

    return self;
}


/***************************************

=item DELETE

Delete a PackFile.

=cut

***************************************/

void
PackFile_DELETE(struct PackFile *self)
{
    if (!self) {
        fprintf(stderr, "PackFile_DELETE: self == NULL!\n");
        return;
    }

    PackFile_clear(self);

    PackFile_FixupTable_DELETE(self->fixup_table);
    self->fixup_table = NULL;

    PackFile_ConstTable_DELETE(self->const_table);
    self->const_table = NULL;

    return;
}


/***************************************

=item clear

Clear a PackFile.

NOTE: The PackFile's magic is set to PARROT_MAGIC, not to zero (0).

=cut

***************************************/

void
PackFile_clear(struct PackFile *self)
{
    if (!self) {
        fprintf(stderr, "PackFile_clear: self == NULL!\n");
        return;
    }

    self->magic = PARROT_MAGIC;

    PackFile_FixupTable_clear(self->fixup_table);
    PackFile_ConstTable_clear(self->const_table);

    if (self->byte_code) {
        mem_sys_free(self->byte_code);
        self->byte_code = NULL;
    }

    self->byte_code_size = 0;

    return;
}


/***************************************

=item get_magic

Get the PackFile's magic. You really should not need to use this, but it is
here for completeness of the interface.

=cut

***************************************/

opcode_t
PackFile_get_magic(struct PackFile *self)
{
    return self->magic;
}


/***************************************

=item set_magic

Set the PackFile's magic. You really should not need to use this, but it is
here for completeness of the interface.

=cut

***************************************/

void
PackFile_set_magic(struct PackFile *self, opcode_t magic)
{
    self->magic = magic;
}


/***************************************

=item get_byte_code_size

Get the size of the byte code.

=cut

***************************************/

opcode_t
PackFile_get_byte_code_size(struct PackFile *self)
{
    return self->byte_code_size;
}


/***************************************

=item get_byte_code

Get the byte code.

NOTE: The memory is owned by the PackFile.

=cut

***************************************/

opcode_t *
PackFile_get_byte_code(struct PackFile *self)
{
    return self->byte_code;
}


/***************************************

=item set_byte_code

Set the byte code.

NOTE: The PackFile makes its own copy of the byte code, so you still
own the memory for the input byte code.

=cut

***************************************/

void
PackFile_set_byte_code(struct PackFile *self, size_t byte_code_size,
                       opcode_t *byte_code)
{
    if (self->byte_code) {
        mem_sys_free(self->byte_code);
        self->byte_code = NULL;
        self->byte_code_size = 0;
    }

    if (byte_code_size > 0 && byte_code) {
        self->byte_code = mem_sys_allocate(byte_code_size);

        if (!self->byte_code) {
            fprintf(stderr, "Could not allocate buffer to copy byte code!\n");
            return;
        }

        mem_sys_memcopy(self->byte_code, byte_code, byte_code_size);

        self->byte_code_size = byte_code_size;
    }

    return;
}


/***************************************

=item unpack

Unpack a PackFile from a block of memory. The format is:

  opcode_t magic

  opcode_t segment_length
  *  fixup_segment

  opcode_t segment_length
  *  const_segment

  opcode_t segment_length
  *  byte_code

Checks to see if the magic matches the Parrot magic number for
Parrot PackFiles.

Returns one (1) if everything is OK, else zero (0).

=cut

***************************************/

opcode_t
PackFile_unpack(struct Parrot_Interp *interpreter, struct PackFile *self,
                opcode_t *packed, size_t packed_size)
{
    opcode_t segment_size;
    opcode_t *cursor;

    if (!self) {
        fprintf(stderr, "PackFile_unpack: self == NULL!\n");
        return 0;
    }

    PackFile_clear(self);

    cursor = packed;

    /*
     * Unpack and verify the magic:
     */

    self->magic = *cursor;
    cursor++;

    if (self->magic != PARROT_MAGIC) {
        fprintf(stderr, "PackFile_unpack: Not a Parrot PackFile!\n");
        return 0;
    }

#if TRACE_PACKFILE
    printf("PackFile_unpack(): Magic verified.\n");
#endif

    /*
     * Unpack the Fixup Table Segment:
     */

    segment_size = *cursor;
    cursor++;

#if TRACE_PACKFILE
    printf("PackFile_unpack(): Unpacking %ld bytes for fixup table...\n",
           segment_size);
#endif

    if (segment_size % sizeof(opcode_t)) {
        fprintf(stderr,
                "PackFile_unpack: Illegal fixup table segment size %d (must be multiple of %d)!\n",
                (int)segment_size, sizeof(opcode_t));
        return 0;
    }

    if (!PackFile_FixupTable_unpack(self->fixup_table, cursor, segment_size)) {
        fprintf(stderr,
                "PackFile_unpack: Error reading fixup table segment!\n");
        return 0;
    }

    cursor += segment_size / sizeof(opcode_t);  /* Segment size is in bytes */

    /*
     * Unpack the Constant Table Segment:
     */

    segment_size = *cursor;
    cursor++;

#if TRACE_PACKFILE
    printf("PackFile_unpack(): Unpacking %ld bytes for constant table...\n",
           segment_size);
#endif

    if (segment_size % sizeof(opcode_t)) {
        fprintf(stderr,
                "PackFile_unpack: Illegal constant table segment size %d (must be multiple of %d)!\n",
                (int)segment_size, sizeof(opcode_t));
        return 0;
    }

    if (!PackFile_ConstTable_unpack
        (interpreter, self->const_table, cursor, segment_size)) {
        fprintf(stderr,
                "PackFile_unpack: Error reading constant table segment!\n");
        return 0;
    }

    cursor += segment_size / sizeof(opcode_t);  /* Segment size is in bytes */

    /*
     * Unpack the Byte Code Segment:
     */

    segment_size = *cursor;
    cursor++;

#if TRACE_PACKFILE
    printf("PackFile_unpack(): Unpacking %ld bytes for byte code...\n",
           segment_size);
#endif

    self->byte_code_size = segment_size;

    if (self->byte_code_size > 0) {
        self->byte_code = mem_sys_allocate(self->byte_code_size);

        if (!self->byte_code) {
            fprintf(stderr,
                    "PackFile_unpack: Unable to allocate memory to copy byte code!\n");
            self->byte_code_size = 0;
            return 0;
        }

        mem_sys_memcopy(self->byte_code, cursor, self->byte_code_size);
    }

    cursor += segment_size / sizeof(opcode_t);  /* Segment size is in bytes */

    return ((size_t)(cursor - packed) * sizeof(opcode_t)) == packed_size;
}


/***************************************

=item pack_size

Determine the size of the buffer needed in order to pack the PackFile into a
contiguous region of memory.

=cut

***************************************/

opcode_t
PackFile_pack_size(struct PackFile *self)
{
    opcode_t magic_size;
    opcode_t segment_length_size;
    opcode_t fixup_table_size;
    opcode_t const_table_size;

    magic_size = sizeof(opcode_t);
    segment_length_size = sizeof(opcode_t);

#if TRACE_PACKFILE
    printf("getting fixup table size...\n");
#endif

    fixup_table_size = PackFile_FixupTable_pack_size(self->fixup_table);

#if TRACE_PACKFILE
    printf("  ... it is %ld\n", fixup_table_size);
#endif

#if TRACE_PACKFILE
    printf("getting const table size...\n");
#endif

    const_table_size = PackFile_ConstTable_pack_size(self->const_table);

#if TRACE_PACKFILE
    printf("  ... it is %ld\n", const_table_size);
#endif

    return magic_size
        + segment_length_size + fixup_table_size
        + segment_length_size + const_table_size
        + segment_length_size + self->byte_code_size;
}


/***************************************

=item pack

Pack the PackFile into a contiguous region of memory. NOTE: The memory block
had better have at least the amount of memory indicated by
PackFile_pack_size()!

=cut

***************************************/

void
PackFile_pack(struct PackFile *self, opcode_t *packed)
{
    opcode_t *cursor = packed;
    opcode_t fixup_table_size =
        PackFile_FixupTable_pack_size(self->fixup_table);
    opcode_t const_table_size =
        PackFile_ConstTable_pack_size(self->const_table);

    /* Pack the magic */

    *cursor = self->magic;
    cursor++;

    /* Pack the fixup table size, followed by the packed fixup table */

    *cursor = fixup_table_size;
    cursor++;

    PackFile_FixupTable_pack(self->fixup_table, cursor);
    cursor += fixup_table_size / sizeof(opcode_t);      /* Sizes are in bytes */

    /* Pack the constant table size, followed by the packed constant table */

    *cursor = const_table_size;
    cursor++;

    PackFile_ConstTable_pack(self->const_table, cursor);
    cursor += const_table_size / sizeof(opcode_t);      /* Sizes are in bytes */

    /* Pack the byte code size, followed by the byte code */

    *cursor = self->byte_code_size;
    cursor++;

    if (self->byte_code_size) {
        mem_sys_memcopy(cursor, self->byte_code, self->byte_code_size);
    }

    return;
}


/***************************************

=item dump

Dump the PackFile to standard out in a human-readable form.

=cut

***************************************/

void
PackFile_dump(struct PackFile *self)
{
    size_t i;

    printf("MAGIC => 0x%08lx,\n", (unsigned long)self->magic);

    printf("FIXUP => {\n");

    PackFile_FixupTable_dump(self->fixup_table);

    printf("},\n");

    printf("CONST => [\n");

    PackFile_ConstTable_dump(self->const_table);

    printf("],\n");

    printf("BCODE => [ # %ld bytes", (long)self->byte_code_size);

    for (i = 0; i < self->byte_code_size / sizeof(opcode_t); i++) {
        if (i % 8 == 0) {
            printf("\n    %08lx:  ", (unsigned long)i * sizeof(opcode_t));
        }
        printf("%08lx ", (unsigned long)self->byte_code[i]);
    }

    printf("\n]\n");

    return;
}


/*

=back

=cut

*/


/******************************************************************************

=head2 PackFile FixupTable Structure Functions

=over 4

=cut

******************************************************************************/


/***************************************

=item new

Allocate a new empty PackFile FixupTable.

=cut

***************************************/

struct PackFile_FixupTable *
PackFile_FixupTable_new(void)
{
    struct PackFile_FixupTable *self =
        mem_sys_allocate((UINTVAL)sizeof(struct PackFile_FixupTable));

    self->dummy = 0;

    return self;
}


/***************************************

=item DELETE

Delete a PackFile FixupTable.

=cut

***************************************/

void
PackFile_FixupTable_DELETE(struct PackFile_FixupTable *self)
{
    if (!self) {
        fprintf(stderr, "PackFile_FixupTable_DELETE: self == NULL!\n");
        return;
    }

    mem_sys_free(self);

    return;
}


/***************************************

=item clear

Clear a PackFile FixupTable.

=cut

***************************************/

void
PackFile_FixupTable_clear(struct PackFile_FixupTable *self)
{
    if (!self) {
        fprintf(stderr, "PackFile_FixupTable_clear: self == NULL!\n");
        return;
    }

    return;
}


/***************************************

=item unpack

Unpack a PackFile FixupTable from a block of memory.

NOTE: There is no format defined for FixupTables yet.

Returns one (1) if everything is OK, else zero (0).

=cut

***************************************/

opcode_t
PackFile_FixupTable_unpack(struct PackFile_FixupTable *self, opcode_t *packed,
                           opcode_t packed_size)
{
    UNUSED(self);
    UNUSED(packed);
    UNUSED(packed_size);
    return 1;
}


/***************************************

=item pack_size

Determine the size of the buffer needed in order to pack the PackFile into a
contiguous region of memory.

=cut

***************************************/

opcode_t
PackFile_FixupTable_pack_size(struct PackFile_FixupTable *self)
{
    UNUSED(self);
    return 0;
}


/***************************************

=item pack

Pack the PackFile FixupTable into a contiguous region of memory. NOTE: The memory
block had better have at least the amount of memory indicated by
PackFile_FixupTable_pack_size()!

=cut

***************************************/

void
PackFile_FixupTable_pack(struct PackFile_FixupTable *self, opcode_t *packed)
{
    UNUSED(self);
    UNUSED(packed);
    return;
}


/***************************************

=item dump

Dump the PackFile FixupTable to standard output.

=cut

***************************************/

void
PackFile_FixupTable_dump(struct PackFile_FixupTable *self)
{
    UNUSED(self);
    return;
}



/*

=back

=cut

*/


/******************************************************************************

=head2 PackFile ConstTable Structure Functions

=over 4

=cut

******************************************************************************/


/***************************************

=item new

Allocate a new empty PackFile ConstTable.

=cut

***************************************/

struct PackFile_ConstTable *
PackFile_ConstTable_new(void)
{
    struct PackFile_ConstTable *self =
        mem_sys_allocate((UINTVAL)sizeof(struct PackFile_ConstTable));

    self->const_count = 0;
    self->constants = NULL;

    return self;
}


/***************************************

=item DELETE

Delete a PackFile ConstTable.

=cut

***************************************/

void
PackFile_ConstTable_DELETE(struct PackFile_ConstTable *self)
{
    if (!self) {
        fprintf(stderr, "PackFile_ConstTable_DELETE: self == NULL!\n");
        return;
    }

    PackFile_ConstTable_clear(self);

    mem_sys_free(self);

    return;
}


/***************************************

=item clear

Clear a PackFile ConstTable.

=cut

***************************************/

void
PackFile_ConstTable_clear(struct PackFile_ConstTable *self)
{
    opcode_t i;

    if (!self) {
        fprintf(stderr, "PackFile_ConstTable_DELETE: self == NULL!\n");
        return;
    }

    for (i = 0; i < self->const_count; i++) {
        PackFile_Constant_DELETE(self->constants[i]);
        self->constants[i] = NULL;
    }

    mem_sys_free(self->constants);

    self->constants = NULL;
    self->const_count = 0;

    return;
}


/***************************************

=item get_const_count

Get the number of constants in the ConstTable.

=cut

***************************************/

opcode_t
PackFile_ConstTable_get_const_count(struct PackFile_ConstTable *self)
{
    if (!self) {
        fprintf(stderr,
                "PackFile_ConstTable_get_const_count: self == NULL!\n");
        return -1;
    }

    return self->const_count;
}


/***************************************

=item push_constant

Push a new Constant onto the ConstTable. Note: the Constant now belongs
to the ConstTable.

=cut

***************************************/

void
PackFile_ConstTable_push_constant(struct PackFile_ConstTable *self,
                                  struct PackFile_Constant *constant)
{
    struct PackFile_Constant **temp;
    opcode_t i;

    if (!self) {
        fprintf(stderr, "PackFile_ConstTable_push_constant: self == NULL!\n");
        return;
    }

    if (!constant) {
        fprintf(stderr,
                "PackFile_ConstTable_push_constant: constant == NULL!\n");
        return;
    }

    temp =
        mem_sys_allocate((self->const_count +
                          1) * sizeof(struct PackFile_Constant *));

    if (!temp) {
        fprintf(stderr,
                "Unable to reallocate Constant array to push a new Constant!\n");
        return;
    }

    for (i = 0; i < self->const_count; i++) {
        temp[i] = self->constants[i];
    }

    temp[self->const_count++] = constant;

    mem_sys_free(self->constants);

    self->constants = temp;

    return;
}


/***************************************

=item constant

Retrieve a Constant from the ConstTable.

=cut

***************************************/

struct PackFile_Constant *
PackFile_ConstTable_constant(struct PackFile_ConstTable *self, opcode_t idx)
{
    if (!self) {
        fprintf(stderr, "PackFile_ConstTable_constant: self == NULL!\n");
        return NULL;
    }

    if (idx < 0 || idx >= self->const_count) {
        return NULL;
    }

    return self->constants[idx];
}


/***************************************

=item unpack

Unpack a PackFile ConstTable from a block of memory. The format is:

  opcode_t const_count
  *  constants

Returns one (1) if everything is OK, else zero (0).

=cut

***************************************/

opcode_t
PackFile_ConstTable_unpack(struct Parrot_Interp *interpreter,
                           struct PackFile_ConstTable *self, opcode_t *packed,
                           opcode_t packed_size)
{
    opcode_t *cursor;
    opcode_t i;
    opcode_t rc = 1;

    if (!self) {
        fprintf(stderr, "PackFile_ConstTable_unpack: self == NULL!\n");
        return 0;
    }

    PackFile_ConstTable_clear(self);

    cursor = packed;

    self->const_count = *cursor;
    cursor++;

#if TRACE_PACKFILE
    printf("PackFile_ConstTable_unpack(): Unpacking %ld constants...\n",
           self->const_count);
#endif

    if (self->const_count == 0) {
        return 1;
    }

    self->constants =
        mem_sys_allocate(self->const_count *
                         sizeof(struct PackFile_Constant *));

    if (!self->constants) {
        fprintf(stderr,
                "PackFile_ConstTable_unpack: Could not allocate memory for array!\n");
        self->const_count = 0;
        return 0;
    }

    for (i = 0; i < self->const_count; i++) {
#if TRACE_PACKFILE
        printf("PackFile_ConstTable_unpack(): Unpacking constant %ld...\n", i);
#endif

        self->constants[i] = PackFile_Constant_new();
        rc = PackFile_Constant_unpack(interpreter, self->constants[i], cursor,
                                      packed_size - (cursor - packed));
        /* NOTE: It would be nice if each of these had its own length first */

        cursor +=
            PackFile_Constant_pack_size(self->constants[i]) / sizeof(opcode_t);
    }

    return rc;
}


/***************************************

=item pack_size

Determine the size of the buffer needed in order to pack the PackFile into a
contiguous region of memory.

=cut

***************************************/

opcode_t
PackFile_ConstTable_pack_size(struct PackFile_ConstTable *self)
{
    opcode_t i;
    opcode_t size = 0;

    if (!self) {
        fprintf(stderr, "PackFile_ConstTable_size: self == NULL!\n");
        return -1;
    }

    for (i = 0; i < self->const_count; i++) {
#if TRACE_PACKFILE
        printf("  ... Getting size of constant #%ld...\n", i);
#endif
        size += PackFile_Constant_pack_size(self->constants[i]);
    }

    return sizeof(opcode_t) + size;
}


/***************************************

=item pack

Pack the PackFile ConstTable into a contiguous region of memory. NOTE: The memory
block had better have at least the amount of memory indicated by
PackFile_ConstTable_pack_size()!

=cut

***************************************/

void
PackFile_ConstTable_pack(struct PackFile_ConstTable *self, opcode_t *packed)
{
    opcode_t *cursor;
    opcode_t i;

    if (!self) {
        fprintf(stderr, "PackFile_ConstTable_pack: self == NULL!\n");
        return;
    }

    cursor = packed;

    *cursor = self->const_count;
    cursor++;

    for (i = 0; i < self->const_count; i++) {
        PackFile_Constant_pack(self->constants[i], cursor);

        cursor +=
            PackFile_Constant_pack_size(self->constants[i]) / sizeof(opcode_t);
    }

    return;
}


/***************************************

=item dump

Dump the PackFile ConstTable to standard output.

=cut

***************************************/

void
PackFile_ConstTable_dump(struct PackFile_ConstTable *self)
{
    opcode_t i;

    if (!self) {
        fprintf(stderr, "PackFile_ConstTable_dump: self == NULL!\n");
        return;
    }

    for (i = 0; i < self->const_count; i++) {
        printf("    # %ld:\n", (long)i);
        PackFile_Constant_dump(self->constants[i]);
    }

    return;
}


/*

=back

=cut

*/



/******************************************************************************

=head2 PackFile Constant Structure Functions

=over 4

=cut

******************************************************************************


***************************************

=item new

Allocate a new empty PackFile Constant.
This is only here so we can make a new one and then do an unpack.

=cut

***************************************/

struct PackFile_Constant *
PackFile_Constant_new(void)
{
    struct PackFile_Constant *self =
        mem_sys_allocate((UINTVAL)sizeof(struct PackFile_Constant));

    self->type = PFC_NONE;

    return self;
}


/***************************************

=item new_integer

Allocate a new PackFile Constant containing an opcode_t.

=cut

***************************************/

struct PackFile_Constant *
PackFile_Constant_new_integer(opcode_t i)
{
    struct PackFile_Constant *self =
        mem_sys_allocate((UINTVAL)sizeof(struct PackFile_Constant));

    self->type = PFC_INTEGER;
    self->integer = i;

    return self;
}


/***************************************

=item new_number

Allocate a new PackFile Constant containing an FLOATVAL.

=cut

***************************************/

struct PackFile_Constant *
PackFile_Constant_new_number(FLOATVAL n)
{
    struct PackFile_Constant *self =
        mem_sys_allocate((UINTVAL)sizeof(struct PackFile_Constant));

    self->type = PFC_NUMBER;
    self->number = n;

    return self;
}


/***************************************

=item new_string

Allocate a new PackFile Constant containing a string.

=cut

***************************************/

struct PackFile_Constant *
PackFile_Constant_new_string(struct Parrot_Interp *interpreter, STRING *s)
{
    struct PackFile_Constant *self =
        mem_sys_allocate((UINTVAL)sizeof(struct PackFile_Constant));

    self->type = PFC_STRING;
    self->string = string_copy(interpreter, s);

    return self;
}


/***************************************

=item DELETE

Delete a PackFile Constant.

=cut

***************************************/

void
PackFile_Constant_DELETE(struct PackFile_Constant *self)
{
    if (!self) {
        fprintf(stderr, "PackFile_Constant_DELETE: self == NULL!\n");
        return;
    }

    PackFile_Constant_clear(self);

    mem_sys_free(self);

    return;
}


/***************************************

=item clear

Clear a PackFile Constant.

=cut

***************************************/

void
PackFile_Constant_clear(struct PackFile_Constant *self)
{
    if (!self) {
        fprintf(stderr, "PackFile_Constant_clear: self == NULL!\n");
        return;
    }

    switch (self->type) {
    case PFC_NONE:
        break;

    case PFC_INTEGER:
        self->integer = 0;
        break;

    case PFC_NUMBER:
        self->number = 0.0;
        break;

    case PFC_STRING:
        if (self->string) {
            string_destroy(self->string);
            self->string = NULL;
        }
        break;

    default:
        fprintf(stderr,
                "PackFile_Constant_clear: Unrecognized type '%c' (%ld)!\n",
                (char)self->type, self->type);
        return;
        break;
    }

    self->type = PFC_NONE;

    return;
}


/***************************************

=item get_type

Get the Constant type.

=cut

***************************************/

opcode_t
PackFile_Constant_get_type(struct PackFile_Constant *self)
{
    if (!self) {
        /* TODO: Is it OK to be silent about this? */
        return PFC_NONE;
    }

    return self->type;
}


/***************************************

=item unpack

Unpack a PackFile Constant from a block of memory. The format is:

  opcode_t type
  opcode_t size
  *  data

Returns one (1) if everything is OK, else zero (0).

=cut

***************************************/

opcode_t
PackFile_Constant_unpack(struct Parrot_Interp *interpreter,
                         struct PackFile_Constant *self, opcode_t *packed,
                         opcode_t packed_size)
{
    opcode_t *cursor;
    opcode_t type;
    opcode_t size;
    opcode_t rc = 1;

    UNUSED(packed_size);

    if (!self) {
        return 0;
    }

    cursor = packed;

    type = *cursor;
    cursor++;

#if TRACE_PACKFILE
    printf("PackFile_Constant_unpack(): Type is %ld ('%c')...\n", type,
           (char)type);
#endif

    size = *cursor;
    cursor++;

#if TRACE_PACKFILE
    printf("PackFile_Constant_unpack(): Size is %ld...\n", size);
#endif

    switch (type) {
    case PFC_NONE:
#if TRACE_PACKFILE
        rc = printf
            ("PackFile_Constant_unpack(): Unpacking no-type constant...\n");
#endif
        break;

    case PFC_INTEGER:
#if TRACE_PACKFILE
        printf("PackFile_Constant_unpack(): Unpacking integer constant...\n");
#endif
        rc = PackFile_Constant_unpack_integer(self, cursor, size);
        break;

    case PFC_NUMBER:
#if TRACE_PACKFILE
        printf("PackFile_Constant_unpack(): Unpacking number constant...\n");
#endif
        rc = PackFile_Constant_unpack_number(self, cursor, size);
        break;

    case PFC_STRING:
#if TRACE_PACKFILE
        printf("PackFile_Constant_unpack(): Unpacking string constant...\n");
#endif
        rc = PackFile_Constant_unpack_string(interpreter, self, cursor, size);
        break;

    default:
        fprintf(stderr,
                "PackFile_Constant_clear: Unrecognized type '%c' during unpack!\n",
                (char)type);
        return 0;
        break;
    }

    return rc;
}


/***************************************

=item unpack_integer

Unpack a PackFile Constant integer from a block of memory. The format is:

  opcode_t value

Returns one (1) if everything is OK, else zero (0).

=cut

***************************************/

opcode_t
PackFile_Constant_unpack_integer(struct PackFile_Constant *self,
                                 opcode_t *packed, opcode_t packed_size)
{
    opcode_t *cursor;
    opcode_t value;

    UNUSED(packed_size);

    if (!self) {
        return 0;
    }

    PackFile_Constant_clear(self);

    cursor = packed;

    value = *cursor;
    cursor++;

    self->type = PFC_INTEGER;
    self->integer = value;

    return 1;
}


/***************************************

=item unpack_number

Unpack a PackFile Constant number from a block of memory. The format is:

  FLOATVAL value

Returns one (1) if everything is OK, else zero (0).

=cut

***************************************/

opcode_t
PackFile_Constant_unpack_number(struct PackFile_Constant *self,
                                opcode_t *packed, opcode_t packed_size)
{
    opcode_t *cursor;
    FLOATVAL value;

    UNUSED(packed_size);

    if (!self) {
        return 0;
    }

    PackFile_Constant_clear(self);

    cursor = packed;

    /* We need to do a memcpy from the packed area to the value 
     * because we can't guarantee that the packed area (which is 
     * aligned for an opcode_t) is suitably aligned for a FLOATVAL.
     * This could be made contingent upon some preprocessor defines 
     * determined by Configure.
     */
    mem_sys_memcopy(&value, cursor, sizeof(FLOATVAL));

    self->type = PFC_NUMBER;
    self->number = value;

    return 1;
}


/***************************************

=item unpack_string

Unpack a PackFile Constant from a block of memory. The format is:

  opcode_t flags
  opcode_t encoding
  opcode_t type
  opcode_t size
  *  data

The data is expected to be zero-padded to an opcode_t-boundary, so any
pad bytes are removed.

Returns one (1) if everything is OK, else zero (0).

=cut

***************************************/

opcode_t
PackFile_Constant_unpack_string(struct Parrot_Interp *interpreter,
                                struct PackFile_Constant *self,
                                opcode_t *packed, opcode_t packed_size)
{
    opcode_t *cursor;
    UINTVAL flags;
    opcode_t encoding;
    opcode_t type;
    size_t size;

    UNUSED(packed_size);

    if (!self) {
        return 0;
    }

    PackFile_Constant_clear(self);

    cursor = packed;

    flags = (UINTVAL)*cursor;
    cursor++;

#if TRACE_PACKFILE
    printf("PackFile_Constant_unpack_string(): flags are 0x%04x...\n", flags);
#endif

    encoding = *cursor;
    cursor++;

#if TRACE_PACKFILE
    printf("PackFile_Constant_unpack_string(): encoding is %ld...\n",
           encoding);
#endif

    type = *cursor;
    cursor++;

#if TRACE_PACKFILE
    printf("PackFile_Constant_unpack_string(): type is %ld...\n", type);
#endif

    size = (size_t)*cursor;
    cursor++;

#if TRACE_PACKFILE
    printf("PackFile_Constant_unpack_string(): size is %ld...\n", size);
#endif

    self->type = PFC_STRING;
    if (encoding == 0) {
        self->string = string_make(interpreter, cursor, size, NULL, flags, NULL);       /* fixme */
        /* This is a constant string */
        self->string->flags |= BUFFER_constant_FLAG;
    }
    else if (encoding == 3) {
        self->string = string_make(interpreter, cursor, size, encoding_lookup("utf32"), flags, chartype_lookup("unicode"));     /* fixme */
        /* This is a constant string */
        self->string->flags |= BUFFER_constant_FLAG;
    }
    else {
        return 0;
    }

    return 1;
}


/***************************************

=item pack_size

Determine the size of the buffer needed in order to pack the PackFile Constant into a
contiguous region of memory.

=cut

***************************************/

opcode_t
PackFile_Constant_pack_size(struct PackFile_Constant *self)
{
    opcode_t packed_size;
    opcode_t padded_size;

    if (!self) {
        /* TODO: OK to gloss over this? */
        return (opcode_t)0;
    }

    switch (self->type) {
    case PFC_NONE:
        packed_size = 0;
        break;

    case PFC_INTEGER:
        packed_size = sizeof(opcode_t);
        break;

    case PFC_NUMBER:
        packed_size = sizeof(FLOATVAL); /* XXX need to pad this? */
        break;

    case PFC_STRING:
        padded_size = self->string->bufused;

        if (padded_size % sizeof(opcode_t)) {
            padded_size += sizeof(opcode_t) - (padded_size % sizeof(opcode_t));
        }

        /* Include space for flags, encoding, type, and size fields.  */
        packed_size = 4 * sizeof(opcode_t) + padded_size;
        break;

    default:
        packed_size = 0;
        break;
    }

    /* Tack on space for the initial type and size fields */
    if (packed_size) {
        return packed_size + 2 * sizeof(opcode_t);
    }
    else {
        return 0;
    }
}


/***************************************

=item pack

Pack the PackFile Constant into a contiguous region of memory. NOTE: The memory
block had better have at least the amount of memory indicated by
PackFile_Constant_pack_size()!

The data is zero-padded to an opcode_t-boundary, so pad bytes may be added.
(Note this padding is not yet implemented for FLOATVALs.)

=cut

***************************************/

void
PackFile_Constant_pack(struct PackFile_Constant *self, opcode_t *packed)
{
    opcode_t *cursor;
    char *charcursor;
    size_t i;
    opcode_t padded_size;
    opcode_t packed_size;

    if (!self) {
        /* TODO: OK to be silent here? */
        return;
    }

    cursor = packed;

    *cursor = self->type;
    cursor++;

    switch (self->type) {
    case PFC_NONE:
        *cursor = 0;
        cursor++;

        /* TODO: OK to be silent here? */
        break;

    case PFC_INTEGER:
        *cursor = sizeof(opcode_t);
        cursor++;

        *cursor = self->integer;
        cursor++;
        break;

    case PFC_NUMBER:
        *cursor = sizeof(FLOATVAL);
        cursor++;
        /* XXX Use memcpy() to avoid alignment issues.
         * Also, do we need to pad things out to an opcode_t boundary?  
         * Consider gcc/x86, with opcode_t = (long long) and 
         * FLOATVAL = (long double):
         * sizeof(long long) = 8
         * sizeof(long double) = 12
         */
        mem_sys_memcopy(cursor, &self->number, sizeof(FLOATVAL));
        cursor += sizeof(FLOATVAL) / sizeof(opcode_t);  /* XXX */
        /* XXX cursor is possibly wrong now (because of alignment
         * issues) but isn't returned from this function anyway!
         */
        break;

    case PFC_STRING:
        padded_size = self->string->bufused;

        if (padded_size % sizeof(opcode_t)) {
            padded_size += sizeof(opcode_t) - (padded_size % sizeof(opcode_t));
        }

        /* Include space for flags, encoding, type, and size fields.  */
        packed_size = 4 * sizeof(opcode_t) + padded_size;

        *cursor = packed_size;
        cursor++;

        *cursor = self->string->flags;
        cursor++;

        if (strcmp(self->string->type->name, "usascii") == 0 &&
            strcmp(self->string->encoding->name, "singlebyte") == 0) {
            *cursor = 0;        /* fixme */
        }
        else if (strcmp(self->string->type->name, "unicode") == 0 &&
                 strcmp(self->string->encoding->name, "utf32") == 0) {
            *cursor = 3;        /* fixme */
        }
        cursor++;

        *cursor = 0;            /* fixme */
        cursor++;

        *cursor = self->string->bufused;
        cursor++;

        /* Switch to char * since rest of string is addressed by
         * characters to ensure padding.  */
        charcursor = (char *)cursor;

        if (self->string->bufstart) {
            mem_sys_memcopy(charcursor, self->string->bufstart,
                            self->string->bufused);
            charcursor += self->string->bufused;

            if (self->string->bufused % sizeof(opcode_t)) {
                for (i = 0;
                     i <
                     (sizeof(opcode_t) -
                      (self->string->bufused % sizeof(opcode_t))); i++) {
                    charcursor[i] = 0;
                }
            }
        }
        /* If cursor is needed below, uncomment the following and
         * ignore the gcc -Wcast-align warning.  charcursor is
         * guaranteed to be aligned correctly by the padding logic
         * above.
         * cursor = (opcode_t *) charcursor;
         */
        break;

    default:
        /* TODO: OK to be silent here? */
        break;
    }

    return;
}


/***************************************

=item dump

Dump the PackFile Constant to standard output.

=cut

***************************************/

void
PackFile_Constant_dump(struct PackFile_Constant *self)
{
    if (!self) {
        /* TODO: OK to be silent here? */
        return;
    }

    switch (self->type) {
    case PFC_NONE:
        /* TODO: OK to be silent here? */
        printf("    [ 'PFC_NONE', undef ],\n");
        break;

    case PFC_INTEGER:
        printf("    [ 'PFC_INTEGER', %ld ],\n", (long)self->integer);
        break;

    case PFC_NUMBER:
        printf("    [ 'PFC_NUMBER', %g ],\n", self->number);
        break;

    case PFC_STRING:
        printf("    [ 'PFC_STRING', {\n");
        printf("        FLAGS    => 0x%04lx,\n",
               (unsigned long)self->string->flags);
        printf("        ENCODING => %s,\n", self->string->encoding->name);
        printf("        TYPE     => %s,\n", self->string->type->name);
        printf("        SIZE     => %ld,\n", (long)self->string->bufused);
        /* TODO: Won't do anything reasonable for most encodings */
        printf("        DATA     => '%.*s'\n",
               (int)self->string->bufused, (char *)self->string->bufstart);
        printf("    } ],\n");
        break;

    default:
        /* TODO: OK to be silent here? */
        break;
    }

    return;
}



/*

=back

=cut

*/

/*
* Local variables:
* c-indentation-style: bsd
* c-basic-offset: 4
* indent-tabs-mode: nil
* End:
*
* vim: expandtab shiftwidth=4:
*/
