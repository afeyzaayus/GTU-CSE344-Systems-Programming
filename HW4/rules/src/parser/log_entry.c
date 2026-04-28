#include <string.h>
#include "log_entry.h"



// --------------------------------------------------
// LEVEL PARSER
// --------------------------------------------------
static int	parse_level_str(char *str, t_level *level)
{
	if (strcmp(str, "ERROR") == 0)
		*level = LV_ERROR;
	else if (strcmp(str, "WARN") == 0)
		*level = LV_WARN;
	else if (strcmp(str, "INFO") == 0)
		*level = LV_INFO;
	else if (strcmp(str, "DEBUG") == 0)
		*level = LV_DEBUG;
	else
		return (0);
	return (1);
}

// --------------------------------------------------
// SKIP SPACES
// --------------------------------------------------
static void	skip_spaces(char **p)
{
	while (**p == ' ')
		(*p)++;
}

// --------------------------------------------------
// TIMESTAMP
// --------------------------------------------------
static int	handle_timestamp(t_log_entry *e, char **p)
{
	char	*start;
	int		len;

	if (**p != '[')
		return (0);
	(*p)++;
	start = *p;

	while (**p && **p != ']')
		(*p)++;
	if (**p != ']')
		return (0);

	len = *p - start;
	if (len != 19)
		return (0);

	strncpy(e->timestamp, start, 19);
	e->timestamp[19] = '\0';

	(*p)++;
	skip_spaces(p);
	return (1);
}

// --------------------------------------------------
// LEVEL
// --------------------------------------------------
static int	handle_level(t_log_entry *e, char **p)
{
	char	buffer[TMP_BUF_SIZE];
	char	*start;
	int		len;

	if (**p != '[')
		return (0);
	(*p)++;
	start = *p;

	while (**p && **p != ']')
		(*p)++;
	if (**p != ']')
		return (0);

	len = *p - start;
	if (len <= 0 || len >= TMP_BUF_SIZE)
		return (0);

	strncpy(buffer, start, len);
	buffer[len] = '\0';

	if (!parse_level_str(buffer, &e->level))
		return (0);

	(*p)++;
	skip_spaces(p);
	return (1);
}

// --------------------------------------------------
// SOURCE
// --------------------------------------------------
static int	handle_source(t_log_entry *e, char **p)
{
	char	*start;
	int		len;

	if (**p != '[')
		return (0);
	(*p)++;
	start = *p;

	while (**p && **p != ']')
		(*p)++;
	if (**p != ']')
		return (0);

	len = *p - start;
	if (len <= 0 || len >= MAX_SOURCE_LEN)
		return (0);

	strncpy(e->source, start, len);
	e->source[len] = '\0';

	(*p)++;
	skip_spaces(p);
	return (1);
}

// --------------------------------------------------
// MESSAGE
// --------------------------------------------------
static int	handle_message(t_log_entry *e, char **p)
{
	if (**p == '\0')
		return (0);

	strncpy(e->message, *p, MAX_MESSAGE_LEN - 1);
	e->message[MAX_MESSAGE_LEN - 1] = '\0';

	e->is_eof = 0;
	return (1);
}

// --------------------------------------------------
// MAIN PARSER
// --------------------------------------------------
int	parse_log_line(char *line, t_log_entry *e)
{
	char	*p;

	p = line;

	if (!handle_timestamp(e, &p))
		return (0);
	if (!handle_level(e, &p))
		return (0);
	if (!handle_source(e, &p))
		return (0);
	if (!handle_message(e, &p))
		return (0);

	return (1);
}