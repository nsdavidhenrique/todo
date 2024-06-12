#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pwd.h>
#include <unistd.h>
#include <stdbool.h>
#include "../include/cJSON.h"

/* define */
struct taskStruct{
    bool done;
    char description[53];
};

/* data */
char *homedir;
char configdir[256] = {0};
char configfile[256] = {0};
char *argumentList[7] = {"add", "delete", "done", "undone", "clean", "empty", "help"};

int commandExist(char* command){
    int status = -1;
    for(int i = 0; i < sizeof(argumentList) / sizeof(argumentList[0]); i++){
        if(status == 0){
            return status;
        }
        status = strcmp(command, argumentList[i]);
    }
    if(status != 0){
        printf("Unknown argument: %s\n", command);
    }

    return status;
}

cJSON *parseJson(FILE *fptr){
    char *fileContent = NULL;
    cJSON *json = NULL;
    int fileLength = 0;
    int readReturn = 0;

    fptr = fopen(configfile, "rb");

    fseek(fptr, 0, SEEK_END);
    if((fileLength = ftell(fptr)) <= 0){
        printf("(fileLength = ftell(fptr)) <= 0 -> ");
        fclose(fptr);
        return NULL;
    }
    if((fileContent = (char*)malloc(sizeof(char) * fileLength + 1)) == NULL){
        printf("(fileContent = malloc()) == NULL -> ");
        fclose(fptr);
        return NULL;
    }
    fileContent[fileLength] = '\0';

    fseek(fptr, 0, SEEK_SET);
    readReturn = fread(fileContent, sizeof(char), fileLength, fptr);
    if(readReturn <= 0 || ferror(fptr)){
        printf("fread() -> ");
        free(fileContent);
        fclose(fptr);
        return NULL;
    }

    if((json = cJSON_Parse(fileContent)) == NULL){
        printf("(json = cJSON_Parse(fileContent)) == NULL -> ");
    }
    free(fileContent);
    fclose(fptr);

    return json;
}

void writeConfig(FILE *fptr, cJSON* json){
    char *string = NULL;

    // Create new config file.
    if(json == NULL){
        cJSON *tasks = NULL;

        json = cJSON_CreateObject();
        tasks = cJSON_CreateArray();
        cJSON_AddItemToObject(json, "tasks", tasks);
    }
    
    if((string = cJSON_Print(json)) == NULL){
        printf("(string = cJSON_Print(config)) == NULL -> Error Writing config");
        fclose(fptr);
        exit(EXIT_FAILURE);
    }

    fptr = fopen(configfile, "wb");
    fwrite(string, sizeof(char), strlen(string), fptr);
    fclose(fptr);

    cJSON_Delete(json);
}

void addTask(FILE *fptr, char* description){
    cJSON *json = NULL;
    cJSON *tasks = NULL;
    cJSON *task = NULL;

    if((json = parseJson(fptr)) == NULL){
        printf("Error parsing json\n");
        fclose(fptr);
        exit(EXIT_FAILURE);
    }
    if((tasks = cJSON_GetObjectItem(json, "tasks")) == NULL){
        printf("cJSON *tasks == NULL -> Error getting tasks object\n");
        fclose(fptr);
        cJSON_Delete(json);
        exit(EXIT_FAILURE);
    }

    task = cJSON_CreateObject();
    cJSON_AddStringToObject(task, "description", description);
    cJSON_AddBoolToObject(task, "done", false);
    cJSON_AddItemToArray(tasks, task);

    writeConfig(fptr, json);
}

void deleteTask(FILE *fptr, char* ID){
    cJSON *json = NULL;
    cJSON *tasks = NULL;
    long tasksLength = 0;
    long index = 0;
    char* endptr;

    if((json = parseJson(fptr)) == NULL){
        printf("Error parsing json\n");
        fclose(fptr);
        exit(EXIT_FAILURE);
    }
    if((tasks = cJSON_GetObjectItem(json, "tasks")) == NULL){
        printf("cJSON *tasks == NULL -> Error getting tasks object\n");
        fclose(fptr);
        cJSON_Delete(json);
        exit(EXIT_FAILURE);
    }

    if((index = strtol(ID, &endptr, 10)) == 0){
        printf("Task does not exist.\n");
    }
    tasksLength = cJSON_GetArraySize(tasks);
    if(index > tasksLength || index < 0){
        printf("Task does not exist.\n");
        return;
    }
    index--;

    cJSON_DeleteItemFromArray(tasks, index);

    writeConfig(fptr, json);
}

