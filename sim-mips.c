/* ECE 353 Lab 3
 * Authors: Vincent Nguyen
 *			Ivan Norman
 *			Alessy LeBlanc
 *			Aleck Chen
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
//feel free to add here any additional library names you may need
#define SINGLE 1
#define BATCH 0
#define REG_NUM 32

enum OP { addOPCODE,addiOPCODE,subOPCODE,mulOPCODE,beqOPCODE,lwOPCODE,swOPCODE,nopOPCODE };

enum REGISTERS {
    rgZERO, rgAT, rgV0, rgV1, rgA0, rgA1, rgA2, rgA3,
    rgT0, rgT1, rgT2, rgT3, rgT4, rgT5, rgT6, rgT7,
    rgS0, rgS1, rgS2, rgS3, rgS4, rgS5, rgS6, rgS7,
    rgT8, rgT9, rgK0, rgK1, rgGP, rgSP, rgFP, rgRA
};

struct inst { int opcode, rd, rs, rt, Imm; };

struct latch { int opcode, rd, rs, rt, Imm, result; };
struct latch IF_ID_LATCH, ID_EX_LATCH, EX_MEM_LATCH, MEM_WB_LATCH; // latches for pipeline

long reg[REG_NUM]; // array of the registers
int numI = 0; /* number of instructions stored */
struct inst *Imemory;
int MEMdata[2048];

long st_cycle = 0;  /* counts cycles of each stage */
long sim_cycle = 0;     /* simulation cycle counter */
long pgm_c = 0;     /* program counter */

int IF_ID_flag = 0; /* latch flag between IF and ID */
int ID_EX_flag = 0; /* latch flag between ID and EX */
int EX_MEM_flag = 0; /* latch flag between EX and MEM */
int MEM_WB_flag = 0; /* latch flag between MEM and WB */
int WB_flag = 0; /* flag for WB stage */

int nops = 0; /* counts the number of nops in the pipeline */

int c;//cycles for memory access
int m;//cycles for multiplication operations
int n;//cycles for other intructions

double IFutil, IDutil, EXutil, MEMutil, WButil; //stage util %
long IFused, IDused, EXused, MEMused, WBused; //stage useful

