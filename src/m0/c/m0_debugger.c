/*
Copyright (C) 2011-2012, Parrot Foundation.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/m0_ops.h"
#include "include/m0_mob_structures.h"
#include "include/m0_interp_structures.h"
#include "include/m0_compiler_defines.h"
#include "include/m0_debugger.h"

const char *M0_OPS_NAMES[] = {
    /* gen_c_opnames_enum_from(m0.ops) template('    "M0_UC_OP",') */
    "M0_NOOP",
    "M0_GOTO",
    "M0_GOTO_IF",
    "M0_GOTO_CHUNK",
    "M0_ADD_I",
    "M0_ADD_N",
    "M0_SUB_I",
    "M0_SUB_N",
    "M0_MULT_I",
    "M0_MULT_N",
    "M0_DIV_I",
    "M0_DIV_N",
    "M0_MOD_I",
    "M0_MOD_N",
    "M0_CONVERT_I_N",
    "M0_CONVERT_N_I",
    "M0_ASHR",
    "M0_LSHR",
    "M0_SHL",
    "M0_AND",
    "M0_OR",
    "M0_XOR",
    "M0_GC_ALLOC",
    "M0_SYS_ALLOC",
    "M0_SYS_FREE",
    "M0_COPY_MEM",
    "M0_SET",
    "M0_SET_IMM",
    "M0_DEREF",
    "M0_SET_REF",
    "M0_SET_BYTE",
    "M0_GET_BYTE",
    "M0_SET_WORD",
    "M0_GET_WORD",
    "M0_CSYM",
    "M0_CCALL_ARG",
    "M0_CCALL_RET",
    "M0_CCALL",
    "M0_PRINT_S",
    "M0_PRINT_I",
    "M0_PRINT_N",
    "M0_EXIT"
    /* end_gen */
};

static char *M0_REGISTER_NAMES[] = {
    "CF",       /*  0 */
    "PCF",      /*  1 */
    "PC",       /*  2 */
    "RETPC",    /*  3 */
    "EH",       /*  4 */
    "CHUNK",    /*  5 */
    "CONSTS",   /*  6 */
    "MDS",      /*  7 */
    "BCS",      /*  8 */
    "INTERP",   /*  9 */
    "SPC4RENT", /* 10 */
    "SPILLCF",  /* 11 */
    "I00",
    "I01",
    "I02",
    "I03",
    "I04",
    "I05",
    "I06",
    "I07",
    "I08",
    "I09",
    "I10",
    "I11",
    "I12",
    "I13",
    "I14",
    "I15",
    "I16",
    "I17",
    "I18",
    "I19",
    "I20",
    "I21",
    "I22",
    "I23",
    "I24",
    "I25",
    "I26",
    "I27",
    "I28",
    "I29",
    "I30",
    "I31",
    "I32",
    "I33",
    "I34",
    "I35",
    "I36",
    "I37",
    "I38",
    "I39",
    "I40",
    "I41",
    "I42",
    "I43",
    "I44",
    "I45",
    "I46",
    "I47",
    "I48",
    "I49",
    "I50",
    "I51",
    "I52",
    "I53",
    "I54",
    "I55",
    "I56",
    "I57",
    "I58",
    "I59",
    "I60",
    "N00",
    "N01",
    "N02",
    "N03",
    "N04",
    "N05",
    "N06",
    "N07",
    "N08",
    "N09",
    "N10",
    "N11",
    "N12",
    "N13",
    "N14",
    "N15",
    "N16",
    "N17",
    "N18",
    "N19",
    "N20",
    "N21",
    "N22",
    "N23",
    "N24",
    "N25",
    "N26",
    "N27",
    "N28",
    "N29",
    "N30",
    "N31",
    "N32",
    "N33",
    "N34",
    "N35",
    "N36",
    "N37",
    "N38",
    "N39",
    "N40",
    "N41",
    "N42",
    "N43",
    "N44",
    "N45",
    "N46",
    "N47",
    "N48",
    "N49",
    "N50",
    "N51",
    "N52",
    "N53",
    "N54",
    "N55",
    "N56",
    "N57",
    "N58",
    "N59",
    "N60",
    "S00",
    "S01",
    "S02",
    "S03",
    "S04",
    "S05",
    "S06",
    "S07",
    "S08",
    "S09",
    "S10",
    "S11",
    "S12",
    "S13",
    "S14",
    "S15",
    "S16",
    "S17",
    "S18",
    "S19",
    "S20",
    "S21",
    "S22",
    "S23",
    "S24",
    "S25",
    "S26",
    "S27",
    "S28",
    "S29",
    "S30",
    "S31",
    "S32",
    "S33",
    "S34",
    "S35",
    "S36",
    "S37",
    "S38",
    "S39",
    "S40",
    "S41",
    "S42",
    "S43",
    "S44",
    "S45",
    "S46",
    "S47",
    "S48",
    "S49",
    "S50",
    "S51",
    "S52",
    "S53",
    "S54",
    "S55",
    "S56",
    "S57",
    "S58",
    "S59",
    "S60",
    "P00",
    "P01",
    "P02",
    "P03",
    "P04",
    "P05",
    "P06",
    "P07",
    "P08",
    "P09",
    "P10",
    "P11",
    "P12",
    "P13",
    "P14",
    "P15",
    "P16",
    "P17",
    "P18",
    "P19",
    "P20",
    "P21",
    "P22",
    "P23",
    "P24",
    "P25",
    "P26",
    "P27",
    "P28",
    "P29",
    "P30",
    "P31",
    "P32",
    "P33",
    "P34",
    "P35",
    "P36",
    "P37",
    "P38",
    "P39",
    "P40",
    "P41",
    "P42",
    "P43",
    "P44",
    "P45",
    "P46",
    "P47",
    "P48",
    "P49",
    "P50",
    "P51",
    "P52",
    "P53",
    "P54",
    "P55",
    "P56",
    "P57",
    "P58",
    "P59",
    "P60"
};

