/***********************************************************************************************************************
File: main.c                                                                

Description:
Container for the MPG firmware.  
***********************************************************************************************************************/

#include "configuration.h"

#ifndef SOLUTION
/***********************************************************************************************************************
Global variable definitions with scope across entire project.
All Global variable names shall start with "G_"
***********************************************************************************************************************/
/* New variables */

/*--------------------------------------------------------------------------------------------------------------------*/
/* External global variables defined in other files (must indicate which file they are defined in) */


/***********************************************************************************************************************
Global variable definitions with scope limited to this local application.
Variable names shall start with "Main_" and be declared as static.
***********************************************************************************************************************/
static u8 Main_u8Servers = 0; /* number of active servers */

/***********************************************************************************************************************
Main Program
***********************************************************************************************************************/

void main(void)
{
  u8 u8CurrentServer;
  ServerType sServer1;
  ServerType *psServerParser;
  
  psServerParser = &sServer1;
  sServer1.u8ServerNumber = 18;
  u8CurrentServer = psServerParser->u8ServerNumber;
  
} /* end main() */


/***********************************************************************************************************************
* Function definitions
***********************************************************************************************************************/
bool InitializeServer(ServerType **psServer_)
{
  if (*psServer_ == NULL)
    return(FALSE);
  return(TRUE);
  
  
  
  psServer->u8ServerNumber = Main_u8Servers;
  
  for (u8 i = 0; i < MAX_DRINKS; i++)
    psServer->asServingTray[i] = EMPTY;
}

/*--------------------------------------------------------------------------------------------------------------------*/
/* End of File */
/*--------------------------------------------------------------------------------------------------------------------*/
#endif /* SOLUTION */