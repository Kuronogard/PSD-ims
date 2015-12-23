/*******************************************************************************
 *  client_graphic_v2.c
 *
 *  Graphic (console) interface for the client
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


#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <ctype.h>
#include "psd_ims_client.h"
#include "bool.h"
#include "errors.h"

#include "debug_def.h"

#define MAX_USER_NAME_CHARS 20
#define MAX_USER_PASS_CHARS 20
#define MAX_USER_INFO_CHARS 100
#define MAX_DESCRIPTION_CHARS 100
#define MAX_MESSAGE_CHARS 300
#define MAX_INPUT_CHARS 100

#define COMMAND_CHAR '/'

#define FLUSH_INPUT(ch) \
	while ( ((ch = getchar()) != '\n') && (ch != EOF) )


int scan_input_string(char *buffer, int max_chars) {
	char aux_char;
	int index = 0;
	
	while( (index < (max_chars-1)) && ((aux_char = getchar()) != '\n') ) {
		if (aux_char == EOF)
			return -1;
		buffer[index++] = aux_char;
	}
	buffer[index] = '\0';
	if (index >= max_chars) FLUSH_INPUT(aux_char);
	
	return index;
	
}
	

boolean is_numeric(char *str) {
	while(*str) {
		if(!isdigit(*str))
			return false;
		str++;
	}
	return true;
}


typedef enum {DEFAULT, EXIT, LOGIN, USER_MAIN, USER_FRIENDS} menu_type;

struct chat_data {
	int chat_id;
	boolean has_attach;
	char attach_path[MAX_INPUT_CHARS];
};

typedef struct client_graphic client_graphic;
struct client_graphic {
	psd_ims_client *client;
	void (*menu_update_screen)(psd_ims_client *client, void *data);
	void *screen_data;
	boolean continue_fetching;
	boolean continue_graphic;
	boolean screen_updateable;
	pthread_mutex_t continue_fetching_mutex;
	pthread_mutex_t continue_graphic_mutex;
	pthread_mutex_t screen_update_mutex;
	pthread_t notifications_tid;
	char input_buffer[MAX_INPUT_CHARS];
};

#define get_continue_fetching(graphic, fetch) \
		pthread_mutex_lock(&(graphic->continue_fetching_mutex)); \
		fetch = graphic->continue_fetching; \
		pthread_mutex_unlock(&(graphic->continue_fetching_mutex))

#define set_continue_fetching(graphic, fetch) \
		pthread_mutex_lock(&(graphic->continue_fetching_mutex)); \
		graphic->continue_fetching = fetch; \
		pthread_mutex_unlock(&(graphic->continue_fetching_mutex))

#define get_continue_graphic(graphic, cont) \
		pthread_mutex_lock(&(graphic->continue_graphic_mutex)); \
		cont = graphic->continue_graphic; \
		pthread_mutex_unlock(&(graphic->continue_graphic_mutex))

#define set_continue_graphic(graphic, cont) \
		pthread_mutex_lock(&(graphic->continue_graphic_mutex)); \
		graphic->continue_graphic = cont; \
		pthread_mutex_unlock(&(graphic->continue_graphic_mutex))

#define get_screen_updateable(graphic, updatebale) \
		pthread_mutex_lock(&(graphic->screen_update_mutex)); \
		updateable = graphic->screen_updateable; \
		pthread_mutex_unlock(&(graphic->screen_update_mutex))

#define set_screen_updateable(graphic, updatebale) \
		pthread_mutex_lock(&(graphic->screen_update_mutex)); \
		graphic->screen_updateable = updatebale; \
		pthread_mutex_unlock(&(graphic->screen_update_mutex))
		
#define set_screen_update(graphic, screen_update, data) \
		pthread_mutex_lock(&(graphic->screen_update_mutex)); \
		graphic->menu_update_screen = screen_update; \
		graphic->screen_data = data; \
		pthread_mutex_unlock(&(graphic->screen_update_mutex))
		
#define screen_update(graphic) \
		pthread_mutex_lock(&(graphic->screen_update_mutex)); \
		graphic->menu_update_screen(graphic->client, graphic->screen_data); \
		pthread_mutex_unlock(&(graphic->screen_update_mutex))


client_graphic *graphic_global;

void retrieve_user_data(psd_ims_client *client);
void stop_client(int sig);
void *notifications_fetch(void *arg);

int run_notifications_thread(client_graphic *graphic);
int configure_signal_handling();


void menu_header_show(const char *string) {
	write(1,"\E[H\E[2J",7);
	printf("=============================================\n");
	printf("              %s\n", string);
	printf("=============================================\n");
}

void menu_footer_show() {
	printf("\n -----------------------------------------------\n");
	printf(" -> ");
}

int get_user_input(char input[], int max_chars) {
	return scan_input_string(input, max_chars);
}

void wait_user() {
	char aux_char;
	printf("\n Press ENTER to continue...");
	FLUSH_INPUT(aux_char);
}

void save_state(psd_ims_client *client) {
	printf(" = Saving state =\n");
	printf(" (NOT implemented)\n");
}


/* =========================================================================
 *  Friend requests Menu
 * =========================================================================*/


