#ifndef GUARD_DEBUG_H
#define GUARD_DEBUG_H

// HACK: menu de debug. Só existe na versão de teste (make debug / DEBUG=1).
#if DEBUG_MENU
extern bool8 gDebugNoClip;
extern bool8 gDebugNoEncounters;
extern bool8 gDebugNoTrainerSight;

void Debug_OpenMenu(void);
#endif

#endif // GUARD_DEBUG_H
