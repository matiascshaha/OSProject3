#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	int size;
	char **items;
} tokenlist;

char *get_input(void);
tokenlist *get_tokens(char *input);

tokenlist *new_tokenlist(void);
void add_token(tokenlist *tokens, char *item);
void free_tokens(tokenlist *tokens);

int main()
{
	while(1)
	{
		printf("$ ");
		char *input = get_input();
		//printf("whole input: %s\n", input);

		tokenlist *tokens = get_tokens(input);
		/*for (int i = 0; i < tokens->size; i++) {
			printf("token %d: (%s)\n", i, tokens->items[i]);
		}*/

		if(strcmp(tokens->items[0], "exit") == 0)
			break;

		else if(strcmp(tokens->items[0], "info") == 0)
		{}

		else if(strcmp(tokens->items[0], "size") == 0)
		{}

		else if(strcmp(tokens->items[0], "ls") == 0)
		{}

		else if(strcmp(tokens->items[0], "cd") == 0)
		{}

		else if(strcmp(tokens->items[0], "creat") == 0)
		{}

		else if(strcmp(tokens->items[0], "mkdir") == 0)
		{}

		else if(strcmp(tokens->items[0], "mv") == 0)
		{}

		else if(strcmp(tokens->items[0], "open") == 0)
		{}

		else if(strcmp(tokens->items[0], "close") == 0)
		{}

		else if(strcmp(tokens->items[0], "lseek") == 0)
		{}

		else if(strcmp(tokens->items[0], "read") == 0)
		{}

		else if(strcmp(tokens->items[0], "write") == 0)
		{}

		else if(strcmp(tokens->items[0], "rm") == 0)
		{}

		else if(strcmp(tokens->items[0], "cp") == 0)
		{}

		/*extra credit if we get to it
		else if(strcmp(tokens->items[0], "rmdir") == 0)
		{}

		else if(strcmp(tokens->items[0], "cp") == 0 && 
				strcmp(tokens->items[1] == "-r") == 0)
		{}
		*/
		else //not a recognized command
			printf("%s: command not found\n", tokens->items[0]);

		free(input);
		free_tokens(tokens);
	}
	//free dynamic memory if necessary
	return 0;
}

tokenlist *new_tokenlist(void)
{
	tokenlist *tokens = (tokenlist *) malloc(sizeof(tokenlist));
	tokens->size = 0;
	tokens->items = (char **) malloc(sizeof(char *));
	tokens->items[0] = NULL; /* make NULL terminated */
	return tokens;
}

void add_token(tokenlist *tokens, char *item)
{
	int i = tokens->size;

	tokens->items = (char **) realloc(tokens->items, (i + 2) * sizeof(char *));
	tokens->items[i] = (char *) malloc(strlen(item) + 1);
	tokens->items[i + 1] = NULL;
	strcpy(tokens->items[i], item);

	tokens->size += 1;
}

char *get_input(void)
{
	char *buffer = NULL;
	int bufsize = 0;

	char line[5];
	while (fgets(line, 5, stdin) != NULL) {
		int addby = 0;
		char *newln = strchr(line, '\n');
		if (newln != NULL)
			addby = newln - line;
		else
			addby = 5 - 1;

		buffer = (char *) realloc(buffer, bufsize + addby);
		memcpy(&buffer[bufsize], line, addby);
		bufsize += addby;

		if (newln != NULL)
			break;
	}

	buffer = (char *) realloc(buffer, bufsize + 1);
	buffer[bufsize] = 0;

	return buffer;
}

tokenlist *get_tokens(char *input)
{
	char *buf = (char *) malloc(strlen(input) + 1);
	strcpy(buf, input);

	tokenlist *tokens = new_tokenlist();

	char *tok = strtok(buf, " ");
	while (tok != NULL) {
		add_token(tokens, tok);
		tok = strtok(NULL, " ");
	}

	free(buf);
	return tokens;
}

void free_tokens(tokenlist *tokens)
{
	for (int i = 0; i < tokens->size; i++)
		free(tokens->items[i]);
	free(tokens->items);
	free(tokens);
}