void doneTask(FILE *fptr, char* ID, cJSON_bool done){
    cJSON *json = NULL;
    cJSON *tasks = NULL;
    cJSON *task = NULL;
    cJSON *obj_done = NULL;
    long tasksLength = 0;
    long index = 0;
    char* endptr;

    if((json = parseJson(fptr)) == NULL){
        printf("Error parsing json\n");
        fclose(fptr);
        exit(EXIT_FAILURE);
    }
    if((tasks = cJSON_GetObjectItem(json, "tasks")) == NULL){
        printf("cJSON *tasks == NULL -> Error getting tasks object\n");
        fclose(fptr);
        cJSON_Delete(json);
        exit(EXIT_FAILURE);
    }


    if((index = strtol(ID, &endptr, 10)) == 0){
        printf("Task does not exist.\n");
    }
    tasksLength = cJSON_GetArraySize(tasks);
    if(index > tasksLength || index < 0){
        printf("Task does not exist.\n");
        return;
    }
    index--;

    task = cJSON_GetArrayItem(tasks, index);
    obj_done = cJSON_GetObjectItem(task, "done");

    if(done == true){
        cJSON_SetBoolValue(obj_done, true);
    } else{
        cJSON_SetBoolValue(obj_done, false);
    }

    writeConfig(fptr, json);
}

void clearTasks(FILE *fptr){
    cJSON *json = NULL;
    cJSON *tasks = NULL;
    int tasksLength = 0;

    if((json = parseJson(fptr)) == NULL){
        printf("Error parsing json\n");
        fclose(fptr);
        exit(EXIT_FAILURE);
    }
    if((tasks = cJSON_GetObjectItem(json, "tasks")) == NULL){
        printf("cJSON *tasks == NULL -> Error getting tasks object\n");
        fclose(fptr);
        cJSON_Delete(json);
        exit(EXIT_FAILURE);
    }

    tasksLength = cJSON_GetArraySize(tasks);
    for(int i =0; i < tasksLength; i++){
        cJSON *task = NULL;
        cJSON *done = NULL;

        task = cJSON_GetArrayItem(tasks, i);
        done = cJSON_GetObjectItem(task, "done");

        if(cJSON_IsTrue(done)){
            cJSON_DeleteItemFromArray(tasks, i);
        }
    }

    writeConfig(fptr, json);
}

