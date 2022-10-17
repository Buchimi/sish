#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <string.h>
#include <unistd.h>

int getNoArgs(char *);
int main(int argc, char *argv[])
{
    size_t bufferSize;
    char *buffer;
    while (1)
    {
        // get the string
        printf("sish> ");
        size_t characters = getline(&buffer, &bufferSize, stdin);
        //

        // tokenize the string
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
            else
            {
                printf("pipe successful\n");
            }
        }
        char *token;
        char *saveptr;
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
                char *arguTok;
                char *arguDelim = " ";
                char *arguSavePtr;

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

                // fork and use the right pipe
                int cpid = fork();
                if (cpid == -1)
                {
                    perror("Fork failed");
                }
                else if (cpid == 0)
                {
                    for (int j = 0; j < noOfPipes * 2; j++)
                    {
                        if (j != (i * 2) || j != (i * 2 + 1))
                        {
                            // close the pipes we don't use
                            close(pipes[j]);
                        }
                    }
                    // child process
                    //  pipe

                    // execvp
                    execvp(argu[0], argu);
                }
                else
                {
                    // parent
                }
            }
            // printf("%s \n", token);
            // printf("we have %d args in first pipe \n", noArgs);
        }
        // printf("%s", buffer);
        // printf("we have %d pipes \n", noOfPipes);
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