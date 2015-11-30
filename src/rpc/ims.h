/*******************************************************************************
 *	ims.h
 *
 *  gsoap declarations
 *
 *
 *  This file is part of PSD-IMS
 * 
 *  Copyright (C) 2015  Daniel Pinto Rivero, Javier Bermúdez Blanco
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ********************************************************************************/

//gsoap psdims service name: psdims
//gsoap psdims service style: rpc
//gsoap psdims service location: http://localhost:10000
//gsoap psdims service encoding: literal
//gsoap psdims service namespace: urn:psdims

typedef enum psdims__notification_type { FRIEND_REQ, PENDING_MESS } psdims__notification_type;

typedef struct psdims__string {
	char *string;
} psdims__string;

typedef struct psdims__notif_friend_info {
	psdims__string name;
	int send_date;
} psdims__notif_friend_info;

typedef struct psdims__notif_friend_list {
	int __sizenelems;	
	psdims__notif_friend_info *user;
} psdims__notif_friend_list;

typedef struct psdims__notif_chat_info {
	int chat_id;
} psdims__notif_chat_info;

typedef struct psdims__notif_chat_list {
	int __sizenelems;
	psdims__notif_chat_info *chat;
} psdims__notif_chat_list;

typedef struct psdims__notifications {
	psdims__notif_friend_list friend_request;
	psdims__notif_friend_list deleted_friends;
	psdims__notif_chat_list new_chats;
	psdims__notif_chat_list deleted_chats;
	psdims__notif_chat_list chats_with_messages;
	int last_timestamp;
} psdims__notifications;

typedef struct psdims__new_chat {
	char *description;
	char *user;	
} psdims__new_chat;

typedef struct psdims__login_info {
	char *name;
	char *password;
} psdims__login_info;

typedef struct psdims__register_info {
	char *name;
	char *password;
	char *information;
} psdims__register_info;

// Users
typedef struct psdims__user_info {
	char *name;
	char *information;
} psdims__user_info;

typedef struct psdims__user_list {
	int __sizenelems;
	psdims__user_info *user;
	int last_timestamp;
} psdims__user_list;


// Messages
typedef struct psdims__message_info {
	char *user;
	char *text;
	int send_date;
} psdims__message_info;

typedef struct psdims__message_list {
	int __sizenelems;
	psdims__message_info *messages;
	int last_timestamp;
} psdims__message_list;

// Chats and chat members
typedef struct psdims__member_list {
	int __sizenelems;
	psdims__string *name;
	int last_timestamp;
} psdims__member_list;

typedef struct psdims__chat_info {
	int chat_id;
	char *description;
	char *admin;
	psdims__member_list *members;
} psdims__chat_info;

typedef struct psdims__chat_list {
	int __sizenelems;
	psdims__chat_info *chat_info;
	int last_timestamp;
} psdims__chat_list;



/********************************************************************
 * Basicas
 ********************************************************************/
// register user
int psdims__user_register(psdims__register_info *user_info, int *ERRCODE);

// borrar user
int psdims__user_unregister(psdims__login_info *login, int *ERRCODE);

// get user information
int psdims__get_user(psdims__login_info *login, psdims__user_info *user);

// get friend list
int psdims__get_friends(psdims__login_info *login, int timestamp, psdims__user_list *friends);

// get chat list
int psdims__get_chats(psdims__login_info *login, int timestamp, psdims__chat_list *chats);

// get chat info
int psdims__get_chat_info(psdims__login_info *login, int chat_id, psdims__chat_info *chat);

// get messages from chat
int psdims__get_chat_messages(psdims__login_info *login, int chat_id, int timestamp, psdims__message_list *messages);

// get pending notifications
int psdims__get_pending_notifications(psdims__login_info *login, int timestamp, psdims__notifications *notifications);

// create new chat
int psdims__create_chat(psdims__login_info *login, psdims__new_chat *new_chat, int *chat_id);

// add member to chat
int psdims__add_member(psdims__login_info *login, char *name, int chat_id, int *ERRCODE);

// quit from chat
int psdims__quit_from_chat(psdims__login_info *login, int chat_id, int *ERRCODE);

// Send message
int psdims__send_message(psdims__login_info *login, int chat_id, psdims__message_info *message, int *timestamp);

// enviar solicitud de amistad a usuario
int psdims__send_friend_request(psdims__login_info *login, char* request_name, int *timestamp);

// aceptar solicitud de amistad
int psdims__accept_request(psdims__login_info *login, char *request_name, int *timestamp);

// rechazar solicitud de amistad
int psdims__decline_request(psdims__login_info *login, char *request_name, int *timestamp);


/********************************************************************
 * Mensajes
 ********************************************************************/
// enviar mensaje de texto a amigo
// recibir mensajes de texto pendientes
// recibir confirmación de entrega de mensajes enviados

/********************************************************************
 * Grupos
 ********************************************************************/
// crear grupo
// pasar a otro user el rol de admin
// añadir amigo a un grupo del que soy admin
// borrar amigo de un grupo del que soy admin
// recibir aviso de mi adicion o borrado de grupo
// enviar mensaje a grupo (igual que enviar mensaje a usuario)