void printTasks(FILE *fptr){
    cJSON *json = NULL;
    cJSON *tasks = NULL;
    char printDone[9];
    int tasksLength = 0;

    if((json = parseJson(fptr)) == NULL){
        printf("Error parsing json\n");
        fclose(fptr);
        exit(EXIT_FAILURE);
    }
    if((tasks = cJSON_GetObjectItem(json, "tasks")) == NULL){
        printf("cJSON *tasks == NULL -> Error getting tasks object\n");
        fclose(fptr);
        cJSON_Delete(json);
        exit(EXIT_FAILURE);
    }
    tasksLength = cJSON_GetArraySize(tasks);

    struct taskStruct *arr;
    int maxLength = 0;

    if(tasksLength > 0){
        arr = (struct taskStruct*)malloc(sizeof(struct taskStruct) * tasksLength);

        for(int i = 0; i < tasksLength; i++){
            cJSON *task = NULL;
            char *description = NULL;
            int descLength = 0;

            task = cJSON_GetArrayItem(tasks, i);
            description = cJSON_GetStringValue(cJSON_GetObjectItem(task, "description"));
            descLength = strlen(description);

            if(cJSON_IsTrue(cJSON_GetObjectItem(task, "done"))){
                arr[i].done = true;
            } else{
                arr[i].done = false;
            }

            if(maxLength < descLength){
                maxLength = descLength;
            }
            if(descLength > 52){
                strncpy(arr[i].description, description, 49);
                arr[i].description[49] = '.';
                arr[i].description[50] = '.';
                arr[i].description[51] = '.';
                arr[i].description[52] = '\0';
            } else{
                memset(arr[i].description, ' ', 52);
                strncpy(arr[i].description, description, descLength);
                arr[i].description[descLength] = '\0';
                arr[i].description[52] = '\0';
            }
        }

        // TODO Progresbar and pre-header
        
        printf("\n");
        if(maxLength <= 52){
            char* taskHeader;
            char* breakline;

            taskHeader = (char*)malloc(sizeof(char) * maxLength + 1);
            breakline = (char*)malloc(sizeof(char) * maxLength + 1);

            strncpy(taskHeader, "Task", 4);
            memset(&taskHeader[4], ' ', maxLength - 4);
            taskHeader[maxLength] = '\0';

            memset(breakline, '-', maxLength);
            breakline[maxLength] = '\0';

            printf("  ID   %s  Status  \n", taskHeader);
            printf("-------%s----------\n", breakline);

            for(int i = 0; i < tasksLength; i++){
                if(i < 10){
                    printf("  %d    ", i+1);
                } else if(i < 100){
                    printf("  %d   ", i+1);
                } else {
                    printf("  %d  ", i+1);
                }

                if(strlen(arr[i].description) < maxLength){
                    arr[i].description[strlen(arr[i].description)] = ' ';
                    arr[i].description[maxLength] = '\0';
                }

                printf("%s", arr[i].description);

                if(arr[i].done == true){
                    printf("   Done \n");
                } else{
                    printf("\n");
                }
            }

            free(taskHeader);
            free(breakline);
        } else{
            printf("  ID   Task                                                 Status  \n");
            printf("--------------------------------------------------------------------\n");

            for(int i = 0; i < tasksLength; i++){
                if(i < 10){
                    printf("  %d    ", i+1);
                } else if(i < 100){
                    printf("  %d   ", i+1);
                } else {
                    printf("  %d  ", i+1);
                }

                if(strlen(arr[i].description) < 52){
                    arr[i].description[strlen(arr[i].description)] = ' ';
                    arr[i].description[52] = '\0';
                }

                printf("%s", arr[i].description);

                if(arr[i].done == true){
                    printf("   Done \n");
                } else{
                    printf("\n");
                }
            }
        }
        printf("\n");

        free(arr);
    } else{
        printf("\n");
        printf("  ID   Task       Status  \n");
        printf("--------------------------\n");
        printf("  0    No tasks.\n");
        printf("\n");
    }

    cJSON_Delete(json);
}

int main(int argc, char *argv[]){

    if((homedir = getenv("HOME")) == NULL){     // Get homedir
        homedir = getpwuid(getuid())->pw_dir;   // Get homedir if Enviroment variable is not set
    }
    strncpy(configdir, homedir, strlen(homedir));
    strcat(configdir, "/.config/todo");

    if(mkdir(configdir, S_IRWXU | S_IRWXG | S_IRWXO) == -1){    // Create config folder
        if(errno != EEXIST){
            perror("Exit Failure: ");
            exit(EXIT_FAILURE);
        }
    }

    FILE *fptr; 
    strncpy(configfile, configdir, strlen(configdir));
    strcat(configfile, "/config.json");

    fptr = fopen(configfile, "rb+");
    if(fptr == NULL){
        writeConfig(fptr, NULL);
    }

    if(argc == 1){
        printTasks(fptr);
    } else if(commandExist(argv[1]) == 0){
        if(strcmp(argv[1], "add") == 0){
            if(argc == 3){
                addTask(fptr, argv[2]);
            } else if(argc == 2){
                printf("Missing task description.\n");
            } else{
                printf("Excess of arguments.\n");
            }
        } else if(strcmp(argv[1], "delete") == 0){
            if(argc == 3){
                deleteTask(fptr, argv[2]);
            } else if(argc == 2){
                printf("Missing ID.\n");
            } else{
                printf("Excess of arguments.\n");
            }
        } else if(strcmp(argv[1], "done") == 0){
            if(argc == 3){
                    doneTask(fptr, argv[2], true);
            } else if(argc == 2){
                printf("Missing ID.\n");
            } else{
                printf("Excess of arguments.\n");
            }
        } else if(strcmp(argv[1], "undone") == 0){
            if(argc == 3){
                doneTask(fptr, argv[2], false);
            } else if(argc == 2){
                printf("Missing ID.\n");
            } else{
                printf("Excess of arguments.\n");
            }
        } else if(strcmp(argv[1], "empty") == 0){
            if(argc == 2){
                writeConfig(fptr, NULL);
            } else{
                printf("Excess of arguments.\n");
            }
        } else{
            if(argc == 2){
                clearTasks(fptr);
            } else{
                printf("Excess of arguments.\n");
            }
        }
        printTasks(fptr);
    }

    return EXIT_SUCCESS;
}