static void
NYI() {
    printf("NOT YET IMPLEMENTED\n");
}

M0_Debugger_Command
str_to_db_cmd(char* str) {
    M0_Debugger_Command cmd = Invalid;

    if(strcmp(str,"c") == 0)
        cmd = Continue;
    else if(strcmp(str, "s") == 0)
        cmd = Step;
    else if(strcmp(str, "p") == 0)
        cmd = Print;
    else if(strcmp(str, "pi") == 0)
        cmd = Print_Integer;
    else if(strcmp(str, "pn") == 0)
        cmd = Print_Number;
    else if(strcmp(str, "ps") == 0)
        cmd = Print_String;
    else if(strcmp(str, "l") == 0)
        cmd = List;
    else if(strcmp(str, "b") == 0)
        cmd = Add_Breakpoint;
    else if(strcmp(str, "B") == 0)
        cmd = Delete_Breakpoint;
    else if(strcmp(str, "L") == 0)
        cmd = List_Breakpoints;
    else if(strcmp(str, "h") == 0)
        cmd = Help;
    else if(strcmp(str, "") == 0)
        cmd = None;
    return cmd;
}

char * get_line(char* str, size_t size, FILE *f) {
    char *p = fgets(str, size, f);

    if(p) {
        size_t last = strlen (str) - 1;
        if (str[last] == '\n')
            str[last] = '\0';
    }
    if(0 == strlen(str)) {
        free(p);
        p = NULL;
    }
    return p;
}

char *
get_script_file_input(char *filename, char *input) {
    char *p;
    static FILE *pFile = NULL;

    if(pFile == NULL) {
        pFile = fopen(filename, "r");
    }

    if(pFile == NULL)
        perror("Error opening file");

    p = get_line(input, 100, pFile);

    if(feof(pFile)) {
        fclose(pFile);
        return NULL;
    }
    return p;
}

char *
get_db_input(M0_Debugger_Info *db_info, M0_Debugger_Command *cmd, char *input) {
    char *p;

    if(db_info->input_source)
        p = get_script_file_input(db_info->input_source, input);
    else
        p = get_line(input, 100, stdin);

    if (p) {
        char *arg = NULL;
        char *tok = NULL;

        if(db_info->input_source != NULL)
            printf("%s\n", input);

        tok = strtok(input, " ");
        if(tok)
            *cmd = str_to_db_cmd(tok);
        else
            return input;

        tok = strtok(NULL, " ");
        if(tok) {
            arg = (char*)realloc(arg, (strlen(tok)+1) * sizeof(char));
            if(!arg)
                perror("Could not allocate memory for arg");
            strcpy(arg,tok);
        }
        return arg;
    }
    return NULL;
}