/*
This reads as input a pointer to a string holding the next
line from the assembly language program, using the fgets() library function to do
so. progScanner() removes all duplicate spaces, parentheses, and commas from
it from it and a pointer to the resulting character string will be returned. Items
will be separated in this character string solely by a single space. For example
add $s0, $s1, $s2 will be transformed to add $s0 $s1 $s2. The instruction
lw $s0, 8($t0) will be converted to lw $s0 8 $t0. If, in a load or store instruction,
mismatched parentheses are detected (e.g., 8($t0( instead of 8($t0) or a missing
or extra ), ), this should be reported and the simulation should then stop.
In this simulator, we will assume that consecutive commas with nothing in between
(e.g., ,,) are a typo for a single comma, and not flag them as an error; such
consecutive commas will be treated as a single comma.
*/
char *mystrchr (const char *s, int c) {
    for (; *s != '\0'; ++s)
        if (*s == c)
            return (char *) s;

    return NULL;
}
// an implementation of strcat bceause quark wont work
char* strcatA(char* dest_ptr, char const* src_ptr)
{
    char* strret = dest_ptr;
    if((NULL != dest_ptr) && (NULL != src_ptr))
    {
        /* Iterate till end of dest string */
        while('\0' != *dest_ptr)
        {
            dest_ptr++;
        }
        /* Copy src string starting from the end NULL of dest */
        while('\0' != *src_ptr)
        {
            *dest_ptr++ = *src_ptr++;
        }
        /* put NULL termination */
        *dest_ptr = '\0';
    }
    return strret;
}
char *progScanner(FILE *f)
{
    char *mystrchr (const char *s, int c);
    char* strcatA(char* dest_ptr, char const* src_ptr);
    char *entry;
    //FILE *ifp;
    char *parsed = malloc(200*sizeof(char));
    entry = malloc(200*sizeof(char));
    //ifp = fopen("C:\\Users\\inorm\\CLionProjects\\untitled\\input.txt", "r");// Input was already defined in the main method
    // just pass it in here. No need to read twice
    //while (fgets(entry, 75,f));
    if(!fgets(entry,100,f)) return (char* )NULL;
    char *try;
    //char *copy= strdup(entry);
    int comcount;
    char* filt;
    if(filt=mystrchr(entry,'\n'))*filt='\0';
    if(filt=mystrchr(entry,'\r'))*filt='\0';
    if(filt=mystrchr(entry,'\t'))*filt=' ';
    char *copy= strdup(entry);
    while(copy[comcount]!=NULL)// checkig for at least one space after a comma
    {
        if(copy[comcount]==',')
        {
            assert(copy[comcount+1]==' ');
        }
        comcount++;
    }
    int maxparams=0;
    try=strtok(entry," ");
    // before going into da loop check for space

    while(try!=NULL)
    {   char *temp2= malloc((strlen(try)+2)*sizeof(char));
        temp2[0]= '\0';
        char *commapos = mystrchr(try,',');
        char * rightpos = mystrchr(try,')');// checks if the end is there
        char * leftpos = mystrchr(try,'(');// checks if the begging parenthesis is there

        if(strcmp(try," ")==0){ continue;}// we already split by space
        else if (commapos!=NULL) // if we see a comma we only want the first one
        {
            assert(try[0]=='$');/// this checks if there is a dollar sign
            assert(try[0]=='$');// this makes sure that there is at least on space after
            try[commapos-try]=NULL;// MAKE ALL COMMAS BUL
            strcatA(parsed, strcatA(strcatA(temp2,try)," "));
            maxparams++;
        }
        else if (leftpos||rightpos) // we foudn the beggining of the right side
        {
            if(leftpos&&rightpos&&try[(leftpos-try)+1]=='$'&& try[(rightpos-try)+1]==NULL) {
                try[leftpos - try] =' ';
                try[rightpos - try] = NULL;
                strcatA(parsed, strcatA(strcatA(temp2,try)," "));
                maxparams++;
            }
            else exit(1); // Mis match oof other stuff}
        }
        else {strcatA(parsed,strcatA(strcatA(temp2,try)," "));maxparams++;}
        try=strtok(NULL," ");
    }
    assert(maxparams<=4);
    parsed[strlen(parsed)-1]='\0';
    //parsed[strlen(parsed)-1]='\0';

    free(entry);
    return parsed;
}
/*
This function accepts as input the output of
the progScanner() function and returns a pointer to a character string in which all
register names are converted to numbers.
MIPS assembly allows you to specify either the name or the number of a register.
For example, both $zero and $0 are the zero register; $t0 and $8 both refer to register
8,and so on. Your parser should be able to handle either representation. (Use the
table of registers in Hennessy and Patterson or look up the register numbers online.)
The code scans down the string looking for the $ delimiter. Whatever is to the right
of this (and to the left of a space or the end of string) is the label or number of the
register. If the register is specified as a number (e.g., $5), then the $ is stripped and
5 is left behind. If it is specified as a register name (e.g., $s0), the name is replaced
by the equivalent register number). If an illegal register name is detected (e.g., $y5)
or the register number is out of bounds (e.g., $987), an error is reported and the
simulator halts.
*/
char **regNumberConverter(char *parsed)
{

    // char *res2[200];
    //strlen(parsed)+2
    char *mystrchr (const char *s, int c);
    char* strcatA(char* dest_ptr, char const* src_ptr);
    char **result=malloc(4*sizeof(char*));//Leave a lil more space thn we need
    int pos=0;
    char *p;
    p=strtok(parsed," ");
    while (p!= NULL)
    { char *temp2= malloc(5*sizeof(char)); // max 2 characers + space + null sec
        temp2[0]= '\0';
        char *val=malloc(3*sizeof(char));



        if(mystrchr(p,'$')!=NULL)
        {
            assert(p[0]=='$');/// this checks if there is a dollar sign
            assert(p[1]!='$');/// cehck for double dollar sign
            //assert(p[1]!='$');/// cehck for double dollar sign
            char *substr = p+1;
            if (strcmp(substr,"zero")==0){val[0]='0';val[1]= '\0';}
            else if (strcmp(substr,"at")==0){val[0]='1';val[1]= '\0';}
            else if (strcmp(substr,"t8")==0){val[0]='2';val[1]='4';val[2]= '\0';}
            else if (strcmp(substr,"t9")==0){val[0]='2';val[1]='5';val[2]= '\0';}
            else if (strcmp(substr,"gp")==0){val[0]='2';val[1]='8';val[2]= '\0'; }
            else if (strcmp(substr,"sp")==0){val[0]='2';val[1]='9';val[2]= '\0';}
            else if (strcmp(substr,"fp")==0){val[0]='3';val[1]='0';val[2]= '\0';}
            else if (strcmp(substr,"ra")==0){val[0]='3';val[1]='1';val[2]= '\0';}
            else if (substr[0]=='a'&&((int)strtol(substr+1,NULL,0)<=3 &&(int)strtol(substr+1,NULL,0)>=0)){sprintf(val,"%i",(int)strtol(substr+1,NULL,0)+4);}
            else if (substr[0]=='t'&&((int)strtol(substr+1,NULL,0)<=7 &&(int)strtol(substr+1,NULL,0)>=0)){sprintf(val,"%i",(int)strtol(substr+1,NULL,0)+8);}
            else if (substr[0]=='s'&&((int)strtol(substr+1,NULL,0)<=7 &&(int)strtol(substr+1,NULL,0)>=0)){sprintf(val,"%i",(int)strtol(substr+1,NULL,0)+16); }
            else if (substr[0]=='k'&&((int)strtol(substr+1,NULL,0)<=1 &&(int)strtol(substr+1,NULL,0)>=0)){sprintf(val,"%i",(int)strtol(substr+1,NULL,0)+26);}
            else if (((int)strtol(substr,NULL,0)<=31 &&(int)strtol(substr,NULL,0)>0) || strcmp(substr,"0")==0){
                int lou;
                for(lou=0; lou<strlen(substr);lou++)
                {val[lou]=substr[lou];}

            } //strcatA(temp2,val)
            else
                exit(1);// AT THIS POINT EVERY CONDITION IS FALSE . STOP THE PROGRAM

            result[pos++]=val;


        }
        else
        {
            result[pos++]=p;
            //if (((int)strtol(p,NULL,0)>0 || strcmp(p,"0")==0))//cheking if its a number in ascii
            //{ continue;}
            //strcatA(result,p);
        }
        p = strtok (NULL, " ");
    }

    return result;
}

