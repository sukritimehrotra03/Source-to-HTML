#ifndef S2HTML_EVENT_H
#define S2HTML_EVENT_H

/* event data properties */
#define USER_HEADER_FILE		1
#define STD_HEADER_FILE			2
#define RES_KEYWORD_DATA		3
#define RES_KEYWORD_NON_DATA	4

#define PEVENT_DATA_SIZE	1024

typedef enum
{
	PEVENT_NULL,
	PEVENT_PREPROCESSOR_DIRECTIVE,
	PEVENT_RESERVE_KEYWORD,
	PEVENT_NUMERIC_CONSTANT,
	PEVENT_STRING,
	PEVENT_HEADER_FILE,
	PEVENT_REGULAR_EXP,
	PEVENT_SINGLE_LINE_COMMENT,
	PEVENT_MULTI_LINE_COMMENT,
	PEVENT_ASCII_CHAR,
	PEVENT_IDENTIFIER,
	PEVENT_EOF
}pevent_e;

typedef struct
{
	pevent_e type; // event type
	int property; // property associated with data
	int length; // data length
	char data[PEVENT_DATA_SIZE]; // cwparsed string
}pevent_t;

/********** function prototypes **********/

pevent_t *get_parser_event(FILE *fp);



#define SIZE_OF_SYMBOLS (sizeof(symbols))
#define SIZE_OF_OPERATORS (sizeof(operators))
#define WORD_BUFF_SIZE	100

/********** Internal states and event of parser **********/
typedef enum
{
	PSTATE_IDLE,
	PSTATE_PREPROCESSOR_DIRECTIVE,
	PSTATE_SUB_PREPROCESSOR_MAIN,
	PSTATE_SUB_PREPROCESSOR_RESERVE_KEYWORD,
	PSTATE_SUB_PREPROCESSOR_ASCII_CHAR,
	PSTATE_HEADER_FILE,
	PSTATE_RESERVE_KEYWORD,
	PSTATE_NUMERIC_CONSTANT,
	PSTATE_STRING,
	PSTATE_SINGLE_LINE_COMMENT,
	PSTATE_MULTI_LINE_COMMENT,
	PSTATE_ASCII_CHAR
}pstate_e;

/********** global variables **********/

/* parser state variable */
static pstate_e state = PSTATE_IDLE;

/* sub state is used only in preprocessor state */
static pstate_e state_sub = PSTATE_SUB_PREPROCESSOR_MAIN;

/* event variable to store event and related properties */
static pevent_t pevent_data;
static int event_data_idx=0;

static char word[WORD_BUFF_SIZE];
static int word_idx=0;


static char* res_kwords_data[] = {"const", "volatile", "extern", "auto", "register",
   						   "static", "signed", "unsigned", "short", "long", 
						   "double", "char", "int", "float", "struct", 
						   "union", "enum", "void", "typedef", ""
						  };

static char* res_kwords_non_data[] = {"goto", "return", "continue", "break", 
							   "if", "else", "for", "while", "do", 
							   "switch", "case", "default","sizeof", ""
							  };

static char operators[] = {'/', '+', '*', '-', '%', '=', '<', '>', '~', '&', ',', '!', '^', '|'};
static char symbols[] = {'(', ')', '{', '}', '[', ']', ':', ';'};

/********** state handlers **********/
pevent_t *pstate_idle_handler(FILE *fd, int ch);
pevent_t *pstate_single_line_comment_handler(FILE *fd, int ch);
pevent_t *pstate_multi_line_comment_handler(FILE *fd, int ch);
pevent_t *pstate_numeric_constant_handler(FILE *fd, int ch);
pevent_t *pstate_string_handler(FILE *fd, int ch);
pevent_t *pstate_header_file_handler(FILE *fd, int ch);
pevent_t *pstate_ascii_char_handler(FILE *fd, int ch);
pevent_t *pstate_reserve_keyword_handler(FILE *fd, int ch);
pevent_t *pstate_preprocessor_directive_handler(FILE *fd, int ch);
pevent_t *pstate_sub_preprocessor_main_handler(FILE *fd, int ch);

int is_symbol(char c);
int is_operator(char c);

#endif
/**** End of file ****/
