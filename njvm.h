#include "bigint/support.h"
#include "bigint/bigint.h"

typedef struct {
  int isObjRef;
  union {
    ObjRef ref;
    int number;
  }u;
} StackSlot;

#define MSB (1 << (8 * sizeof(unsigned int) -1))
#define BROKEN_HEART ( (1 << (8 * sizeof(unsigned int) - 2)))
#define OPCODE(x)    ((x) >> 24)
#define IMMEDIATE(x) ((x) & 0x00FFFFFF)
#define SIGN_EXTEND(i) ((i) & 0x00800000 ? (i) | 0xFF000000 : (i))
#define GET_REFS(objRef) ((ObjRef *)(objRef)->data)
#define GET_SIZE(objRef) ((objRef)->size & ~((MSB) | (BROKEN_HEART)))
#define IS_PRIM(objRef) (((objRef)->size & MSB) == 0)
#define IS_RELOCATED(x) ( ((x)->size & BROKEN_HEART) != 0)
#define FORWARD_POINTER(x) ((x)->size & ~BROKEN_HEART)
#define GET_PRIM(objRef) (*(int *)(objRef)->data)



int readProgram(char *path);
void init_vm(int reset);
void run(void);
void run_debug_mode(void);
void print_help(void);
void execute(int instruction);
void push(int data);
void push_ref(ObjRef ref);
int pop(void);
ObjRef pop_ref(void);
void clear_njvm(void);
ObjRef newCompoundObject(int numObjRefs);
void print_gc_stats(void);
void init_heap(void);
ObjRef heap_malloc(size_t size);
void gc(void);
ObjRef copy(ObjRef ref);
ObjRef heap_copy(ObjRef ref);
void free_heap(void);


unsigned int *program;
int program_size;
int static_data_size;
int stack_num_of_slots;

StackSlot *stack;
ObjRef *static_data_area;
ObjRef return_register;

ObjRef _nil;

int halt;
int debug;
unsigned int pc;

int sp;
int fp;

char *from;
char *to;
char *free_p;

int heap_half;

unsigned int alloc_count;
unsigned int alloc_bytes;
unsigned int alloc_living_count;
unsigned int alloc_living_bytes;

int stack_default;
int heap_default;

int gcstats;
int gcpurge;

int version;

char *instruction_names[42] = {"halt","pushc","add","sub","mul","div","mod","rdint","wrint","rdchr","wrchr","pushg","popg","asf","rsf","pushl","popl","eq","ne","gt","ge","lt","le","jmp","brf","brt","call","ret","drop","pushr","popr","dup","new","getf","putf","newa","getfa","putfa","getsz","pushn","refeq","refne"};