int accept_friend_req(psd_ims_client *client, char *name) {
	if( psd_send_request_accept(client, name) != 0 ) {
		printf(" Could not accept the friend request\n");
		wait_user();
		return -1;
	}

	printf(" User '%s' accepted as friend\n", name);
	wait_user();
}


int decline_friend_req(psd_ims_client *client, char *name) {
	if( psd_send_request_decline(client, name) != 0 ) {
		printf(" Could not reject the friend request\n");
		wait_user();
		return -1;
	}

	printf(" User '%s' rejected as friend\n", name);
	wait_user();
}


void screen_friend_req_show(psd_ims_client *client, void *data) {
	req_iterator *iterator;
	char *name;
	int time;
	menu_header_show("PSD IMS - Friend requests");
	printf(" (/e)exit, (/a<name>)accept <name>, (/d<name>)decline <name>\n");
	printf(" --------------------------------------------\n");

	psd_begin_req_iteration(client, iterator);
	while(psd_req_iterator_valid(iterator)) {
		psd_req_iterator_name(iterator, name);
		psd_req_iterator_time(iterator, time);
		printf(" [%d]: %s\n", time, name);
		psd_req_iterator_next(client, iterator);
	}
	psd_end_req_iteration(client, iterator);
	menu_footer_show();
}


void menu_friend_req(client_graphic *graphic) {
	boolean exit = false;
	boolean cont;
	int ret;

	set_screen_update(graphic, screen_friend_req_show, NULL);

	do {
		set_screen_updateable(graphic, TRUE);
		screen_update(graphic);
		ret = get_user_input(graphic->input_buffer, MAX_INPUT_CHARS);
		set_screen_updateable(graphic, FALSE);
		
		if (ret < 0) return;
		if (ret < 2) continue;
		
		if (graphic->input_buffer[0] == COMMAND_CHAR) {
			switch(graphic->input_buffer[1]) {
				case 'e':
					exit = true;
					break;
				case 'a':
					accept_friend_req(graphic->client, &graphic->input_buffer[2]);
					break;
				case 'd':
					decline_friend_req(graphic->client, &graphic->input_buffer[2]);
					break;
			}
		} 
		
		get_continue_graphic(graphic, cont);
		cont = cont && !exit;
		
	} while( cont );
	
}


/* =========================================================================
 *  Chat Menu
 * =========================================================================*/

