#ifndef GRACEFULEXIT_H
#define GRACEFULEXIT_H

#define GFUL_INIT NULL

typedef struct contextDataforClosing{
    void (*pFunction)(void *);
    void *pArguments;
    struct contextDataforClosing *pPreviousStruct;
}gful_lifo;

void closeSingleContext(gful_lifo **graceful_lifo);

void closeContexts(struct contextDataforClosing *);

void closeContextsRecusive(struct contextDataforClosing *);

void createContextDataforClosing(   void *, void *, 
                                    struct contextDataforClosing **);

#endif //GRACEFULEXIT_H