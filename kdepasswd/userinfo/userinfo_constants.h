/***************************************************************************
 *   Copyright 2003 Braden MacDonald <bradenm_k@shaw.ca>                   *
 *   Copyright 2003 Ravikiran Rajagopal <ravi@ee.eng.ohio-state.edu>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License (version 2) as   *
 *   published by the Free Software Foundation.                            *
 *                                                                         *
 ***************************************************************************/
 /*
  * @file Constants for User Account (userinfo)
  */
#ifndef USERINFO_CONSTANTS_H
#define USERINFO_CONSTANTS_H

//////////////////////// Constants: ////////////////////////
#define USER_FACE_FILE     "/.face.icon"   // The file in the user's home dir that we change
#define USER_FACES_DIR     "/.faces/"      // User can install additional faces to ~/.faces/
#define USER_CUSTOM_FILE   "/Custom.png"   // An image set by this program, UserFacesDir+this
#define USER_CUSTOM_KEY    "Zz_custom"     // Do not change

#define SYS_DEFAULT_FILE   ".default.face.icon"  // The system-wide default image
#define KDM_FACES_DIR      "pics/users/"         // Directory where system-wide faces are stored (KDMDIR+this)
#define KDM_USER_FACES_DIR "faces/"              // Directory where kdm stores user pics (KDMDIR+this)

#define FACE_PIX_SIZE      64              // Default Size for faces is 60x60
#define FACE_BTN_SIZE      FACE_PIX_SIZE+10// Size for the Face button in the main dialog

// Move to an enum later on.
#define FACE_SRC_USERONLY    4
#define FACE_SRC_USERFIRST   3
#define FACE_SRC_ADMINFIRST  2    // USERONLY *MUST* be one more than USERFIRST,
#define FACE_SRC_ADMINONLY   1    // which must be one more than ADMINFIRST, etc.
////////////////////////////////////////////////////////////

#endif
