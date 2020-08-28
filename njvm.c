#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "njvm.h"
#include "bigint//support.h"
#include "bigint//bigint.h"


int main(int argc, char *argv[]){
  int i;
  int flag;
  _nil = NULL;

  version = 8;

  stack_default = 64;
  heap_default = 8192;

  gcstats = 0;
  gcpurge = 0;
  debug = 0;
  flag = 0;

  if(argc == 1) printf("Error: no code file specified\n");
  for(i = 1; i < argc; i++){
    if(strstr(argv[i],"--")!=NULL){
      if(!strcmp(argv[i],"--version")){
        printf("Ninja Virtual Machine version %d (compiled %s, %s)\n",version,__DATE__,__TIME__);
        exit(0);
      }
      else if(!strcmp(argv[i],"--help")){
        printf("Usage: ./njvm_ref [options] <code file>\n");
        printf("Options:\n");
        printf("  --stack <n>      set stack size to n KBytes (default: n = 64)\n");
        printf("  --heap <n>       set heap size to n KBytes (default: n = 8192)\n");
        printf("  --gcstats        show garbage collection statistics\n");
        printf("  --gcpurge        purge old objects after collection\n");
        printf("  --debug          start virtual machine in debug mode\n");
        printf("  --version        show version and exit\n");
        printf("  --help           show this help and exit\n");
        exit(0);
      }
      else if(!strcmp(argv[i],"--debug")){
        debug = 1;
      }
      else if(!strcmp(argv[i],"--stack")){
        stack_default = atoi(argv[i+1]);
        if(stack_default <= 0){
          printf("Error: illegal stack size\n");
          exit(0);
        }
      }
      else if(!strcmp(argv[i],"--heap")){
        heap_default = atoi(argv[i+1]);
        if(heap_default <= 0){
          printf("Error: illegal heap size\n");
          exit(0);
        }
      }
      else if(!strcmp(argv[i],"--gcstats")){
        gcstats = 1;
      }
      else if(!strcmp(argv[i],"--gcpurge")){
        gcstats = 1;
      }
      else{
          printf("Error: unknown option '%s', try './njvm_ref --help'\n", argv[i]);
          exit(0);
      }
    }
    else{
        flag = readProgram(argv[i]);
    }
  }
  if(flag){
    if(debug){
      init_heap();
      run_debug_mode();
      clear_njvm();
    }
    else{
      init_heap();
      run();
      clear_njvm();
    }
  }
  else{
  }

  return 0;
}


int readProgram(char *path){
  FILE *f;
  char *format;
  int tmp;

  format = (char *)malloc(sizeof(char)*4);
  tmp = 0;

  if(NULL==(f = fopen(path,"r"))){
    printf("Error: cannot open code file '%s'\n",path);
    exit(0);
  }

  fread(format,sizeof(char),4,f);
  if(strncmp(format,"NJBF",4)){
    printf("Invalid format!\n");
    return 0;
  }

  fread(&tmp,sizeof(int),1,f);
  if(version!=tmp){
    printf("Error: file '%s' has wrong version number\n",path);
    return 0;
  }

  fread(&program_size,sizeof(int),1,f);
  fread(&static_data_size,sizeof(int),1,f);

  program = (unsigned int *)malloc(program_size * sizeof(unsigned int));
  fread(program, sizeof(int), program_size, f);
  fclose(f);
  free(format);
  return 1;
}

void init_vm(int reset){
  halt = 0;
  pc = 0;
  return_register = (ObjRef)heap_malloc(sizeof(ObjRef));
  stack = malloc(stack_default * 1024);
  sp = 0;
  fp = 0;
  static_data_area = (ObjRef *)calloc(static_data_size,sizeof(ObjRef));

}