int add_member_to_chat(psd_ims_client *client, int chat_id) {
	char member[MAX_USER_NAME_CHARS];
	char *name, *information;
	friends_iterator *iterator;

	printf(" Friends \n");
	psd_begin_fri_iteration(client, iterator);
	while(psd_fri_iterator_valid(iterator)) {
		psd_fri_iterator_name(iterator, name);
		psd_fri_iterator_information(iterator, information);
		printf(" %s: %s\n", name, information);
		psd_fri_iterator_next(client, iterator);
	}
	psd_end_fri_iteration(client, iterator);
	
	printf("\n member: ");
	scan_input_string(member, MAX_USER_NAME_CHARS);

	if( psd_add_member_to_chat(client, member, chat_id) != 0 ) {
		printf(" FAIL: Could not add %s to the chat\n", member);
		wait_user();
		return -1;
	}
	return 0;
}


int delete_member_from_chat(psd_ims_client *client, int chat_id) {
	printf("Not implemented\n");
	wait_user();
	return -1;	
}


int leave_chat(psd_ims_client *client, int chat_id) {
	if( psd_quit_from_chat(client, chat_id) != 0 ) {
		printf(" Failed to leave the chat\n");
		wait_user();
		return -1;
	}

	return 0;
}


int attach_file(char *file_path, char *file_info, int max_info_chars) {
	FILE *fd;
	
	// An ugly way to see if a file exists...
	if( (fd = fopen(file_path, "r")) == NULL ) {
		DEBUG_FAILURE_PRINTF("Could not read the file");
		return -1;
	}
	fclose(fd);
		
	printf("\n file info: ");
	scan_input_string(file_info, max_info_chars);
	return 0;
}


int send_message(psd_ims_client *client, char *text, int chat_id, char *file_path, char *MIME_type, char *file_info) {

	if( psd_send_message(client, chat_id, text, file_path, MIME_type, file_info) != 0 ) {
		printf(" Failed to send the message\n");
		wait_user();
		return -1;
	}

	return 0;
}


void screen_chat_show(psd_ims_client *client, void *data_raw) {
	chat_mes_iterator *iterator;
	char *sender = NULL, *text = NULL, *attach_path = NULL;
	int time;
	time_t t;
	struct tm tm;
	
	
	struct chat_data *data;

	data = (struct chat_data*)data_raw;

	menu_header_show("PSD IMS - Chats");
	printf(" (/e)exit, (/a)add member, (/d)delete member, (/l)leave\n");
	printf(" (/t)<file> attach <file> to message, (/r)remove attach\n");
	printf(" --------------------------------------------\n");

	psd_begin_mes_iteration(client, data->chat_id, iterator);
	while(psd_mes_iterator_valid(iterator)) {
		psd_mes_iterator_sender(iterator, sender);
		psd_mes_iterator_text(iterator, text);
		psd_mes_iterator_time(iterator, time);
		t = (time_t)time;
		tm = *localtime(&t);
		psd_mes_iterator_attach_path(iterator, attach_path);
		printf(" (%02d:%02d:%02d) [%s]: %s %s\n", tm.tm_hour, tm.tm_min, tm.tm_sec, ((sender)? sender:"I") , text, (attach_path)?"[ATTACH]":"");
		psd_mes_iterator_next(iterator);
	}
	psd_end_mes_iteration(client, iterator);

	if(data->has_attach)
		printf(" attached file: %s", data->attach_path);
	menu_footer_show();
}


