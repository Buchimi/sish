#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int getNoArgs(char *);                                                                  // this function returns the number of "args" or number commands seperatted by a space
void addToHistory(char *history[], int *historyIndex, char *buffer, int *historyStart); // add's command to the history
void printHistory(char *history[], int historyStart);
void clearHistory(char *history[]);

int main(int argc, char *argv[])
{

    char *history[100]; // initializes the history to have a max of 100
    for (int i = 0; i < 100; i++)
    {
        history[i] = NULL; // set's every element to null
    }
    int historyStart = 0; // the pointer to the start of the history
    int historyIdx = 0;   // pointer to the next empty slot in the history
    int fromHist = 0;     // boolean for if this command is called from the history
    char *cmdFromHistory; // copy of the command from history
    while (1)
    {
        // prereqs for getline
        size_t bufferSize;
        char *buffer;
        size_t characters;

        if (fromHist)
        {
            // if this command is from history, we don't want to print nor get the line
            fromHist = 0;
            buffer = cmdFromHistory;
        }
        else
        {
            // get the string
            printf("sish> ");
            characters = getline(&buffer, &bufferSize, stdin);
            buffer[characters - 1] = '\0';
            addToHistory(history, &historyIdx, buffer, &historyStart);
        }

        // find the number of pipes. This is important for figuring out the amount of pipes
        int noOfPipes = 0;
        for (int i = 0; buffer[i] != '\0'; i++)
        {
            if (buffer[i] == '|')
            {
                noOfPipes++;
            }
        }

        // initialize a dynamic pipe array
        int *pipes = (int *)(malloc(sizeof(int) * noOfPipes * 2));

        // create as many pipes we need in the array.
        // odd indexes are read file descriptors, even ones are write file descriptors
        for (int i = 0; i < noOfPipes; i++)
        {
            if (pipe(pipes + (2 * i)) == -1)
            {
                perror("pipe failed");
            }
        }

        // here we tokenize the string by separating each command separated by pipes
        char *token = NULL;
        char *saveptr = NULL;
        char *delim = "|";

        // we break the loop if the token equals NULL
        for (int i = 0;; buffer = NULL, i++)
        {

            token = __strtok_r(buffer, delim, &saveptr);
            if (token == NULL)
            {

                break;
            }
            // get the number of args. This is important if
            int noArgs = getNoArgs(token);
            if (noArgs > 0)
            {
                char *argu[noArgs + 1];
                char *arguTok = NULL;
                char *arguDelim = " ";
                char *arguSavePtr = NULL;

                // tokenize and put the commands in an array
                for (int j = 0;; token = NULL, j++)
                {
                    arguTok = __strtok_r(token, arguDelim, &arguSavePtr);
                    argu[j] = arguTok;
                    if (arguTok == NULL)
                    {
                        break;
                    }
                }

                // if we found an exit command
                if (strcmp(argu[0], "exit") == 0)
                {
                    // clean up the pipes
                    free(pipes);
                    for (int i = 0; i < 100; i++)
                    {
                        if (history[i] != NULL)
                        {
                            // since every entry is manually allocated, free everything
                            free(history[i]);
                        }
                    }
                    exit(0);
                }
                // if we find a cd command
                else if (strcmp(argu[0], "cd") == 0)
                {
                    if (argu[1] == NULL)
                    {
                        // incase there is no dir provided
                        printf("cd usage: cd <directory> \n");
                    }
                    else
                    {
                        // change dir
                        chdir(argu[1]);
                    }
                    continue;
                }
                // if we have a history command
                else if (strcmp(argu[0], "history") == 0)
                {

                    // if we have more than one arrgument and the second one is '-c'
                    if (noArgs > 1 && strcmp(argu[1], "-c") == 0)
                    {
                        clearHistory(history);
                        historyIdx = 0;
                    }
                    // if we just have more than one arg, it's a number arg
                    else if (noArgs > 1)
                    {
                        int translatedPtr = atoi(argu[1]) + historyStart;

                        if (translatedPtr > 99 || history[translatedPtr] == NULL)
                        {
                            // this is beyond our history array
                            printf("Invalid history index");
                        }
                        // if we do have this command
                        else if (history[translatedPtr] != NULL)
                        {
                            cmdFromHistory = history[translatedPtr]; // get the history command
                            fromHist = 1;                            // let sish know this command is from the history
                        }
                    }
                    // only one arg, so it's a lone history command
                    else
                    {
                        printHistory(history, historyStart);
                    }
                    continue;
                }

                int cpid = fork();
                if (cpid == -1)
                {
                    perror("Fork failed");
                }
                else if (cpid == 0) // child process
                {
                    // assign pipes
                    for (int j = 0; j < (noOfPipes * 2); j++)
                    {
                        if (!(j == (i * 2 - 2) || j == (i * 2 + 1)))
                        {
                            // i * 2 - 2 is the read pipe, i * 2 + 1 is the write pipe
                            //  close the pipes we don't use
                            close(pipes[j]);
                        }
                    }

                    // redirect and close read pipe based on the rule
                    if (i * 2 - 2 >= 0)
                    {
                        // read pipe
                        // this won't run for the first cmd before the very first pipe. why?
                        // it has an i == 0, thus a read fid at -2, which  is < 0
                        dup2(pipes[i * 2 - 2], STDIN_FILENO);
                        close(pipes[i * 2 - 2]);
                    }
                    // redirect and close write pipes based on the rule
                    if (i * 2 + 1 < noOfPipes * 2)
                    {
                        // write pipe
                        //  say we have two pipes, i at the last pipe == 2.
                        //  thus this won't run for the very last cmd since it'll attempt to write beyond the array
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
                // here we want to close the pipes we redirected in child ONLY
                // this is so that the parent doesn't interfere with inter process communications
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

                // wait for child to finish executing
                waitpid(cpid, NULL, 0);
            }
            else
            {
                // this execs if commands end with an empty pipe
                printf("Illegal pipe '|' usage");
            }
        }
    }
}

int getNoArgs(char *cmd)
{
    int index = 0;
    int noOfArgs = 0;

    // if command starts with a letter
    if (cmd[index] != ' ')
    {
        noOfArgs++;
        while (cmd[index] != ' ' && cmd[index] != '\0')
        {
            // find the next space of end of string
            index++;
        }
    }
    // while we find a space
    while (cmd[index] == ' ')
    {
        index++;
        // if space ends in empty string, exit
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
                // take it to the next space or newline.
                // though if it's a newline we return at the bottom
                index++;
            }
        }
    }
    return noOfArgs;
}

void addToHistory(char *history[], int *historyIndex, char *str, int *historyStart)
{

    // copy string
    char *ptr = (char *)malloc(sizeof(char) * strlen(str));
    strcpy(ptr, str);

    // slap it into our history
    history[*historyIndex] = ptr;

    // increment or wrap to 0
    *historyIndex = (*historyIndex + 1) % 100;
    // check to see if we need to increase the history start index
    if (*historyIndex == *historyStart)
    {

        *historyStart = (*historyStart + 1) % 100;
    }
}
