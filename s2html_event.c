#include <stdio.h>
#include <string.h>
#include "s2html_event.h"

/********** Utility functions **********/

static int is_reserved_keyword(char *word)
{
    int idx = 0;
    while (*res_kwords_data[idx])
    {
        if (strcmp(res_kwords_data[idx++], word) == 0)
            return RES_KEYWORD_DATA;
    }
    idx = 0;
    while (*res_kwords_non_data[idx])
    {
        if (strcmp(res_kwords_non_data[idx++], word) == 0)
            return RES_KEYWORD_NON_DATA;
    }
    return 0;
}

int is_symbol(char c)
{
    for (int idx = 0; idx < sizeof(symbols); idx++)
        if (symbols[idx] == c)
            return 1;
    return 0;
}

int is_operator(char c)
{
    for (int idx = 0; idx < sizeof(operators); idx++)
        if (operators[idx] == c)
            return 1;
    return 0;
}

static void set_parser_event(pstate_e s, pevent_e e)
{
    pevent_data.data[event_data_idx] = '\0';
    pevent_data.length = event_data_idx;
    event_data_idx = 0;
    word_idx = 0;
    state = s;
    state_sub = PSTATE_SUB_PREPROCESSOR_MAIN;  // Reset sub-state explicitly
    pevent_data.type = e;
}

/********** Event Dispatcher **********/

pevent_t *get_parser_event(FILE *fd)
{
    int ch;
    pevent_t *evptr = NULL;
    while ((ch = fgetc(fd)) != EOF)
    {
        switch (state)
        {
        case PSTATE_IDLE:
            if ((evptr = pstate_idle_handler(fd, ch)) != NULL)
                return evptr;
            break;
        case PSTATE_SINGLE_LINE_COMMENT:
            if ((evptr = pstate_single_line_comment_handler(fd, ch)) != NULL)
                return evptr;
            break;
        case PSTATE_MULTI_LINE_COMMENT:
            if ((evptr = pstate_multi_line_comment_handler(fd, ch)) != NULL)
                return evptr;
            break;
        case PSTATE_PREPROCESSOR_DIRECTIVE:
            do {
                evptr = pstate_preprocessor_directive_handler(fd, ch);
                if (evptr != NULL)
                    return evptr;
            } while ((ch = fgetc(fd)) != EOF);
            break;
        case PSTATE_RESERVE_KEYWORD:
            if ((evptr = pstate_reserve_keyword_handler(fd, ch)) != NULL)
                return evptr;
            break;
        case PSTATE_NUMERIC_CONSTANT:
            if ((evptr = pstate_numeric_constant_handler(fd, ch)) != NULL)
                return evptr;
            break;
        case PSTATE_STRING:
            if ((evptr = pstate_string_handler(fd, ch)) != NULL)
                return evptr;
            break;
        case PSTATE_ASCII_CHAR:
            if ((evptr = pstate_ascii_char_handler(fd, ch)) != NULL)
                return evptr;
            break;
        case PSTATE_HEADER_FILE:
    if ((evptr = pstate_header_file_handler(fd, ch)) != NULL)
        return evptr;
    break;
        default:
            state = PSTATE_IDLE;
            break;
        }
    }
    set_parser_event(PSTATE_IDLE, PEVENT_EOF);
    return &pevent_data;
}

/********** State Handlers **********/

pevent_t *pstate_idle_handler(FILE *fd, int ch)
{
    if (ch == '/')
    {
        int next = fgetc(fd);
        if (next == '*')
        {
            state = PSTATE_MULTI_LINE_COMMENT;
            pevent_data.data[event_data_idx++] = '/';
            pevent_data.data[event_data_idx++] = '*';
            return NULL;
        }
        else if (next == '/')
        {
            state = PSTATE_SINGLE_LINE_COMMENT;
            pevent_data.data[event_data_idx++] = '/';
            pevent_data.data[event_data_idx++] = '/';
            return NULL;
        }
        else
        {
            ungetc(next, fd);
            pevent_data.data[event_data_idx++] = ch;
            return NULL;
        }
    }
    else if (ch == '#')
    {
        state = PSTATE_PREPROCESSOR_DIRECTIVE;
        state_sub = PSTATE_SUB_PREPROCESSOR_MAIN;
        pevent_data.data[event_data_idx++] = ch;
        return NULL;
    }
    else if (ch == '"')
    {
        state = PSTATE_STRING;
        pevent_data.data[event_data_idx++] = ch;
        return NULL;
    }
    else if (ch == '\'')
    {
        state = PSTATE_ASCII_CHAR;
        pevent_data.data[event_data_idx++] = ch;
        return NULL;
    }
    else if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'))
    {
        state = PSTATE_RESERVE_KEYWORD;
        word[word_idx++] = ch;
        pevent_data.data[event_data_idx++] = ch;
        return NULL;
    }
    else if (ch >= '0' && ch <= '9')
    {
        state = PSTATE_NUMERIC_CONSTANT;
        pevent_data.data[event_data_idx++] = ch;
        return NULL;
    }
    else if (is_symbol(ch) || is_operator(ch))
    {
        pevent_data.data[event_data_idx++] = ch;
        set_parser_event(PSTATE_IDLE, PEVENT_REGULAR_EXP);
        return &pevent_data;
    }
    else
    {
        pevent_data.data[event_data_idx++] = ch;
        return NULL;
    }
}