/*
This function uses the output of regNumberConverter().
The instruction is returned as an inst struct with fields for each of the fields of MIPS
assembly instructions, namely opcode, rs, rt, rd, Imm. Of course, not all the fields
will be present in all instructions; for example, beq will have just two register and
one Imm fields.
Each of the fields of the inst struct will be an integer. You should use
the enumeration type to conveniently describe the opcodes, e.g., enum inst
{ADD,ADDI,SUB,MULT,BEQ,LW,SW}. You can assume that the assembly language
instructions are written in lower-case so there is no clash with these enum quantities.
If an illegal opcode is detected (e.g., a typo such as sbu instead of sub) or an error
is detected in any other field, the error is reported and the simulation is stopped.
The error type should be reported; these error types are:
� Illegal opcode (see the example above).
� Immediate field that contains a number too large to store in the 16 bits that
are available in the machine code format.
� Missing $ in front of a register name (e.g., a program which says s0 instead of
$s0).
Also, the simulator should flag an error and stop further activity whenever a misaligned
memory access occurs. Recall from ECE 232 that a misaligned integer access
is one where the memory address of the lw or sw source or target is not a multiple
of 4.
The above are the only error conditions you are required to catch.
4
The function parser() will place the parsed instruction (in the form of a struct) in
the linear array that is used to represent the Instruction Memory.
*/
int checkMult4(int x)
{
    if(x%4 == 0)
        return 1;
    else
        return 0;
}

struct inst parser(char **instr){
    struct inst i;
    /*
    char* token[2];
    char* copy = strdup(instr);
	token[0] = strtok(copy, " ");
	int x = 0;
	while(token[x] != NULL)
	{
		x++;
		token[x] = strtok(NULL, " ");
	}

    char* regResult = regNumberConverter(instr);
    char* reg[3];
	reg[0] = strtok(regResult, " ");
	x = 0;
	while(reg[x] != NULL)
	{
		reg[x++] = strtok(NULL, " ");
	}
	/*
	 FOR R-TYPE: (ADD, SUB, MUL)
	 token[0] = opcode
	 token[1] = rd
	 token[2] = rs
	 token[3] = rt

     FOR I-TYPE: (ADDI, LW)
     token[0] = opcode
     token[1] = rt
     token[2] = rs
     token[3] = imm(signextnd)

     FOR SW
     token[0] = opcode
     token[1] = rs
     token[2] = signext
     token[3] = rt

     FOR BEQ
     token[0] = opcode
     token[1] = rs
     token[2] = rt
     token[3] = imm(signextnd)
	*/

