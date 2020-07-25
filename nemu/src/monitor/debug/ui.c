#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"
#include "cpu/exec.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);
void isa_reg_display();
uint32_t instr_fetch(vaddr_t *pc, int len);
uint32_t expr(char *e, bool *success); 

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}

static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);

/* PA1.3 */
static int cmd_si(char *args){	
	char *arg = strtok(NULL, " ");
	if(arg == NULL){
		cpu_exec(1);
		return 0;
	}
	
	char ch;
	uint64_t n = 0;
	while(*arg != '\0')
	{
		ch = *arg++;
		if(ch < '0' || ch > '9'){
			printf("Input format error");
			return 0;
		}
		n = n * 10 + (ch - '0');
	}
	if(n == 0) n = 1;
	cpu_exec(n);
	return 0;
}

/* PA1.3 */
static int cmd_info(char *args){
	char *arg = strtok(NULL, " ");
	if(arg == NULL) return 0;
	if(arg[0] == 'r'){
		isa_reg_display();
	}

	return 0;
}

/* PA1.3 */
static int cmd_x(char *args){
	char *arg = strtok(NULL, " ");
	if(arg == NULL) return 0;
	int n = 0, i;
	sscanf(arg, "%d", &n);
	arg = strtok(NULL, " ");
	if(arg == NULL) return 0;
	uint32_t expr;
	sscanf(arg, "%x", &expr);
	for(i = 0; i < n; ++ i){
		printf("%#x: ", expr);
		printf("%#x\n", instr_fetch(&expr, 4));
	}
	return 0;
}

/* PA1.5 1.6 
 * Date: 2020/7/25
 */
int cmd_p(char *args)
{
	if(args == NULL) return 0;
	bool success = true;
	uint32_t value = expr(args, &success);
	if(success){
		printf("%u - %#x\n", value, value);
	}
	return 0;
}


static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  /* PA1.3 */
  { "si","Format: si [N]\n"\
    "     Execute the program with N(default: 1) step", cmd_si },
  { "info", "Format: info [rf]\n"\
	"       r: Print the values of all registers", cmd_info },
  { "p", "Format: p EXPR\n" "    Calculate the value of the expression EXPR\n", cmd_p},
  { "x", "Format: x N EXPR\n" \
	"    Use EXPR as the starting address, and output N consecutive 4 bytes in hexadecimal form", cmd_x }


  /* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void ui_mainloop(int is_batch_mode) {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}
