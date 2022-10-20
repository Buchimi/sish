#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int getNoArgs(char *);
void addToHistory(char *history[], int *historyIndex, char *args[]);
void printHistory(char *history[], int historyIndex);
void clearHistory(char *history[]);

int main(int argc, char *argv[])
{
    char *history[100];
    int historyIdx = 0;
    while (1)
    {
        size_t bufferSize;
        char *buffer;
        // get the string
        printf("sish> ");
        size_t characters = getline(&buffer, &bufferSize, stdin);
        //
        // tokenize the string
        buffer[characters - 1] = '\0';

        int noOfPipes = 0;
        for (int i = 0; buffer[i] != '\0'; i++)
        {
            if (buffer[i] == '|')
            {
                noOfPipes++;
            }
        }
        int pipes[noOfPipes * 2];

        for (int i = 0; i < noOfPipes; i++)
        {
            if (pipe(pipes + (2 * i)) == -1)
            {
                perror("pipe failed");
            }
        }

        char *token = NULL;
        char *saveptr = NULL;
        char *delim = "|";

        for (int i = 0;; buffer = NULL, i++)
        {

            token = __strtok_r(buffer, delim, &saveptr);
            if (token == NULL)
            {
                break;
            }
            int noArgs = getNoArgs(token);
            if (noArgs > 0)
            {
                char *argu[noArgs + 1];
                char *arguTok = NULL;
                char *arguDelim = " ";
                char *arguSavePtr = NULL;

                // tokenize and put in array
                for (int j = 0;; token = NULL, j++)
                {
                    arguTok = __strtok_r(token, arguDelim, &arguSavePtr);
                    argu[j] = arguTok;
                    if (arguTok == NULL)
                    {
                        break;
                    }
                }

                if (strcmp(argu[0], "exit") == 0)
                {

                    exit(0);
                }
                else if (strcmp(argu[0], "cd") == 0)
                {
                    if (argu[1] == NULL)
                    {
                        printf("cd usage: cd <directory>");
                    }
                    else
                    {
                        chdir(argu[1]);
                    }
                    addToHistory(history, &historyIdx, argu);
                    continue;
                }
                else if (strcmp(argu[0], "history") == 0)
                {

                    if (noArgs > 1 && strcmp(argu[1], "-c") == 0)
                    {
                        clearHistory(history);
                        historyIdx = 0;
                    }
                    else
                    {
                        printHistory(history, historyIdx);
                    }
                    addToHistory(history, &historyIdx, argu);
                    continue;
                }

                // fork and use the right pipe
                int cpid = fork();
                if (cpid == -1)
                {
                    perror("Fork failed");
                }
                else if (cpid == 0)
                {
                    for (int j = 0; j < (noOfPipes * 2); j++)
                    {
                        if (!(j == (i * 2 - 2) || j == (i * 2 + 1)))
                        {
                            // i * 2 - 2 is the read pipe, i * 2 + 1 is the write pipe
                            //  close the pipes we don't use
                            close(pipes[j]);
                            printf("We closed pipe %d in cmd %s", j, argu[0]);
                        }
                    }

                    // child process
                    //  pipe
                    if (i * 2 - 2 >= 0)
                    {
                        // read pipe
                        // this won't run for the first cmd before the very first pipe. why?
                        // it has an i == 0, thus a read fid at -2, which  is < 0
                        dup2(pipes[i * 2 - 2], STDIN_FILENO);
                        close(pipes[i * 2 - 2]);
                    }
                    if (i * 2 + 1 < noOfPipes * 2)
                    {
                        // say we have two pipes, i at the last pipe == 2.
                        // thus this won't run for the very last cmd since it'll attempt to write beyond the array
                        dup2(pipes[i * 2 + 1], STDOUT_FILENO);
                        close(pipes[i * 2 + 1]);
                    }
                    // execvp
                    if (execvp(argu[0], argu) == -1)
                    {
                        perror("execvp failed");
                    }
                }

                // parent
                // here we want to close the pipes we duped in child 2 ONLY

                addToHistory(history, &historyIdx, argu);
                if (i * 2 - 2 >= 0)
                {
                    // read pipe
                    close(pipes[i * 2 - 2]);
                }
                if (i * 2 + 1 < noOfPipes * 2)
                {
                    // write pipe
                    close(pipes[i * 2 + 1]);
                }

                // maybe we should wait for the child at this point
                waitpid(cpid, NULL, 0);
            }
        }


    }
}

int getNoArgs(char *cmd)
{
    int index = 0;
    int noOfArgs = 0;

    if (cmd[index] != ' ')
    {
        noOfArgs++;
        while (cmd[index] != ' ' && cmd[index] != '\0')
        {
            index++;
        }
    }

    while (cmd[index] == ' ')
    {
        index++;
        if (cmd[index] == '\0')
        {
            return noOfArgs;
        }
        else if (cmd[index] != ' ')
        {
            // if we have potential arguments
            noOfArgs++;
            while (cmd[index] != ' ' && cmd[index] != '\0')
            {
                // take it back to ' '
                index++;
            }
        }
    }
    return noOfArgs;
}

void addToHistory(char *history[], int *historyIndex, char *args[])
{
    // note, history is an array who's pointer starts at the end of the arr and moves backwards
    int count = 0;
    for (int i = 0; args[i] != NULL; i++)
    {
        count += strlen(args[i]) + 1;
    }

    char *str = (char *)malloc(sizeof(char) * (count));
    history[*historyIndex] = str;

    for (int i = 0; args[i] != NULL; i++)
    {
        strcat(str, args[i]);
        strcat(str, " ");
    }

    *historyIndex = (*historyIndex + 1) % 100;
}
void printHistory(char *history[], int historyIndex)
{
    int count = 0;
    historyIndex = (historyIndex - 1) % 100;
    if (history[historyIndex] == NULL)
    {
        return;
    }
    while (history[historyIndex] != NULL && count < 100)
    {
        printf("%d %s\n", count, history[historyIndex]);
        count++;
        historyIndex = (historyIndex + 99) % 100;
    }
}
void clearHistory(char *history[])
{
    for (int i = 0; i < 100; i++)
    {
        history[i] = NULL;
    }
}