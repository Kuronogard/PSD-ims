/*******************************************************************************
 *	network.c
 *
 *  client network management
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

#include "soapH.h"
#include "psdims.nsmap"
#include "network.h"
#include "psd_ims_client.h"

#include <stdlib.h>

#include "debug_def.h"

#ifdef DEBUG
#include "leak_detector_c.h"
#endif


/*
 *
 *
 */
network *net_new(char *serverURL) {
	network *new_network;
	if( (new_network = malloc( sizeof(network) )) == NULL ) {
		DEBUG_FAILURE_PRINTF("Could not allocate network estructure");
		return NULL;
	}
	if( (new_network->serverURL = malloc( sizeof(char)*(strlen(serverURL)+1) )) == NULL ) {
		DEBUG_FAILURE_PRINTF("Could not allocate network estructure");
		free(new_network);
		return NULL;
	}
	strcpy(new_network->serverURL,serverURL);
	new_network->login_info.name = NULL;
	new_network->login_info.password = NULL;	

	soap_init(&new_network->soap);
	return new_network;
}


/*
 *
 *
 */
void net_free(network *network) {
	soap_end(&network->soap);
	soap_done(&network->soap);

	free(network->serverURL);
	free(network->login_info.name);
	free(network->login_info.password);
	network->serverURL = NULL;
	network->login_info.name = NULL;
	network->login_info.password = NULL;	
	free(network);
}


/*
 *
 *
 */
psdims__user_info *net_login(network *network, char *name, char *password) {
	int soap_response = 0;
	psdims__login_info login_info;
	psdims__user_info *user_info;
	char *soap_error;

	login_info.name = name;
	login_info.password = password;

	if ( (user_info = malloc(sizeof(psdims__user_info)) ) == NULL ) {
		DEBUG_FAILURE_PRINTF("Could not allocate memory for user info");
		return NULL;
	}

	soap_response = soap_call_psdims__get_user(&network->soap, network->serverURL, "", &login_info, user_info);
	if( soap_response != SOAP_OK ) {
		soap_error = malloc(sizeof(char)*200);
		soap_sprint_fault(&network->soap, soap_error, sizeof(char)*200);
		DEBUG_FAILURE_PRINTF("Server request failed: %s", soap_error);
		free(soap_error);
		free(user_info);
		return NULL;
	}

	if ( (network->login_info.name = malloc(strlen(name) + sizeof(char)) ) == NULL ) {
		DEBUG_FAILURE_PRINTF("Could not allocate memory for network user name");
		return NULL;
	}
	if ( (login_info.password = malloc(strlen(name) + sizeof(char)) ) == NULL ) {
		DEBUG_FAILURE_PRINTF("Could not allocate memory for network pass name");
		free(network->login_info.name);
		network->login_info.name = NULL;
		return NULL;
	}

	strcpy(network->login_info.name, name);
	strcpy(network->login_info.password, password);

	return user_info;
}


/*
 *
 *
 */
psdims__notification_list *net_recv_notifications(network *network) {
	int soap_response = 0;
	psdims__notification_list *notification_list;
	char *soap_error;


	if ( (notification_list = malloc(sizeof(psdims__notification_list)) ) == NULL ) {
		DEBUG_FAILURE_PRINTF("Could not allocate memory for notification list");
		return NULL;
	}

	soap_response = soap_call_psdims__get_pending_notifications(&network->soap, network->serverURL, "", &network->login_info, notification_list);
	if( soap_response != SOAP_OK ) {
		soap_error = malloc(sizeof(char)*200);
		soap_sprint_fault(&network->soap, soap_error, sizeof(char)*200);
		DEBUG_FAILURE_PRINTF("Server request failed: %s", soap_error);
		free(soap_error);
		free(notification_list);
		return NULL;
	}

	return notification_list;
}


/*
 *
 *
 */
