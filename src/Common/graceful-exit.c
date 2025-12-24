#include <stdio.h>
#include <stdlib.h>

#include "graceful-exit.h"

void closeSingleContext(gful_lifo **graceful_lifo){
    if(!graceful_lifo){
        printf("Invalid context.\n");
        return;
    }
    ((*graceful_lifo)->pFunction)((void*)(*graceful_lifo)->pArguments);
    gful_lifo *temp = *graceful_lifo;
    *graceful_lifo = (*graceful_lifo)->pPreviousStruct;
    free(temp);
}

void closeContexts(struct contextDataforClosing *Data){
    if(!Data){
        printf("Invalid context.\n");
        return;
    }
    closeContextsRecusive(Data);
}

void closeContextsRecusive(struct contextDataforClosing *Data){
    printf("Executing close contexts\n");
    (Data->pFunction)((void*)Data->pArguments);
    printf("Completed close contexts\n");
    printf("%p\n", Data->pPreviousStruct);
    if((Data->pPreviousStruct)){
        printf("Recurisng\n");
        closeContexts(Data->pPreviousStruct);
    }
    free(Data);
}

void createContextDataforClosing(   void *pInFunction, void *pInArguments, 
                                    struct contextDataforClosing **pInPreviousStruct){

    struct contextDataforClosing *newStruct = malloc(sizeof(struct contextDataforClosing));

    if(!newStruct){
        printf("Failed to create programContext(malloc).\n");
        closeContexts(*pInPreviousStruct);
        exit(1);
    }

    newStruct->pFunction = pInFunction;
    newStruct->pArguments = pInArguments;
    newStruct->pPreviousStruct = *pInPreviousStruct;
    *pInPreviousStruct = newStruct;
    return;
}