const char * ops_to_name(const unsigned char ops) {
    unsigned int op_num  = (unsigned int) ops;
    const char * op_name = M0_OPS_NAMES[op_num];
    return op_name;
}

char * register_to_name(const unsigned char reg) {
    unsigned int reg_num  = (unsigned int) reg;
    char * reg_name = M0_REGISTER_NAMES[reg_num];
    return reg_name;
}

unsigned int name_to_register_id(const char *name) {
    unsigned int reg = -1;
    unsigned int i   =  0;
    for ( ; i < 256; i++) {
        if (strcmp(name, M0_REGISTER_NAMES[i]) == 0) {
            reg = i;
            break;
        }
    }
    return reg;
}

static void
debug_print(M0_Debugger_Command cmd, M0_CallFrame *cf, char *arg)
{
    int reg = name_to_register_id(arg);
    unsigned long *a, *b;
    switch(cmd) {
      case Print_String:
        printf("%s\n", (char *)(cf->registers[ reg ] + 8));
        break;
      case Print_Integer:
        printf("%d\n", (unsigned int)(cf->registers[ reg ] ));
        break;
      case Print_Number:
        printf("%f\n", (float)(cf->registers[ reg ] ));
        break;
      case Print:
      default:
        a =  (unsigned long*)&cf->registers[ reg ];
        b = a + 1;
        printf("0x%lX%lX\n", *b, *a);
    }
}

static void
debug_list(M0_CallFrame *cf, const unsigned char *ops, const unsigned long pc)
{
    const char *op_name = ops_to_name(ops[pc*4]);
    char       *arg1    = register_to_name(ops[pc*4+1]);
    char       *arg2    = NULL;
    char       *arg3    = NULL;
    UNUSED(cf);

    switch (ops[pc*4]) {
        case M0_GOTO_IF:
        case M0_GOTO:
        case M0_GOTO_CHUNK:
            arg1 = calloc(10,sizeof(char));
            sprintf(arg1, "%d", (int)(ops[pc*4+1]));
        case M0_SET_IMM:
            arg2 = calloc(10,sizeof(char));
            arg3 = calloc(10,sizeof(char));
            sprintf(arg2, "%d", (int)(ops[pc*4+2]));
            sprintf(arg3, "%d", (int)(ops[pc*4+3]));
            break;
        default:
            arg2 = register_to_name(ops[pc*4+2]);
            arg3 = register_to_name(ops[pc*4+3]);
    }
    printf("%s\t%s,%s,%s\n", op_name, arg1, arg2, arg3);
}

static void
debug_add_breakpoint(char * arg, M0_Debugger_Info* db_info)
{
    unsigned long bp;
    unsigned int  n_bp;
    if(!arg) {
        printf("You must specify a PC in order to add a breakpoint\n");
        return;
    }
    bp = strtoul(arg, NULL, 10);
    n_bp = ++(db_info->n_breakpoints);
    db_info->breakpoints = realloc(db_info->breakpoints, n_bp*sizeof(unsigned long));
    db_info->breakpoints[n_bp-1] = bp;
}

static void
debug_delete_breakpoint(char * arg, M0_Debugger_Info* db_info)
{
    unsigned long bp;
    unsigned long n_bp;
    if(!arg) {
        bp = 0;
        db_info->n_breakpoints = n_bp = 0;
        db_info->breakpoints = realloc(db_info->breakpoints, n_bp*sizeof(unsigned long));
    } else {
        n_bp = --(db_info->n_breakpoints);
        bp = strtoul(arg, NULL, 10);
        if(bp < db_info->n_breakpoints) {
            for( ; bp < db_info->n_breakpoints; bp++)
                db_info->breakpoints[bp] = db_info->breakpoints[bp+1];
        }
        db_info->breakpoints = realloc(db_info->breakpoints, n_bp*sizeof(unsigned long));
    }
}

static void
debug_list_breakpoints(char * arg, M0_Debugger_Info* db_info)
{
    unsigned int n_bp = db_info->n_breakpoints;
    unsigned int i = 0;

    UNUSED(arg);
    printf("There are %d breakpoint(s)\n", n_bp);
    for( ; i < n_bp; i++)
        printf("Breakpoint #%u:\tPC=%lu\n", i, db_info->breakpoints[i]);
}