psdims__message_list *net_recv_pending_messages(network *network, int chat_id) {
	int soap_response = 0;
	psdims__message_list *message_list;
	char *soap_error;


	if ( (message_list = malloc(sizeof(psdims__message_list)) ) == NULL ) {
		DEBUG_FAILURE_PRINTF("Could not allocate memory for message list");
		return NULL;
	}

	soap_response = soap_call_psdims__get_chat_messages(&network->soap, network->serverURL, "", &network->login_info, chat_id, message_list);
	if( soap_response != SOAP_OK ) {
		soap_error = malloc(sizeof(char)*200);
		soap_sprint_fault(&network->soap, soap_error, sizeof(char)*200);
		DEBUG_FAILURE_PRINTF("Server request failed: %s", soap_error);
		free(soap_error);
		free(message_list);
		return NULL;
	}

	return message_list;
}


/*
 *
 *
 */
psdims__chat_list *net_recv_new_chats(network *network) {
	int soap_response = 0;
	psdims__chat_list *chat_list;
	char *soap_error;


	if ( (chat_list = malloc(sizeof(psdims__chat_list)) ) == NULL ) {
		DEBUG_FAILURE_PRINTF("Could not allocate memory for chat list");
		return NULL;
	}

	soap_response = soap_call_psdims__get_chats(&network->soap, network->serverURL, "", &network->login_info, chat_list);
	if( soap_response != SOAP_OK ) {
		soap_error = malloc(sizeof(char)*200);
		soap_sprint_fault(&network->soap, soap_error, sizeof(char)*200);
		DEBUG_FAILURE_PRINTF("Server request failed: %s", soap_error);
		free(soap_error);
		free(chat_list);
		return NULL;
	}

	return chat_list;
}


/*
 *
 *
 */
int net_user_register(network *network, char *name, char *password, char *information){
	int soap_response = 0;
	psdims__register_info user_info;

	user_info.name = name;
	user_info.password = password;
	user_info.information = information;

	if( soap_call_psdims__user_register(&network->soap, network->serverURL, "", &user_info, &soap_response) != SOAP_OK ) {
		DEBUG_FAILURE_PRINTF("Server request failed");
		return -1;
	}

	// Comprobar error del servidor
	return 0;
}


/*
 *
 *
 */
int net_send_message(network *network, int chat_id, char *text, char *attach_path) {
	int soap_response = 0;
	psdims__message_info message_info;

	message_info.user = NULL;
	message_info.text = text;
	// TODO Falta el archivo adjunto

	if( soap_call_psdims__send_message(&network->soap, network->serverURL, "", &network->login_info, chat_id, &message_info, &soap_response) != SOAP_OK ) {
		DEBUG_FAILURE_PRINTF("Server request failed");
		return -1;
	}

	// Comprobar error del servidor
	return 0;
}


/*
 *
 *
 */
int net_send_friend_request(network *network, char *user) {
	int soap_response = 0;

	soap_call_psdims__send_friend_request(&network->soap, network->serverURL, "", &network->login_info, user, &soap_response);

	if( soap_response != SOAP_OK ) {
		DEBUG_FAILURE_PRINTF("Server request failed");
		return -1;
	}

	// Comprobar error del servidor
	return 0;
}


/*
 *
 *
 */
int net_send_request_accept(network *network, char *user) {
	int soap_response = 0;

	soap_call_psdims__accept_request(&network->soap, network->serverURL, "", &network->login_info, user, &soap_response);
	if( soap_response != SOAP_OK ) {
		DEBUG_FAILURE_PRINTF("Server request failed");
		return -1;
	}

	// Comprobar error del servidor
	return 0;
}


/*
 *
 *
 */
int net_send_request_decline(network *network, char *user) {
	int soap_response = 0;

	soap_call_psdims__decline_request(&network->soap, network->serverURL, "", &network->login_info, user, &soap_response);
	if( soap_response != SOAP_OK ) {
		DEBUG_FAILURE_PRINTF("Server request failed");
		return -1;
	}

	// Comprobar error del servidor
	return 0;
}


/*
 *
 *
 */
void net_free_user() {

}


void net_free_user_list() {

}


void net_free_notification() {

}


void net_free_notification_list() {

}


void net_free_message() {

}


void net_free_message_list() {

}


void net_free_chat() {

}


void net_free_chat_list() {

}


