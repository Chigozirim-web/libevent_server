#ifndef PLAYER_H
#define PLAYER_H

#include "chlng.h"

#define PLAYER_STATE_NEW 0x00 /* new player */
#define PLAYER_STATE_WRONG 0x01 /* last guess was wrong */
#define PLAYER_STATE_GUESSED 0x02 /* last guess was correct */
#define PLAYER_STATE_CONTINUE 0x04 /* continue with another guess */
#define PLAYER_STATE_FINISHED 0x08 /* player finished the game */


typedef struct player 
{
    int state; /* state of the player */
    int solved; /* correctly solved challenges */
    int total; /* total number of challenges */
    chlng_t *chlng; /* current challenge */
} player_t;


/* Allocate a new player. */
extern player_t* player_new();


/* Reset the internal state of the player. */
extern void player_reset(player_t *p);


/* Delete a player and all its resources. */
extern void player_del(player_t *p);


/* Allocate a new challenge for the player. */
extern int player_fetch_chlng(player_t *p);


/* Retrieve a greeting message. */
extern int player_get_greeting(player_t *p, char **m);


/* Retrieve the challenge message. */
extern int player_get_challenge(player_t *p, char **m);


/* Post a message to the player and retrieve the response message. */
extern int player_post_challenge(player_t *p, char *guess, char **m);

/*

    if(strcmp(guess[0], "R") != 0)
    {
        printf("M: Send your guess in the form 'R: word\r\n'. \n");
        return -1;
    }
    if(strcmp(guess[1], ":") != 0)
    {
        printf("M: Send your guess in the form 'R: word\r\n'. \n");
        return -1;
    }
*/
#endif