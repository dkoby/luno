
C language
==========

Naming convetions
-----------------

Camel notation should be used. Names should start with low case letter.
Structure, enum declarations should have ***\_t*** suffix.
Examples:

    struct someStructTypeHere_t
    {
        /* ... */
    };

    enum tokenType_t tokenType;
    struct deviceState_t deviceState;
    struct deviceState_t devState;
    int refCounter;

Simple letters, abbrevations, etc can be named  in low case.

    int   i;    /* Index. */
    char* mb;   /* Memory block. */
    void* bdev; /* Block device. */

Function name should start with same name as file name of ".c" file contained
this function. For example functions inside of file "test.c" should be named
like follows:

    void testInit()
    {

    }

    void testProcess()
    {

    }

File names should be low case.

    void testrequestInit()
    {

    }

    void testrequestProcess()
    {

    }


Local (static) functions should separte file name with ***\_*** prefix.

    static int testrequest_getResult()
    {
        /* ... */
    }

Typedefs allowed only for primitive types and not allowed for structures
and enums.

    typedef unt8_t u8
    typedef int32_t i8

Macro names must be upper case. Words must be separated by ***\_*** character.

    #define TEST_FEATURE
    #define SOME_MACRO_WITH_ARGUMENTS(a, b) do {test(a, b);} while (0)

Code formatting
---------------

Tabs are 4 space characters. Tab characters not allowed.

If possible on line should not exceed 80 characters.

Function implementation should be preceding with comment block:

    /*
     * Some function.
     */
    void someFunction()
    {
    }

Function arguments and return description in comment block:

    /*
     * Display character on screen.
     *
     * ARGS
     *     gc      Graphical context.
     *     ch      Character to put.
     *     x       x coordinate.
     *     y       y coordinate.
     *     font    Font to use.
     */
    void gsPutChar(struct gcontext_t *gc, char ch, int x, int y, struct font_t *font)
    {
        /* ... */
    }

    /*
     * ARGS
     *     eth    Pointer to ethernet interface structure.
     *
     * RETURN
     *     Zero on success, -1 on error.
     */
    int ethInit(struct ethInterface_t *eth)
    {
        /* ... */
    }

Comments should start with upper case and terminated with dot.

    /*
     * Some important comment.
     */

    /* Less important comment. */

    /*
     * Some comment that span multiple lines. Have same formatting as important
     * comment block.
     */

    u16 length; /* Length of whole message. */
    u8  crc;    /* CRC of message. */

If some code should be notted use ***NOTE*** comment or even ***XXX***.

    val = -1; /* NOTE */

    /* NOTE */
    block->next = NULL;

    /* XXX Be care of this. */
    if (capacity > SOME_VALUE)
    {
        /* ... */
    }

Not reached code should be commented with ***NOTREACHED*** statement.

    if (condition)
    {
        /* NOTREACHED */
        debug("Unreached");
    }
    
Open bracket should be on other line for ***if, while, for, ...*** and function
declarations. Statements ***else, elese if*** can have open bracket on same line.

    for (;;)
    {

    }

    void someFunction()
    {
        /* ... */
        if (conditionOne)
        {

        } else if (conditionTwo) {

        } else {

        }
    }

Assigment statements for same logical unit should be aligned.

    blob->length  = length;
    blob->next    = NULL;
    blob->value64 = 0;
    
Other
-----

System/external library headers should precede other headers.
Include directive for corresponding source *.c file should be separated by /* */.
If possible, headers should be sorted by name.

	#include <stdlib.h>
	#include <string.h>
    #include <sys/time.h>
    #include <sys/types.h>
    #include <unistd.h>
	/* */
	#include "server.h"
	/* */
	#include "client.h"
	#include "debug.h"

	/*
     *
     */
	int serverInit()
	{

	}