void run_debug_mode(void){
  char *input;
  int breakpoint;
  int closed;
  int i;
  unsigned int instruction;
  instruction = 0;

  breakpoint = -1;
  i = 0;
  input = malloc(4 * sizeof(char));
  closed = 0;
  init_vm(0);

  printf("Welcome to NJVM debug mode:\n\n");
  print_help();
  printf("\n");

  while(!closed){
    printf("Debug--> ");
    fgets(input,4,stdin);
    printf("\n");

    switch(input[0]){
      case 's': printf("Stack:\n");
                for(i = 0; i <= sp; i++){
                  if(i==sp){
                    if(stack[i].isObjRef==1){
                      bip.op1 = stack[i].u.ref;
                      printf("    sp->%p:\t",(void *)stack[i].u.ref);
                      bigPrint(stdout);
                      printf("\n");
                    }
                    else{
                      printf("    sp->%d \t\n",stack[i].u.number);
                    }
                  }
                  else{
                    if(stack[i].isObjRef==1){
                      bip.op1 = stack[i].u.ref;
                      printf("        %p:\t",(void *)stack[i].u.ref);
                      bigPrint(stdout);
                      printf("\n");
                    }
                    else{
                      printf("        %d \t\n",stack[i].u.number);
                    }
                  }
                }
                break;
      case 'g': printf("Static data area:\n");
                for(i = 0; i < static_data_size; i++){
                  printf("        %p:\t\n",(void *)static_data_area[i]);
                }
                break;
      case 'p': printf("Program:\n");
                for(i = 0; i < program_size; i++){
                  printf("\t%s\t%d\n",instruction_names[OPCODE(program[i])], SIGN_EXTEND(IMMEDIATE(program[i])));
                }
                break;
      case 'n': if(halt==0){
                  instruction = program[pc];
                  pc++;
                  execute(instruction);
                  printf("Executing instruction: %s\t%d\n",instruction_names[OPCODE(instruction)], SIGN_EXTEND(IMMEDIATE(instruction)));
                }
                break;
      case 'r': printf("Running:\n");
                while(halt == 0) {
                  if(pc == breakpoint){
                    breakpoint = -1;
                    break;
                  }
                  instruction = program[pc];
                  pc++;
                  execute(instruction);
                  printf("\t\t%s\t%d\n",instruction_names[OPCODE(instruction)],  SIGN_EXTEND(IMMEDIATE(instruction)));
                }
                break;
      case 'b': printf("Set breakpoint to %d\n",input[2]-48);
                breakpoint = input[2]-48;
                break;
      case 'h': print_help();
                break;
      case 'e':
      case 'x': closed = 1;
                break;
      default: printf("Unknown action. Type 'h' for help.\n\n");
               break;
    }
    if(halt == 1){
      printf("Program ended. Resetting state.\n");
      init_vm(1);
      breakpoint = -1;
    }

    printf("\n");
  }
  clear_njvm();
}

void print_help(void){
  printf("Type 's' to print the stack\n"
         "     'g' to print the global data area\n"
         "     'p' to print the program\n\n"
         "     'n' to execute the next instruction\n"
         "     'b [num] to set breakpoint\n"
         "     'r' to execute program until end or breakpoint\n");
}



void run(void){
  int instruction;
  init_vm(0);
  printf("Ninja Virtual Machine started\n");
  while(halt == 0) {
    instruction = program[pc];
    pc++;
    execute(instruction);
  }
  gc();
  printf("Ninja Virtual Machine stopped\n");
}