static void
print_help()
{
    printf("Available Commands:\n");
    printf("\tc     : continue until the next breakpoint or the end of the program\n");
    printf("\ts     : single step (execute the next m0 command)\n");
    printf("\tp  ARG: print ARG (treat ARG as hex)\n\t\tCurrently ARG only supports registers\n");
    printf("\tpi ARG: print ARG (treat ARG as an unsigned integer)\n\t\tCurrently ARG only supports registers\n");
    printf("\tpn ARG: print ARG (treat ARG as a float)\n\t\tCurrently ARG only supports registers\n");
    printf("\tps ARG: print ARG (treat ARG as a string)\n\t\tCurrently ARG only supports registers\n");
    printf("\tl     : list the decompiled source code for the line that is about to be executed\n");
    printf("\tb PC  : create a new breakpoint at PC\n");
    printf("\tB ARG : delete the breakpoint # ARG\n");
    printf("\tB     : delete all breakpoints\n");
    printf("\tL     : list breakpoints\n");
    printf("\th     : print this help message\n");
}

static void
db_prompt(M0_Debugger_Info *db_info, M0_CallFrame *cf, const unsigned char *ops, const unsigned long pc)
{
    static M0_Debugger_Command  last_cmd   = None;
    static char                *last_arg   = NULL;
    int                         done       = 0;
    char                       *user_input = calloc(100, sizeof(char));

    while(!done) {
        M0_Debugger_Command  cmd = None;
        char                *arg = NULL;
        printf("PC=%lu> ", pc);
        arg = get_db_input(db_info, &cmd, user_input);
        if(cmd == None) {
            cmd = last_cmd;
            arg = last_arg;
        }
        else {
            last_cmd = cmd;
            last_arg = arg;
        }
        switch (cmd) {
            case Continue:
                if(db_info->n_breakpoints > 0)
                    db_info->state = BREAK;
                else
                    db_info->state = RUN;
                done = 1;
                break;
            case Step:
                db_info->state = STEP;
                done = 1;
                break;
            case Print:
            case Print_Integer:
            case Print_Number:
            case Print_String:
                debug_print(cmd, cf, arg);
                break;
            case List:
                debug_list(cf, ops, pc);
                break;
            case Add_Breakpoint:
                debug_add_breakpoint(arg, db_info);
                break;
            case Delete_Breakpoint:
                debug_delete_breakpoint(arg, db_info);
                break;
            case List_Breakpoints:
                debug_list_breakpoints(arg, db_info);
                break;
            case Help:
                print_help();
                break;
            case None:
                break;
            default:
                printf("bad command: %s\n", user_input);
                printf("type 'h' for help\n");
        }
    }
    free(user_input);
}

int
check_breakpoints(M0_Debugger_Info *db_info, const unsigned long pc) {
    unsigned int i        = 0;
    unsigned int bp_found = 0;
    for(; i < db_info->n_breakpoints; i++)
        if(db_info->breakpoints[i] == pc) {
            bp_found = 1;
            break;
        }
    return bp_found;
}

static void
parse_argv(M0_Debugger_Info *db_info, int argc, const char* argv[])
{
    if(argc > 2) {
        int i = 2;
        while(i < argc) {
            if(strcmp("-s",argv[i]) == 0) {
                size_t size = strlen(argv[++i]);
                db_info->input_source = malloc((size+1)*sizeof(char));
                strcpy(db_info->input_source, argv[i]);
            }
            i++;
        }
    }
    db_info->state = STEP;
}

void
debugger(int argc, const char* argv[], M0_Interp *interp, M0_CallFrame *cf, const unsigned char *ops, const unsigned long pc)
{
    static M0_Debugger_Info db_info;
    UNUSED(interp);

    switch (db_info.state) {
      case INIT:
            parse_argv(&db_info, argc, argv);
            db_info.n_breakpoints = 0;
            break;
        case STEP:
            db_prompt(&db_info, cf, ops, pc);
            break;
        case BREAK:
            if (1 == check_breakpoints(&db_info, pc))
                db_prompt(&db_info, cf, ops, pc);
            break;
        case RUN:
            break;
        default:
            break;
    }
}

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4 cinoptions='\:2=2' :
 */