    if(strcmp(instr[0], "add") == 0){
        i.opcode = addOPCODE;
        //printf("%s\n", instr[1]);
        //checkDollarSign(instr[1]);
        i.rd = atoi(instr[1]);
        //checkDollarSign(instr[2]);
        i.rs = atoi(instr[2]);
        //checkDollarSign(instr[3]);
        i.rt = atoi(instr[3]);
    }
    else if(strcmp(instr[0], "addi") == 0){
        i.opcode = addiOPCODE;
        //checkDollarSign(instr[1]);
        i.rt = atoi(instr[1]);
        //checkDollarSign(instr[2]);
        i.rs = atoi(instr[2]);
        //assert(checkMult4(atoi(instr[3])) == 1);
        assert(atoi(instr[3]) >= 0);
        i.Imm = atoi(instr[3]);
        assert(i.Imm <= (unsigned) 0xFFFF);
    }
    else if(strcmp(instr[0], "sub") == 0){
        i.opcode = subOPCODE;
        //checkDollarSign(instr[1]);
        i.rd = atoi(instr[1]);
        //checkDollarSign(instr[2]);
        i.rs = atoi(instr[2]);
        //checkDollarSign(instr[3]);
        i.rt = atoi(instr[3]);
    }
    else if(strcmp(instr[0], "mul") == 0){
        i.opcode = mulOPCODE;
        //checkDollarSign(instr[1]);
        i.rd = atoi(instr[1]);
        //checkDollarSign(instr[2]);
        i.rs = atoi(instr[2]);
        //checkDollarSign(instr[3]);
        i.rt = atoi(instr[3]);
    }
    else if(strcmp(instr[0], "beq") == 0){
        i.opcode = beqOPCODE;
        //checkDollarSign(instr[1]);
        i.rs = atoi(instr[1]);
        //checkDollarSign(instr[2]);
        i.rt = atoi(instr[2]);
        //assert(checkMult4(atoi(instr[3])) == 1);
        assert(atoi(instr[3]) >= 0);
        i.Imm = atoi(instr[3]);

        assert(i.Imm <= (unsigned) 0xFFFF);
    }
    else if(strcmp(instr[0], "lw") == 0){
        i.opcode = lwOPCODE;
        //checkDollarSign(instr[1]);
        i.rt = atoi(instr[1]);
        assert(checkMult4(atoi(instr[2])) == 1);
        //assert(atoi(instr[2]) >= 0);
        i.Imm = atoi(instr[2]);
        //checkDollarSign(instr[3]);
        i.rs = atoi(instr[3]);

        assert(i.Imm <= (unsigned) 0xFFFF);
    }
    else if(strcmp(instr[0], "sw") == 0){
        i.opcode = swOPCODE;
        //checkDollarSign(instr[1]);
        i.rs = atoi(instr[1]);
        checkMult4(atoi(instr[2]) == 1);
        assert(atoi(instr[2]) >= 0);
        i.Imm = atoi(instr[2]);
        //checkDollarSign(instr[3]);
        i.rt = atoi(instr[3]);

        assert(i.Imm <= (unsigned) 0xFFFF);
    }
    else
        exit(1);
    return i;
}
/*
These functions simulate activity in each of the five pipeline stages. All data,
structural, and control hazards must be taken into account. Keep in mind that
several operations are multicycle and that these stages are themselves not
pipelined. For example, if an add takes 4 cycles, the next instruction cannot
enter EX until these cycles have elapsed. This, in turn, can cause IF to be
blocked by ID. Branches will be resolved in the EX stage. We will cover in the
lectures some some issues related to realizing these functions. Keep in mind
that the only requirement is that the simulator be cycle-by-cycle
register-accurate. You don�t have to simulate the control signals. So, you can
simply pass the instruction from one pipeline latch to the next.
*/
void IF(struct inst i){
    if(numI >= sim_cycle && IF_ID_flag == 0){
        if(st_cycle == c){
            if(nops == 0){
                pgm_c += 4;
                IFused += 1;
                IF_ID_LATCH.opcode = i.opcode;
                IF_ID_LATCH.rs = i.rs;
                IF_ID_LATCH.rd = i.rd;
                IF_ID_LATCH.rt = i.rt;
                IF_ID_LATCH.Imm = i.Imm;

            }
            else{
                nops -= 1;
                IF_ID_LATCH.opcode = nopOPCODE;
                IF_ID_LATCH.rd = 0;
                IF_ID_LATCH.rt = 0;
                IF_ID_LATCH.Imm = 0;

            }
            st_cycle = 0;
            WB_flag = 0;
            IF_ID_flag = 1;
            sim_cycle += 1;

        }
        else st_cycle += 1;
    }
}