pevent_t *pstate_sub_preprocessor_main_handler(FILE *fd, int ch)
{
    if (ch == '<' || ch == '"')
    {
        // Finish the #include part
        pevent_data.data[event_data_idx] = '\0';
        pevent_data.length = strlen(pevent_data.data);
        pevent_data.type = PEVENT_PREPROCESSOR_DIRECTIVE;

        // Temporarily store #include part
        static pevent_t directive_event;
        directive_event = pevent_data;

        // Now start header file parsing
        pevent_data.data[0] = ch;
        event_data_idx = 1;

        // Set next state to header file parsing
        state = PSTATE_HEADER_FILE;

        // Return the #include event first
        return &directive_event;
    }

    pevent_data.data[event_data_idx++] = ch;

    if (ch == '\n')
    {
        set_parser_event(PSTATE_IDLE, PEVENT_PREPROCESSOR_DIRECTIVE);
        return &pevent_data;
    }

    return NULL;
}

pevent_t *pstate_preprocessor_directive_handler(FILE *fd, int ch)
{
    switch (state_sub)
    {
    case PSTATE_SUB_PREPROCESSOR_MAIN:
        return pstate_sub_preprocessor_main_handler(fd, ch);
    default:
        state = PSTATE_IDLE;
        break;
    }
    return NULL;
}

pevent_t *pstate_header_file_handler(FILE *fd, int ch)
{
    pevent_data.data[event_data_idx++] = ch;

    if ((pevent_data.data[0] == '<' && ch == '>') ||
    (pevent_data.data[0] == '"' && ch == '"'))
	{
		// Trim to just the header file name
		pevent_data.data[event_data_idx] = '\0';

		char temp[PEVENT_DATA_SIZE];
		strncpy(temp, pevent_data.data + 1, event_data_idx - 2);
		temp[event_data_idx - 2] = '\0';

		strcpy(pevent_data.data, temp);
		pevent_data.length = strlen(pevent_data.data);

		pevent_data.property = (ch == '>') ? STD_HEADER_FILE : USER_HEADER_FILE;

		set_parser_event(PSTATE_IDLE, PEVENT_HEADER_FILE);
		return &pevent_data;
	}

    return NULL;
}

pevent_t *pstate_single_line_comment_handler(FILE *fd, int ch)
{
    pevent_data.data[event_data_idx++] = ch;
    if (ch == '\n')
    {
        set_parser_event(PSTATE_IDLE, PEVENT_SINGLE_LINE_COMMENT);
        return &pevent_data;
    }
    return NULL;
}

pevent_t *pstate_multi_line_comment_handler(FILE *fd, int ch)
{
    pevent_data.data[event_data_idx++] = ch;
    if (ch == '/' && pevent_data.data[event_data_idx - 2] == '*')
    {
        set_parser_event(PSTATE_IDLE, PEVENT_MULTI_LINE_COMMENT);
        return &pevent_data;
    }
    return NULL;
}

pevent_t *pstate_string_handler(FILE *fd, int ch)
{
    pevent_data.data[event_data_idx++] = ch;
    if (ch == '"' && pevent_data.data[event_data_idx - 2] != '\\')
    {
        set_parser_event(PSTATE_IDLE, PEVENT_STRING);
        return &pevent_data;
    }
    return NULL;
}

pevent_t *pstate_ascii_char_handler(FILE *fd, int ch)
{
    pevent_data.data[event_data_idx++] = ch;
    if (ch == '\'' && pevent_data.data[event_data_idx - 2] != '\\')
    {
        set_parser_event(PSTATE_IDLE, PEVENT_ASCII_CHAR);
        return &pevent_data;
    }
    return NULL;
}

pevent_t *pstate_numeric_constant_handler(FILE *fd, int ch)
{
    if (ch >= '0' && ch <= '9')
    {
        pevent_data.data[event_data_idx++] = ch;
    }
    else
    {
        ungetc(ch, fd);
        set_parser_event(PSTATE_IDLE, PEVENT_NUMERIC_CONSTANT);
        return &pevent_data;
    }
    return NULL;
}

pevent_t *pstate_reserve_keyword_handler(FILE *fd, int ch)
{
    if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_')
    {
        word[word_idx++] = ch;
        pevent_data.data[event_data_idx++] = ch;
    }
    else
    {
        word[word_idx] = '\0';
        ungetc(ch, fd);
        int result = is_reserved_keyword(word);
        if (result)
        {
            pevent_data.property = result;
            set_parser_event(PSTATE_IDLE, PEVENT_RESERVE_KEYWORD);
        }
        else
        {
            set_parser_event(PSTATE_IDLE, PEVENT_IDENTIFIER);  // NEW IDENTIFIER HANDLER
        }
        return &pevent_data;
    }
    return NULL;
}