void menu_chat(client_graphic *graphic, int chat_id) {
	int ret = -1;
	boolean exit = FALSE;
	boolean cont = TRUE;
	char file_type[] = "";
	char file_info[MAX_DESCRIPTION_CHARS];


	struct chat_data *chat_data = malloc(sizeof(struct chat_data));
	chat_data->has_attach = FALSE;
	chat_data->chat_id = chat_id;

	set_screen_update(graphic, screen_chat_show, chat_data);


	do {
		set_screen_updateable(graphic, TRUE);
		screen_update(graphic);
		ret = get_user_input(graphic->input_buffer, MAX_INPUT_CHARS);
		set_screen_updateable(graphic, FALSE);
		
		if (ret < 0) return;
		if (ret < 1) continue;
		if (graphic->input_buffer[0] == COMMAND_CHAR) {
			if (ret < 2) continue;
			switch(graphic->input_buffer[1]) {
				case 'e':
					exit = true;
					break;
				case 'a':
					add_member_to_chat(graphic->client, chat_id);
					break;
				case 'd':
					delete_member_from_chat(graphic->client, chat_id);
					break;
				case 'l':
					leave_chat(graphic->client, chat_id);
					break;
				case 't':
					if (attach_file( &graphic->input_buffer[2], file_info, MAX_DESCRIPTION_CHARS) == 0) {
						chat_data->has_attach = TRUE;
						strcpy(chat_data->attach_path, &graphic->input_buffer[2]);
					}
					break;
				case 'r':
					chat_data->has_attach = FALSE;
					break;
			}
		} 
		else {
			if (chat_data->has_attach) 
				send_message(graphic->client, graphic->input_buffer, chat_id, chat_data->attach_path, file_type, file_info);
			else 
				send_message(graphic->client, graphic->input_buffer, chat_id, NULL, NULL, NULL);
			chat_data->has_attach = FALSE;
		}
		
		get_continue_graphic(graphic, cont);
		cont = cont && !exit;
		
	} while( cont );
}


/* =========================================================================
 *  Main Menu
 * =========================================================================*/

int create_new_chat(psd_ims_client *client) {
	char description[MAX_DESCRIPTION_CHARS];
	char member[MAX_USER_NAME_CHARS];
	int chat_id;
	char *name, *information;
	friends_iterator *iterator;

	printf(" Friends \n");
	psd_begin_fri_iteration(client, iterator);
	while(psd_fri_iterator_valid(iterator)) {
		psd_fri_iterator_name(iterator, name);
		psd_fri_iterator_information(iterator, information);
		printf(" %s: %s\n", name, information);
		psd_fri_iterator_next(client, iterator);
	}
	psd_end_fri_iteration(client, iterator);

	printf("\n member: ");
	scan_input_string(member, MAX_USER_NAME_CHARS);
	printf("\n description: ");
	scan_input_string(description, MAX_DESCRIPTION_CHARS);

	if( (chat_id = psd_create_chat(client, description, NULL)) < 0 ) {
		printf(" FAIL: Could not create the chat\n");
		wait_user();
		return -1;
	}

	if( psd_add_member_to_chat(client, member, chat_id) != 0 ) {
		psd_quit_from_chat(client, chat_id);
		printf(" Failed to create the chat\n");
		wait_user();
		return -1;
	}

	printf(" Chat with %s created\n", member);
	return 0;
}


int send_friend_request(psd_ims_client *client) {
	char name[MAX_USER_NAME_CHARS];

	printf("\n\n User name: ");
	scan_input_string(name, MAX_USER_NAME_CHARS);

	if( psd_send_friend_request(client, name) != 0 ) {
		printf(" FAIL: Could not request friendship to '%s'\n", name);
		wait_user();
		return -1;
	}

	printf(" Sended a friend request to '%s'\n", name);
	wait_user();

	return 0;
}


void screen_main_show(psd_ims_client *client, void *data) {
	chats_iterator *iterator;
	char *description;
	int id, unread; 

	menu_header_show("PSD IMS - Chats");
	printf(" (/e)exit, (/l)logout, (/n)new chat (/f)send friend request\n");
	printf(" (/r)review friend requests\n");
	printf(" --------------------------------------------\n");

	psd_begin_chat_iteration(client, iterator);
	while(psd_chat_iterator_valid(iterator)) {
		psd_chat_iterator_description(iterator, description);
		psd_chat_iterator_id(iterator, id);
		psd_chat_iterator_unread(iterator, unread);
		printf(" (%d) %d: \n", unread, id);
		psd_chat_iterator_next(client, iterator);
	}
	psd_end_chat_iteration(client, iterator);

	menu_footer_show();
}