void ID(){
    if (numI >= sim_cycle - 1 && ID_EX_flag == 0) {
    if (st_cycle == 1) {
      if (IF_ID_LATCH.opcode < 6) {
        nops = 3;
        numI += nops;
      }
      ID_EX_flag = 1;
      IF_ID_flag = 0;
      st_cycle = 0;
      ++IDused;

      ID_EX_LATCH.opcode = IF_ID_LATCH.opcode;
      ID_EX_LATCH.rd = IF_ID_LATCH.rd;
      ID_EX_LATCH.rt = IF_ID_LATCH.rt;
      ID_EX_LATCH.rs = IF_ID_LATCH.rs;
      ID_EX_LATCH.Imm = IF_ID_LATCH.Imm;


      if (sim_cycle >= numI) {
            WB_flag = 0;
            sim_cycle += 1;
        }
    }
    else {
        st_cycle += 1;
    }
  }
}

void EX () {
    if (EX_MEM_flag == 0 && sim_cycle - 2 <= numI) {
        if (st_cycle == m) {
            if (ID_EX_LATCH.opcode == mulOPCODE) {
                EX_MEM_LATCH.result = reg[ID_EX_LATCH.rs] * reg[ID_EX_LATCH.rt];
                EX_MEM_LATCH.opcode = ID_EX_LATCH.opcode;
                EX_MEM_LATCH.rd = ID_EX_LATCH.rd;
                EX_MEM_LATCH.rt = ID_EX_LATCH.rt;
                EX_MEM_LATCH.rs = ID_EX_LATCH.rs;
                EX_MEM_LATCH.Imm = ID_EX_LATCH.Imm;

                ++EXused;
                EX_MEM_flag = 1;
                ID_EX_flag = 0;
                st_cycle = 0;

                if (sim_cycle >= numI + 1) { ++sim_cycle; WB_flag = 0; }
                return;
            }
        }

        if (st_cycle == n) {
            EX_MEM_LATCH.opcode = ID_EX_LATCH.opcode;
            EX_MEM_LATCH.rd = ID_EX_LATCH.rd;
            EX_MEM_LATCH.rt = ID_EX_LATCH.rt;
            EX_MEM_LATCH.rs = ID_EX_LATCH.rs;
            EX_MEM_LATCH.Imm = ID_EX_LATCH.Imm;

            if (EX_MEM_LATCH.opcode != nopOPCODE) ++EXused;

            if(ID_EX_LATCH.opcode==nopOPCODE)
            {

            }
            else if (ID_EX_LATCH.opcode==addOPCODE){EX_MEM_LATCH.result = reg[EX_MEM_LATCH.rs] + reg[EX_MEM_LATCH.rt];}
            else if (ID_EX_LATCH.opcode==addiOPCODE){EX_MEM_LATCH.result = reg[EX_MEM_LATCH.rs] + EX_MEM_LATCH.Imm;}
            else if (ID_EX_LATCH.opcode==beqOPCODE)
            {
                EX_MEM_LATCH.result = reg[EX_MEM_LATCH.rs] == reg[EX_MEM_LATCH.rt];
                if (EX_MEM_LATCH.result && EX_MEM_LATCH.opcode == beqOPCODE) {
                    pgm_c += 4 * EX_MEM_LATCH.Imm;
                }
            }
            else if (ID_EX_LATCH.opcode==lwOPCODE){EX_MEM_LATCH.result = reg[EX_MEM_LATCH.rs] + EX_MEM_LATCH.Imm;}
            else if (ID_EX_LATCH.opcode==subOPCODE){EX_MEM_LATCH.result = reg[EX_MEM_LATCH.rs] - reg[EX_MEM_LATCH.rt];}
            else if (ID_EX_LATCH.opcode==swOPCODE){EX_MEM_LATCH.result = reg[EX_MEM_LATCH.rs] + EX_MEM_LATCH.Imm;}

            EX_MEM_flag = 1;
            ID_EX_flag = 0;
            st_cycle = 0;

            if (sim_cycle >= numI + 1) { ++sim_cycle; WB_flag = 0; }
            return;
        }

        ++st_cycle;
    }
}
void MEM () {
  if (MEM_WB_flag == 0 && sim_cycle - 3 <= numI) {
    if (st_cycle == c){

      MEM_WB_LATCH.opcode = EX_MEM_LATCH.opcode;
      MEM_WB_LATCH.result = EX_MEM_LATCH.result;
      MEM_WB_LATCH.rd = EX_MEM_LATCH.rd;
      MEM_WB_LATCH.rs = EX_MEM_LATCH.rs;
      MEM_WB_LATCH.rt = EX_MEM_LATCH.rt;
      MEM_WB_LATCH.Imm = EX_MEM_LATCH.Imm;

      switch(EX_MEM_LATCH.opcode){

      case lwOPCODE:
        reg[EX_MEM_LATCH.rt] = MEMdata[EX_MEM_LATCH.result];
        MEMused = MEMused + 1;
        break;

      case swOPCODE:
        MEMdata[EX_MEM_LATCH.result] = reg[EX_MEM_LATCH.rt];
        MEMused = MEMused + 1;
        break;

      default:
        break;
      }

      MEM_WB_flag = 1;
      EX_MEM_flag = 0;
      WB_flag = 1;
      st_cycle = 0;

      if (sim_cycle >= numI + 2) {
          sim_cycle = sim_cycle + 1;
          WB_flag = 0;
        }
    }
    else{

      st_cycle = st_cycle + 1;
    }
  }
}