void execute(int instruction){
  int sign_extended_immediate_value;
  int i, j;
  int index;
  char c;

  i = 0;
  j = 0;
  index = 0;

  sign_extended_immediate_value = SIGN_EXTEND(IMMEDIATE(instruction));

  switch(OPCODE(instruction)){
    case 0: halt = 1;
            break;
    case 1: bigFromInt(sign_extended_immediate_value);
            push_ref(bip.res);
            break;
    case 2: bip.op2 = pop_ref();
            bip.op1 = pop_ref();
            bigAdd();
            push_ref(bip.res);
            break;
    case 3:
            bip.op2 = pop_ref();
            bip.op1 = pop_ref();
            bigSub();
            push_ref(bip.res);
            break;
    case 4:
            bip.op2 = pop_ref();
            bip.op1 = pop_ref();
            bigMul();
            push_ref(bip.res);
            break;
    case 5:
            bip.op2 = pop_ref();
            bip.op1 = bip.op2;
            if(bigToInt() == 0){
              printf("Division by zero!");
              exit(0);
            }
            bip.op1 = pop_ref();
            bigDiv();
            push_ref(bip.res);
            break;
    case 6:
          bip.op2 = pop_ref();
          bip.op1 = bip.op2;
          if(bigToInt() == 0){
            printf("Modulo by zero!");
            exit(0);
          }
          bip.op1 = pop_ref();
          bigDiv();
          push_ref(bip.rem);
          break;
    case 7: bigRead(stdin);
            push_ref(bip.res);
            break;
    case 8: bip.op1 = pop_ref();
            bigPrint(stdout);
            break;
    case 9: scanf("%c",&c);
            bigFromInt(c);
            push_ref(bip.res);
            break;
    case 10: bip.op1 = pop_ref();
             c = bigToInt();
             printf("%c",c);
             break;
    case 11: push_ref(static_data_area[sign_extended_immediate_value]);
             break;
    case 12: static_data_area[sign_extended_immediate_value] = pop_ref();
             break;
    case 13: push(fp);
             fp = sp;
             if(sp + sign_extended_immediate_value >= (stack_default * 1024)/12){
               printf("Stack Overflow!\n");
               exit(0);
             }
             j = sp;
             for(i = sp; i < j + sign_extended_immediate_value; i++){
               push_ref(_nil);
             }
             break;
    case 14: sp = fp;
             fp = pop();
             break;
    case 15: push_ref(stack[fp + sign_extended_immediate_value].u.ref);
             break;
    case 16: stack[fp + sign_extended_immediate_value].u.ref = pop_ref();
             break;
    case 17: bip.op2 = pop_ref();
             bip.op1 = pop_ref();
             bigFromInt(bigCmp()==0);
             push_ref(bip.res);
             break;
    case 18: bip.op2 = pop_ref();
             bip.op1 = pop_ref();
             bigFromInt(bigCmp()!=0);
             push_ref(bip.res);
             break;
    case 19: bip.op2 = pop_ref();
             bip.op1 = pop_ref();
             bigFromInt(bigCmp()<0);
             push_ref(bip.res);
             break;
    case 20: bip.op2 = pop_ref();
             bip.op1 = pop_ref();
             bigFromInt(bigCmp()<=0);
             push_ref(bip.res);
             break;
    case 21: bip.op2 = pop_ref();
             bip.op1 = pop_ref();
             bigFromInt(bigCmp()>0);
             push_ref(bip.res);
             break;
    case 22: bip.op2 = pop_ref();
             bip.op1 = pop_ref();
             bigFromInt(bigCmp()>=0);
             push_ref(bip.res);
             break;
    case 23: pc = sign_extended_immediate_value;
             break;
    case 24: bigFromInt(0);
             bip.op1 = pop_ref();
             bip.op2 = bip.res;
             if(bigCmp()==0){
               pc = sign_extended_immediate_value;
             }
             break;
    case 25: bigFromInt(1);
             bip.op1 = pop_ref();
             bip.op2 = bip.res;
             if(bigCmp()==0){
               pc = sign_extended_immediate_value;
             }
             break;
    case 26: push(pc);
             pc = sign_extended_immediate_value;
             break;
    case 27: pc = pop();
             break;
    case 28: i = 0;
             while(i < sign_extended_immediate_value){
               pop_ref();
               i++;
             }
             break;
    case 29: push_ref(return_register);
             break;
    case 30: return_register = pop_ref();
             break;
    case 31: bip.op1 = pop_ref();
             push_ref(bip.op1);
             push_ref(bip.op1);
             break;
    case 32: push_ref(newCompoundObject(sign_extended_immediate_value));
             break;
    case 33: bip.op1 = pop_ref();
             bip.op1 = *(GET_REFS(bip.op1)+sign_extended_immediate_value);
             push_ref(bip.op1);
             break;
    case 34: bip.op2 = pop_ref();
             bip.op1 = pop_ref();
             *(GET_REFS(bip.op1)+sign_extended_immediate_value) = bip.op2;
             break;
    case 35: bip.op1 = pop_ref();
             push_ref(newCompoundObject(bigToInt()));
             break;
    case 36: bip.op1 = pop_ref();
             index = bigToInt();
             bip.res = pop_ref();
             bip.op1 = *(GET_REFS(bip.res)+index);
             push_ref(bip.op1);
             break;
    case 37: bip.op2 = pop_ref();
             bip.op1 = pop_ref();
             index = bigToInt();
             bip.rem = pop_ref();
             bigFromInt(GET_PRIM(bip.op1));

             *(GET_REFS(bip.rem)+index) = bip.op2;
             break;
    case 38: bip.op1 = pop_ref();
             if(IS_PRIM(bip.op1)){
               bigFromInt(-1);
             }
             else{
               bigFromInt(GET_SIZE(bip.op1));
             }
             push_ref(bip.res);
             break;
    case 39: push_ref(_nil);
             break;
    case 40: bip.op2 = pop_ref();
             bip.op1 = pop_ref();
             if(bip.op2==bip.op1){
               bigFromInt(1);
             }
             else{
               bigFromInt(0);
             }
             push_ref(bip.res);
             break;
    case 41: bip.op2 = pop_ref();
             bip.op1 = pop_ref();
             if(bip.op2==bip.op1){
               bigFromInt(0);
             }
             else{
               bigFromInt(1);
             }
             push_ref(bip.res);
             break;
    default: printf("Unsupported action!\n");
             break;
  }
}


void push(int data){
  if(sp+1 >  (stack_default * 1024)/12){
    printf("Stack overflow!");
    exit(0);
  }
  stack[sp].isObjRef = 0;
  stack[sp].u.number = data;
  sp++;
}

void push_ref(ObjRef ref){
  if(sp+1 > (stack_default * 1024)/12){
    printf("Stack overflow!");
    exit(0);
  }
  stack[sp].isObjRef = 1;
  stack[sp].u.ref = ref;
  sp++;
}

