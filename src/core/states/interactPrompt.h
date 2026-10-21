// interactPrompt.h - the popup shown over the dungeon: a Yes/No question or an OK message.
//
// Usage from another state:
//     interactPromptOpen("Open the door?", PROMPT_YES_NO);
// When the popup closes, the state underneath gets its resume() callback, and can ask
//     interactPromptAccepted()
// for the answer (true = Yes, or OK; false = No, or Esc).
#ifndef INTERACT_PROMPT_H
#define INTERACT_PROMPT_H

#include <stdbool.h>

typedef enum { PROMPT_YES_NO, PROMPT_OK, PROMPT_CHOICE } PromptKind;

#define PROMPT_MAX_CHOICES 6

// Registers STATE_INTERACT_PROMPT with the state manager.
void interactPromptRegister(void);

// Pushes the popup. The message is copied, so it may be a temporary string.
void interactPromptOpen(const char *message, PromptKind kind);

// Pushes a popup with a vertical menu of 1..PROMPT_MAX_CHOICES options. Labels and message are copied.
void interactPromptOpenChoice(const char *message, const char *const *labels, int count);

// The answer to the most recent Yes/No or OK popup.
bool interactPromptAccepted(void);

// The index of the option picked in the most recent choice menu.
int interactPromptChoice(void);

#endif