void WB () {
  if (WB_flag == 0 && sim_cycle - 4 <= numI) {
    if (st_cycle == 1) {

      switch(MEM_WB_LATCH.opcode){
        case nopOPCODE:

        case beqOPCODE:
        break;

        case addOPCODE:

        case subOPCODE:

        case mulOPCODE:

        reg[MEM_WB_LATCH.rd] = MEM_WB_LATCH.result;
        WBused = WBused + 1;
        break;

        case addiOPCODE:
        reg[MEM_WB_LATCH.rt] = MEM_WB_LATCH.result;
        WBused = WBused + 1;
        break;

        default:
        break;
      }

      MEM_WB_flag = 0;
      WB_flag = 1;
      st_cycle = 0;


      if (sim_cycle >= numI + 3) {
        sim_cycle = sim_cycle + 1;
        WB_flag = 0;
      }
    }
    else{
    st_cycle = st_cycle + 1;
  }
}
}
int main( int argc, char *argv[])
{

    //*char teststr[]="add $s1, $s2, $t1";
    //char * parsed1 = progScanner();
    //char **parsed2= regNumberConverter(parsed1);
    //struct inst a = parser(parsed2);
    //printf("%d",a);

    int sim_mode = 0;//mode flag, 1 for single-cycle, 0 for batch

    int i;//for loop counter
    //long pgm_c = 0;//program counter
    //long sim_cycle = 0;//simulation cycle counter
    //define your own counter for the usage of each pipeline stage here

    int test_counter = 0;
    FILE *input = NULL;
    FILE *output = NULL;
    printf("The arguments are:");

    for (i = 1; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    printf("\n");
    if (argc == 7) {
        if (strcmp("-s", argv[1]) == 0) {
            sim_mode = SINGLE;
        } else if (strcmp("-b", argv[1]) == 0) {
            sim_mode = BATCH;
        } else {
            printf("Wrong sim mode chosen\n");
            exit(0);
        }

        m = atoi(argv[2]);
        n = atoi(argv[3]);
        c = atoi(argv[4]);
        input = fopen(argv[5], "r");
        output = fopen(argv[6], "w");

    } else {
        printf("Usage: ./sim-mips -s m n c input_name output_name (single-sysle mode)\n or \n ./sim-mips -b m n c input_name  output_name(batch mode)\n");
        printf("m,n,c stand for number of cycles needed by multiplication, other operation, and memory access, respectively\n");
        exit(0);
    }
    if (input == NULL) {
        printf("Unable to open input or output file\n");
        exit(0);
    }
    if (output == NULL) {
        printf("Cannot create output file\n");
        exit(0);
    }
    //initialize registers and program counter
    if (sim_mode == 1) {
        for (i = 0; i < REG_NUM; i++) {
            reg[i] = 0;
        }
    }

    //start your code from here


    //find number of instructions from input file
    /* char c;
     numI = 0;
     for (c = getc(input); c != EOF; c = getc(input)) {
         if (c == '\n') { // Increment count if this character is newline
             numI++;
         }
     }
     numI++;*/
    //fclose(input);
    //input = fopen(argv[5], "r");

    IFused = 0;
    IDused = 0;
    EXused = 0;
    MEMused = 0;
    WBused = 0;

    char **inputLine;
    //char *line[];
    int c;
    for (c = getc(input); c != EOF; c = getc(input))
        if (c == '\n') // Increment count if this character is newline
            numI = numI + 1;
    //numI++;
    fclose(input);
    input = fopen(argv[5], "r");


    Imemory = malloc(sizeof(struct inst) * numI);
    int position = 0;

    char * instructLIne= progScanner(input);
    //inputLine= regNumberConverter(instructLIne);
    while(strcmp(instructLIne,"haltSimulation")!=0) {

        inputLine = regNumberConverter(instructLIne);
        Imemory[position++] = parser(inputLine);
        instructLIne= progScanner(input);
        //numI++;


    }
    fflush(stdin);

    while (sim_cycle - 4 <= numI - 1) {
        if (sim_cycle - 4 >= 0) { WB(); /*printf("I'm done with WB.\n");*/}
        if (sim_cycle - 3 >= 0) { MEM(); /*printf("I'm done with MEM.\n");*/}
        if (sim_cycle - 2 >= 0) { EX(); /*printf("I'm done with EX.\n");*/}
        if (sim_cycle - 1 >= 0) { ID(); /*printf("I'm done with ID.\n");*/}
        IF(Imemory[pgm_c / 4]); /*printf("I'm done with IF.\n");*/


        if (sim_mode == SINGLE) {
            printf("CLK: %ld\n", sim_cycle);
            for (i = 1; i < REG_NUM; ++i) {
                printf("%ld  ", reg[i]);
            }
            printf("-  PC: %ld\n", pgm_c);
            printf("Press ENTER to continue\n");
            while (getchar() != '\n');
        }
    }
    //printf("I am done with cycles.\n");
    //free(Imemory);



    int totalUsed = IFused + IDused + EXused + MEMused + WBused;
    IFutil = (double) IFused / totalUsed;
    IDutil = (double) IDused / totalUsed;
    EXutil = (double) EXused / totalUsed;
    MEMutil = (double) MEMused / totalUsed;
    WButil = (double) WBused / totalUsed;
    //printf("I am done with calculations.\n");
    if (sim_mode == BATCH) {
        fprintf(output, "program name: %s\n", argv[5]);
        fprintf(output, "stage utilization: %f  %f  %f  %f  %f \n", IFutil, IDutil, EXutil, MEMutil, WButil);

        fprintf(output, "register values ");

        for (i = 1; i < REG_NUM; i++)
            fprintf(output, "%ld  ", reg[i]);

        fprintf(output, "%ld\n", pgm_c);
        //fclose(output);
        //printf("I dont close output.\n");
    }
    //printf("I am done calculating values.\n");
    fclose(input);
    //printf("I closed input.\n");




    //printf("I am done closing files.\n");
    return 0;


    //output code 2: the following code will output the register
    //value to screen at every cycle and wait for the ENTER key
    //to be pressed; this will make it proceed to the next cycle




    /*
    //add the following code to the end of the simulation,
    //to output statistics in batch mode
    if(sim_mode==0){
        fprintf(output,"program name: %s\n",argv[5]);
        fprintf(output,"stage utilization: %f  %f  %f  %f  %f \n",
                         ifUtil, idUtil, exUtil, memUtil, wbUtil);
                 // add the (double) stage_counter/sim_cycle for each
                 // stage following sequence IF ID EX MEM WB

        fprintf(output,"register values ");
        for (i=1;i<REG_NUM;i++){
            fprintf(output,"%d  ",reg[i]);
        }
        fprintf(output,"%d\n",pgm_c);
    }
    //close input and output files at the end of the simulation
    fclose(input);
    fclose(output);
    return 0;}
    */
}
