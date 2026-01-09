#ifndef GRACEFULEXIT_H
#define GRACEFULEXIT_H

#define GFUL_INIT NULL

typedef void (genericfunction)(void);

typedef struct contextDataforClosing{
    genericfunction *function;
    void *pArguments;
    struct contextDataforClosing *pPreviousStruct;
}gful_lifo;

void closeSingleContext(gful_lifo **graceful_lifo);

void closeContexts(struct contextDataforClosing *);

void closeContextsRecursive(struct contextDataforClosing *);

void createContextDataforClosing(   genericfunction *, void *, 
                                    struct contextDataforClosing **);

#endif //GRACEFULEXIT_H
