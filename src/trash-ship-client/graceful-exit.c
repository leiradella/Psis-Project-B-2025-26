#include <stdio.h>
#include <stdlib.h>

#include "graceful-exit.h"

void closeSingleContext(gful_lifo **graceful_lifo){
    if(!graceful_lifo){
        printf("Invalid context.\n");
        return;
    }
    gful_lifo LastPosition = **graceful_lifo;
    if(LastPosition.pArguments == NULL)
    {
        ((void (*)(void)) LastPosition.function)();
    }
    else
    {
        ((void (*)(void *)) LastPosition.function)(LastPosition.pArguments);
    }
    gful_lifo *temp = *graceful_lifo;
    *graceful_lifo = (*graceful_lifo)->pPreviousStruct;
    free(temp);
}

void closeContexts(struct contextDataforClosing *Data){
    if(!Data){
        printf("Invalid context.\n");
        return;
    }
    closeContextsRecursive(Data);
}

void closeContextsRecursive(struct contextDataforClosing *Data){
    printf("Executing close contexts\n");
    gful_lifo LastPosition = *Data;
    if(LastPosition.pArguments == NULL)
    {
        ((void (*)(void)) LastPosition.function)();
    }
    else
    {
        ((void (*)(void *)) LastPosition.function)(LastPosition.pArguments);
    }    
    printf("Completed close contexts\n");
    printf("%p\n", (void *)Data->pPreviousStruct);
    if((Data->pPreviousStruct)){
        printf("Recurisng\n");
        closeContextsRecursive(Data->pPreviousStruct);
    }
    free(Data);
}

void createContextDataforClosing(   genericfunction *pInFunction, void *pInArguments, 
                                    struct contextDataforClosing **pInPreviousStruct){

    struct contextDataforClosing *newStruct = malloc(sizeof(struct contextDataforClosing));

    if(!newStruct){
        printf("Failed to create programContext(malloc).\n");
        closeContexts(*pInPreviousStruct);
        exit(1);
    }

    newStruct->function = pInFunction;
    newStruct->pArguments = pInArguments;
    newStruct->pPreviousStruct = *pInPreviousStruct;
    *pInPreviousStruct = newStruct;
    return;
}