void menu_main(client_graphic *graphic, boolean *global_exit) {
	boolean exit = FALSE;
	boolean cont = TRUE;
	int ret;
	int chat_id;

	set_screen_update(graphic, screen_main_show, NULL);

	do {
		set_screen_updateable(graphic, TRUE);
		screen_update(graphic);
		ret = get_user_input(graphic->input_buffer, MAX_INPUT_CHARS);
		set_screen_updateable(graphic, FALSE);
		if (ret < 0) return;
		if (ret < 1) continue;
		if (graphic->input_buffer[0] == COMMAND_CHAR) {
			if (ret < 2) continue;
			switch(graphic->input_buffer[1]) {
				case 'e':
					exit = true;
					*global_exit = true;
					break;
				case 'l':
					exit = true;
					*global_exit = false;
					break;
				case 'n':
					create_new_chat(graphic->client);
					break;
				case 'f':
					send_friend_request(graphic->client);
					break;
				case 'r':
					menu_friend_req(graphic);
					set_screen_update(graphic, screen_main_show, NULL);
					break;
			}
		} 
		else if (is_numeric(graphic->input_buffer)) {
			chat_id = atoi(graphic->input_buffer);
			if(psd_chat_exists(graphic->client, chat_id)) {
				menu_chat(graphic, chat_id);
				set_screen_update(graphic, screen_main_show, NULL);
			}
		}
		
		get_continue_graphic(graphic, cont);
		cont = cont && !exit;
		
	} while( cont );
	
	psd_logout(graphic->client);
}



/* =========================================================================
 *  Login Menu
 * =========================================================================*/

int user_register(psd_ims_client *client) {
	char name[MAX_USER_NAME_CHARS];
	char pass[MAX_USER_PASS_CHARS];
	char info[MAX_USER_INFO_CHARS];
	int ret;

	printf("\n User name: ");
	ret = scan_input_string(name, MAX_USER_NAME_CHARS);
	if (ret < 0) return -1;
	printf(" User password: ");
	ret = scan_input_string(pass, MAX_USER_PASS_CHARS);
	if (ret < 0) return -1;
	printf(" Describe yourself: ");
	ret = scan_input_string(info, MAX_USER_INFO_CHARS);
	if (ret < 0) return -1;

	if ( psd_user_register(client, name, pass, info) != 0 ) {
		printf(" Register failed, maybe the name is already registered or the conection is failing\n");
		wait_user();
		return -1;
	}

	printf(" User '%s' correctly registered\n", name);
	wait_user();

	return 0;
}


int login(psd_ims_client *client) {
	char name[MAX_USER_NAME_CHARS];
	char pass[MAX_USER_PASS_CHARS];
	int ret;

	printf("\n User name: ");
	ret = scan_input_string(name, MAX_USER_NAME_CHARS);
	if (ret < 0) return -1;
	printf(" User password: ");
	ret = scan_input_string(pass, MAX_USER_PASS_CHARS);
	if (ret < 0) return -1;


	printf(" %s %s\n", name, pass);
	if ( psd_login(client, name, pass) != 0 ) {
		printf(" Login failed, maybe the credentials are incorrect or the conection is failing\n");
		wait_user();
		return -1;
	}

	printf(" Logged in as '%s'\n", name);
	printf(" Retrieving user data ...");	
	retrieve_user_data(client);

	printf(" Done\n");
	wait_user();

	return 0;
}


void screen_login_show(psd_ims_client *client, void *data) {
	menu_header_show("PSD IMS - Login");
	printf(" 1. Alta\n");
	printf(" 2. Login\n");
	printf("\n 0. Salir\n");
	menu_footer_show();
}