int pop(void){
  int pop;
  if(sp <= 0){
    printf("Stack underflow!");
    exit(0);
  }
  sp--;
  pop = stack[sp].u.number;
  return pop;
}

ObjRef pop_ref(void){
  ObjRef pop;
  if(sp <= 0){
    printf("Stack underflow!");
    exit(0);
  }
  sp--;
  pop = stack[sp].u.ref;
  return pop;
}

void fatalError(char *msg){
}



ObjRef newCompoundObject(int refs){
  ObjRef ref;
  ref = (ObjRef)heap_malloc(sizeof(int) + (refs * sizeof(ObjRef)));
  ref->size = refs | 0x1 << 31;
  return ref;
}

ObjRef newPrimObject(int dataSize){
  ObjRef object;
  object = (ObjRef)heap_malloc(sizeof(int) + dataSize);
  object->size = dataSize;
  return object;
}

void init_heap(){
  heap_half = heap_default * 1024;
  heap_half = heap_half/2;

  from = (char *)malloc(heap_half);
  to = (char *)malloc(heap_half);

  memset(from,'\0',heap_half);
  memset(to,'\0',heap_half);

  free_p = from;

  alloc_count = 0;
  alloc_bytes = 0;
  alloc_living_count = 0;
  alloc_living_bytes = 0;
}

ObjRef heap_malloc(size_t size){
  ObjRef ref;
  if(free_p + size >= from + heap_half){
    gc();
    if(free_p + size >= from + heap_half){
      puts("gc alloc error!");
      exit(0);
    }
  }
  ref = (ObjRef)free_p;
  if(ref == NULL){
    puts("heap alloc error!");
    exit(0);
  }
  free_p += size;
  alloc_count++;
  alloc_bytes += size;
  alloc_living_count++;
  alloc_living_bytes += size;
  return ref;
}

void print_gc_stats(){
  printf("Garbage Collector:\n");
  printf("    %u objects (%u Bytes) allocated since last collection\n", alloc_count-alloc_living_count, alloc_bytes-alloc_living_bytes);
  printf("    %u objects (%u Bytes) copied during this collection\n", alloc_living_count, alloc_living_bytes);
  printf("    %u of %u bytes free after this collection\n", heap_half - alloc_living_bytes, heap_half);

  alloc_count = 0;
  alloc_bytes = 0;
}

void gc(void){
  char *tmp;
  char *scan;
  int i;

  alloc_living_count = 0;
  alloc_living_bytes = 0;

  tmp = from;
  from = to;
  to = tmp;

  memset(from, '\0', heap_half);
  free_p = from;

  for(i = 0; i < sp; i++){
    if(stack[i].isObjRef==1){
      stack[i].u.ref = copy(stack[i].u.ref);
    }
  }

  for(i = 0; i < static_data_size; i++){
    static_data_area[i] = copy(static_data_area[i]);
  }

  return_register = copy(return_register);
  bip.op1 = copy(bip.op1);
  bip.op2 = copy(bip.op2);
  bip.res = copy(bip.res);
  bip.rem = copy(bip.rem);

  scan = from;
  while(scan < free_p){
    if(!IS_PRIM((ObjRef)scan)){
      for(i = 0; i < GET_SIZE((ObjRef)scan); i++){
            GET_REFS((ObjRef)scan)[i] = copy((ObjRef)GET_REFS((ObjRef)scan)[i]);
      }
      scan += sizeof(unsigned int) + GET_SIZE((ObjRef)scan) * sizeof(ObjRef);
    }
    else{
      scan += sizeof(unsigned int) + GET_SIZE((ObjRef)scan);
    }
  }

  if(gcstats == 1){
    print_gc_stats();
  }
  if(gcpurge == 1){
    memset(to,'\0',heap_half);
  }
}



ObjRef copy(ObjRef ref){
  ObjRef copy;
  if(ref == NULL){
    copy = NULL;
  }
  else if(IS_RELOCATED(ref)){
    copy = (ObjRef) (from + FORWARD_POINTER(ref));
  }
  else{
    if(!IS_PRIM(ref)){
      copy = newCompoundObject(GET_SIZE(ref));
      memcpy(copy->data,ref->data,GET_SIZE(ref) * sizeof(ObjRef));

    }
    else{
      copy = newPrimObject(ref->size);
      memcpy(copy->data,ref->data,ref->size);
    }
    ref->size = BROKEN_HEART | ((char*) copy - from);
  }
  return copy;
}

void free_heap(){
  free(from);
  free(to);
}


void clear_njvm(void){
  free(stack);
  free(static_data_area);
  free_heap();
  stack = NULL;
  static_data_area = NULL;
}