void menu_login(client_graphic *graphic) {
	boolean exit = FALSE;
	boolean cont = TRUE;

	set_screen_update(graphic, screen_login_show, NULL);

	do {
		set_screen_updateable(graphic, TRUE);
		screen_update(graphic);
		get_user_input(graphic->input_buffer, MAX_INPUT_CHARS);
		set_screen_updateable(graphic, FALSE);
		switch(graphic->input_buffer[0]) {
			case '0': 
				exit = TRUE;
				break;   // salir
			case '1':	
				user_register(graphic->client);
				break;
			case '2': 
				if( login(graphic->client) == 0) {
					configure_signal_handling();
					//run_notifications_thread(client);
					menu_main(graphic, &exit);
					set_screen_update(graphic, screen_login_show, NULL);
				}
				break;
		}
		
		get_continue_graphic(graphic, cont);
		cont = cont && !exit;
			
	} while( cont );
}


/* =========================================================================
 *  Main function
 * =========================================================================*/


int graphic_client_run(psd_ims_client *client) {
	client_graphic graphic;

	
	graphic.menu_update_screen = NULL;
	graphic.screen_data = NULL;
	graphic.notifications_tid = -1;
	graphic.client = client;
	
	graphic.continue_graphic = TRUE;
	graphic.continue_fetching = FALSE;
	graphic.screen_updateable = FALSE;
	
	pthread_mutex_init(&graphic.continue_fetching_mutex, NULL);
	pthread_mutex_init(&graphic.continue_graphic_mutex, NULL);
	pthread_mutex_init(&graphic.screen_update_mutex, NULL);

	graphic_global = &graphic;

	configure_signal_handling();
	menu_login(&graphic);
	
	DEBUG_INFO_PRINTF("Freeing client resources");
	
}


void retrieve_user_data(psd_ims_client *client) {
	if ( psd_recv_friends(client) < 0 ) {
		printf(" Failed to retrieve new friends");
		wait_user();
		return;
	}

	if ( psd_recv_chats(client) < 0 ) {
		printf(" Failed to retrieve new chats");
		wait_user();
		return;
	}

	if ( psd_recv_all_messages(client) < 0 ) {
		printf(" Failed to retrieve the chats messages");
		wait_user();
		return;
	}
}


int configure_signal_handling() {
	if (signal(SIGINT, stop_client) == SIG_ERR) {
		DEBUG_FAILURE_PRINTF("Could not attach SIGINT handler");
		return -1;
	}
	if (signal(SIGTERM, stop_client) == SIG_ERR) {
		DEBUG_FAILURE_PRINTF("Could not attach SIGTERM handler");
		return -1;
	}
	if (signal(SIGABRT, stop_client) == SIG_ERR) {
		DEBUG_FAILURE_PRINTF("Could not attach SIGABRT handler");
		return -1;
	}
}

int run_notifications_thread(client_graphic *graphic) {
	// start the thread to get notifications
	if (pthread_create(&graphic->notifications_tid, NULL, &notifications_fetch, graphic) != 0 ) {
		printf("Could not create the notifications thread\n");
		return 0;
	}
}

void *notifications_fetch(void *arg) {
	client_graphic *graphic;
	sigset_t sig_blocked_mask;
	sigset_t old_sig_mask;

	graphic = (client_graphic*)arg;

	graphic->continue_fetching = TRUE;

	while(1) {
		sleep(1);
		psd_recv_notifications(graphic->client);
		screen_update(graphic);

		pthread_mutex_lock(&(graphic->continue_fetching_mutex));
		if( !graphic->continue_fetching ) {
			pthread_mutex_unlock(&(graphic->continue_fetching_mutex));
			DEBUG_INFO_PRINTF("Ending notifications service...");
			break;
		}
		pthread_mutex_unlock(&(graphic->continue_fetching_mutex));
	}
}


void stop_client(int sig) {

	set_continue_fetching(graphic_global, FALSE);

	// wait for the thread to end
	if( graphic_global->notifications_tid != -1) {
		pthread_join(graphic_global->notifications_tid, NULL);
	}

	// Now the notification service is not running
	
	// Save the state
	// To save the state, I should be sure that the system is stable
	// but the process could be accesing a structure when the signal was thrown...

	set_continue_graphic(graphic_global, FALSE);
	fclose(stdin);

}