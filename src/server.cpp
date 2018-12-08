#include <arpa/inet.h>
#include <ctype.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
using namespace std;






// Intended Space - Don't erase empty lines







/**********************
  Structs
***********************/
#define MAX_USER_NAME 30
#define MAX_ROOM_NAME 30
#define MAX_MSG_CHAR 100

#define MAX_ROOM_NUM 20
#define MAX_USER_IN_A_ROOM 20
#define MAX_CLIENTS 100

#define DEFAULT_USR_NAME "ChangeName"

typedef struct User{
  char user_name[MAX_USER_NAME];
  int socket;
  int room_id;
} User;
typedef struct Room{
  char room_name[MAX_ROOM_NAME];
  int room_id;
  int num_users;
  int socket_list_in_Room[MAX_USER_IN_A_ROOM];
} Room;
typedef struct Message{
  char message[MAX_MSG_CHAR];
  int socket;
} Message;





// Intended Space - Don't erase empty lines






/*******************
  GLOBAL VARIABLES 
********************/
/* Max text line length */
#define MAXLINE 8192

/* Second argument to listen() */
#define LISTENQ 1024

Room Room_list[MAX_ROOM_NUM];
User User_list[MAX_CLIENTS];

int num_room_list = 0;

/* Simplifies calls to bind(), connect(), and accept() */
typedef struct sockaddr SA;

// A lock for the message buffer.
pthread_mutex_t lock;
pthread_mutex_t room;

// We will use this as a simple circular buffer of incoming messages.
char message_buf[20][50];

// This is an index into the message buffer.
int msgi = 0;

void init_Rooms_Users_Messages(){ 
  User initUser;
  initUser.user_name[0] = '\0';
  initUser.socket = -1;
  initUser.room_id = -1;
  for(int i=0; i<MAX_CLIENTS; i++){
    User_list[i] = initUser;
  }
  Room initRoom;
  initRoom.room_name[0] = '\0';
  initRoom.room_id = -1;
  initRoom.num_users = 0;
  initRoom.socket_list_in_Room[0] = -1;
  for(int i=0; i<MAX_ROOM_NUM; i++){
    Room_list[i] = initRoom;
  }
}







// Intended Space - Don't erase empty lines








/********************************
  SOCKETS
********************************/
int volatile socket_list[MAX_CLIENTS];  // global variable for storing sockets
void initialize_sockets(){  // initialize all the socket by -1 to indicate it is empty
  for(int i=0; i<MAX_CLIENTS; i++){
    socket_list[i] = -1;
  }
}
void print_sockets(){ // print current connected sockets
  printf("--- socket list ---\n");
  for(int i=0; i<MAX_CLIENTS; i++){
    if(socket_list[i] != -1)
      printf("socket[%d]:%d\n", i, socket_list[i]);
  }
  printf("-------------------\n");
}
void add_sockets(int connfd){ // add new socket into current socket list. Sockets are filled from left to right
  for(int i=0; i<MAX_CLIENTS; i++){
    if( socket_list[i] != -1) { continue; }
    else {
      socket_list[i] = connfd;
      break;
    }
  }
}
void delete_socket(int connfd){ // find a matching socket and delete it.
  for(int i=0; i<MAX_CLIENTS; i++){
    if( socket_list[i] == connfd) {
      socket_list[i] = -1;
      break;
    }
  }
}






// Intended Space - Don't erase empty lines







/********************************
  ROOM
*********************************/
int get_number_of_room_list(){ //room list
  return num_room_list;
}
int increare_number_of_roomlist(){
  num_room_list++;  
}
int get_User_list_index_by_socket(int connfd){  
  /** find an User index from User_list[] by comparing corresponding socket(client) **/
  for(int i=0; i<MAX_CLIENTS; i++){
    if(User_list[i].socket == connfd){  // found the corresponding socket(client)
      return i;   // return index number of corresponding socket(client)
    }
  }
  return -1;  // that socket(client) is not existing
}
int add_new_User_in_User_list(User user){  // add an User into User_list[]
  int i = 0;
  for(i=0; i<MAX_CLIENTS; i++){
    if(User_list[i].socket!=-1){  // if this User slot is not empty
      continue; // check next
    } else { // this slot is empty
      User_list[i] = user;  // add the user into User_list[]
      return i; // successfully added user into User_list and return index
    }
  }
  return -1; // couldn't find empty slot, or there was an error
}
int add_User_in_existing_Room(int connfd, char *nickname, int room_id){
  /***
    purpose:  adding a User into existing Room
    assumption: room_id of the existing Room is found,
                we do now know if there is an User with this socket(client), not by nickname(bcoz nickname will change at sometime)
  ***/
  int n = Room_list[room_id].num_users; // getting num of users in the corresponding Room
  if (n >= MAX_USER_IN_A_ROOM) {  // if number of maximum users exceeded,
    return -1;  // there is already MAX users in the room, return error(-1)
  } // there is an empty spot in the Room
  Room_list[room_id].socket_list_in_Room[n] = connfd; // add the socket(client) into the user list of the Room
  Room_list[room_id].num_users++; // increase the number of users in the Room
  int idx = get_User_list_index_by_socket(connfd);  // check if there is an existing User in User_list[] by checking returning value of socket number
  if (idx != -1) {  // there is an existing User in User_list[] of corresponding socket(client)
    bzero(User_list[idx].user_name, MAX_USER_NAME); // initialize
    strcmp(User_list[idx].user_name, nickname); // update User's name by nickname
  } else {    // there is no such socket(client) in User_list[], so create a new User with this socket(client)
    User newUser; // create a new User
    bzero(newUser.user_name, MAX_USER_NAME);  // initialize name
    strcmp(newUser.user_name, nickname);  // update name by nickname
    newUser.socket = connfd;  // match this User with the connected socket(client)
    newUser.room_id = room_id;  // mark the room_id which this User belongs to currently
    add_new_User_in_User_list(newUser); // add this User of corresponding socket(client) into User_list[]
  }
  return 1;
}
int get_room_userlist(char *room_name){//printout list of user in this room, can just printout nicknames.
  //since room id isnot needed.
  return 0;
}
// int create_room_backup(char *room_name, char *user_name){// create room if room is not existed, and add the user.
//   //if room is existed and not full , we add the user.
//   pthread_mutex_lock(&room);
//   printf("\t[+]creating a room \"%s\".\n", room_name);
//   int x = get_number_of_room_list();
  
//   for (int i=0; i <=x; i++){
//     if((*(room_list[i])).room_name==room_name){
//       //join the room
//       // User *user_list = (*(room_list[i])).user_list;

//     }
//     if(i==x)
//     {
//       if (x>=MAX_ROOM_NUM) return -1;
//       strcpy((*(room_list[x])).room_name, room_name);
//       (*(room_list[x])).room_id = x;
//       //update user_list of the room
//       num_room_list++;
//       printf("\tsize of room_list: %d\n", get_number_of_room_list());
//     }
//   }
 
  
//   pthread_mutex_unlock(&room);
//   return num_room_list;
// }
int create_room(int connfd, char *nickname, char *room_name){
  /***
      purpose: create a new Room
      assumption: there is no existing Room in Room_list[],
                  index of Room_list[] and room_id is matched always, so first Room's room_id is 0 at Room_list[0], and so on.
  ***/
  pthread_mutex_lock(&room);
  printf("\t[+]creating a room \"%s\".\n", room_name);
  int x = get_number_of_room_list();

  Room newRoom; // create a temporary Room object
  strcpy(newRoom.room_name, room_name); // update its name with room_name
  newRoom.room_id = x;  // update its room_id with
  newRoom.num_users = 1;  // this Room is created, it means there is at least one User inside
  newRoom.socket_list_in_Room[0] = connfd;  // add socket(client) into socket_list_in_Room at 0 because it is just created.
  Room_list[x] = newRoom; // put this Room into Room_list[], index number and room_id is always matched
  increare_number_of_roomlist();  // increate number_of_roomlist
  pthread_mutex_unlock(&room);
  return num_room_list;
}

int is_room_name_existing(char *room_name){
  for(int i=0; i<get_number_of_room_list(); i++){
    if (strcmp(Room_list[i].room_name, room_name) == 0 ){ // there is an existing room with the same name
      printf("Existing a room with the name %s\n", room_name);
      return Room_list[i].room_id; // return room_id
    }
  }
  printf("No such name of room existing %s\n", room_name);
  return -1;  // there is no room with the same name
}



int leave_room(char *nickname,int room_id){
  int tempSocket=-1;
  for(int i=0;i<MAX_CLIENTS;i++){
    char *tempName=User_list[i].user_name;
    if(strcmp(nickname,tempName)==0)
    {
      tempSocket=User_list[i].socket;
    }
  }

  for(int i=0;i<MAX_ROOM_NUM;i++){
    if(tempSocket==Room_list[room_id].socket_list_in_Room[i])
    {
      Room_list[room_id].socket_list_in_Room[i]=-1;
      return 1;
    }
    
  }
  return -1;
}

int check_user_in_which_room(char *nickname){
  
  for(int i=0;i<MAX_CLIENTS;i++){
    char *tempName=User_list[i].user_name;
    if(strcmp(nickname,tempName)==0)
    {
      return User_list[i].room_id;
    }
  }
 // we can simply this function, since we do not need pointer.
  // for(int i=0; i<(*(room_list[room_id])).num_users; i++){
  //   if ( (*(*(room_list[room_id])).user_list[i]).user_id == user_id) {
  //     return true;
  //   }
  // }
  return -1;
}

const char * check_username_by_socket(int connfd){
  printf("connfd is %d",connfd);
  for(int i=0;i<MAX_CLIENTS;i++){
    printf("searching %d" ,i);
    int tempSocket=User_list[i].socket;
    printf("socketnumber is %d",tempSocket);
    if(connfd == tempSocket)
    {
      printf("FIND name %s" ,User_list[i].user_name);
      return User_list[i].user_name;
    }
  }
  return 0;
}

int JOIN_Nickname_Room(int connfd, char *nickname, char *room_name){// create room if room is not existed, and add the user.
  /*** 
    purpose: process the command of "\JOIN nichname roomname"
  // if there is existing room with room_name
  //    add an user in the existing room() - we do not know if there is such an User in User_list[] yet.
  // else (no such room with the name)
  //    if number of room >= MAX
  //        return error
  //    create_room() - create a new Room
  // return success
  ***/
  int room_id = check_user_in_which_room(nickname);
  int temp_leave_room = leave_room(nickname,room_id);
  if(temp_leave_room == 1){

  if(int r_id = is_room_name_existing(room_name) > -1){  // if there is existing room with same name
    add_User_in_existing_Room(connfd, nickname, r_id);  // add 
  } else {  // nope? then we need to create a new room
    if (get_number_of_room_list() >= MAX_ROOM_NUM) { return -1; }
       // if number of room is full //leave the old room.

    create_room(connfd, nickname, room_name);
  }
  }
  return 1;
}




// Intended Space - Don't erase empty lines









/****************************************
  INCOMING MESSAGES FROM CLIENTS
*****************************************/
// Initialize the message buffer to empty strings.
void init_message_buf() {
  int i;
  for (i = 0; i < 20; i++) {
    strcpy(message_buf[i], "");
  }
}

// A wrapper around recv to simplify calls.
int receive_message(int connfd, char *message) {
  return recv(connfd, message, MAXLINE, 0);
}

// A wrapper around send to simplify calls.
int send_message(int connfd, char *message) {
  return send(connfd, message, strlen(message), 0);
}

// This function adds a message that was received to the message buffer.
// Notice the lock around the message buffer.
void add_message(char *buf) {
  pthread_mutex_lock(&lock);
  strncpy(message_buf[msgi % 20], buf, 50);
  int len = strlen(message_buf[msgi % 20]);
  message_buf[msgi % 20][len] = '\0';
  msgi++;
  pthread_mutex_unlock(&lock);
}

// Destructively modify string to be upper case
void upper_case(char *s) {
  while (*s) {
    *s = toupper(*s);
    s++;
  }
}

// A predicate function to test incoming message.
int is_Command_message(char *message) { 
  if(message[0]=='\\'){
    //printf("%s\n","it is Command" );
    return true;
  }
  else{
    return false;
  }
}

int send_ROOM_message(int connfd) {
  char message[1024] = "ROOM";
  printf("Sending: %s \n", message);

  return send_message(connfd, message);
}
int send_JOIN_message(int connfd){
  
}
/* Command: \ROOMS */
int send_roomlist_message(int connfd){
  char *prefix = (char *)"Room [";
  char *suffix = (char *)"]: ";
  char list_buffer[(MAX_ROOM_NUM + 2) * (sizeof(prefix) + MAX_ROOM_NAME + 1)];
  bzero(list_buffer, sizeof(list_buffer));  // initialize list_buffer
  
  char *preprefix = (char *)"\nNumber of Rooms: ";
  char temp[10]; // buffer to save int values
  sprintf(temp, "%d", get_number_of_room_list());
  //temp[strlen(temp)] = '\n';
  //temp[9] = '\0';

  strcat(list_buffer, preprefix);
  strcat(list_buffer, temp);
  
  for(int i=0;i<num_room_list;i++){
    strcat(list_buffer, "\n");
    strcat(list_buffer, prefix);  // add prefix
    int temp_int = Room_list[i].room_id;  // get room_id
    sprintf(temp, "%d", temp_int);  // convert room_id into chars
    strcat(list_buffer, temp); // add room_id chars
    strcat(list_buffer, suffix);  // add suffix
    strcat(list_buffer, Room_list[i].room_name); // add room_name
    
  }
  printf("Sneding: %s\n", list_buffer);
  return send_message(connfd, list_buffer);
}


/* Command: \WHO */
int send_userlist_message(int connfd) { 
  char message[20 * 50] = "";
  int tempRoom_ID = -1;
  for(int i=0;i<MAX_CLIENTS;i++){
    int tempSocket = User_list[i].socket;
    if(tempSocket==connfd)
    {
      tempRoom_ID=User_list[i].room_id;
    }
  }
  //we find room_id

  for(int i=0;i<MAX_ROOM_NUM;i++){
    int current_socket_in_room=Room_list[tempRoom_ID].socket_list_in_Room[i];
    char *nickname[MAX_USER_NAME];
    if(current_socket_in_room >-1)
    {
      for(int i=0; i<MAX_CLIENTS;i++){
        if(User_list[i].socket==current_socket_in_room)
        {
          strcat(message, User_list[i].user_name);
          strcat(message,",");
        }
      }

    }
    
    
  }
  
  
  

  // End the message with a newline and empty. This will ensure that the
  // bytes are sent out on the wire. Otherwise it will wait for further
  // output bytes.
  strcat(message, "\n\0");
  printf("Sending: %s", message);

  return send_message(connfd, message);
}

int send_helplist_message(int connfd) { // this part do not need help list.

  char message[20 * 100] = "";
  const char *temp_user_list[6];
    temp_user_list[0] = "\\JOIN nickname room";
    temp_user_list[1] = "\\ROOMS";
    temp_user_list[2] = "\\LEAVE";
    temp_user_list[3] = "\\WHO";
    temp_user_list[4] = "\\nickname message";
    temp_user_list[5] = "\\HELP";
  for (int i = 0; i < 6; i++) {
    if (strcmp(temp_user_list[i], "") == 0) break;//room list
    strcat(message, temp_user_list[i]);//room list
    strcat(message, "\n");
  }

  // End the message with a newline and empty. This will ensure that the
  // bytes are sent out on the wire. Otherwise it will wait for further
  // output bytes.
  strcat(message, "\n\0");
  printf("Sending: %s", message);

  return send_message(connfd, message);
}

char *array[3];// for string to token
void string_to_token(char buf[]){
  int i = 0;
    char *p = strtok (buf, " ");
    
    while (p != NULL)
    {
        array[i++] = p;
        p = strtok (NULL, " ");
    }
}

int process_message(int connfd, char *message) {//idk if we can use case switch
  
  if (is_Command_message(message)) {
    printf("Received a Command: ");
    // Bug: if the command is not made of three words, e.g., just "\JOIN", then this will crash.
    //      we should deal with those edge cases.
    if(strncmp(message, "\\JOIN",5) == 0){
      printf("%s\n","it is \\JOIN nickname room" );
      string_to_token(message);//convert to tokens
      if(!array[1]||!array[2]){
        printf("%s \n","make sure you have 3 arguments");
        return send_message(connfd, (char *)"command not recognized,make sure you have 3 arguments");
      }
      //array[1] is nickname
      //array[2] is room name
      printf("\n the room name is %s", array[2]);
      printf("nickname:%s\n", array[1]);
      // we can safely call JOIN_Nickname_Room() without any considerations here.
      // the function will deal with any cases.
      if(JOIN_Nickname_Room(connfd, (char *)array[1],(char *)array[2]) == -1){
        return send_message(connfd, (char *)"Error creating a room");
      }

      return send_message(connfd, (char *)"Created a room");
    }
    else if(strcmp(message, "\\ROOMS") == 0){//this part is fine
            printf("%s\n","it is \\ROOMS" );
            // printf("%s\n", get_room_list());
            return send_roomlist_message(connfd);
    }
    else if(strcmp(message, "\\LEAVE") == 0){//this part is fine
      char message[1024] = "GoodBye";
      return send_message(connfd, message);

    }
    else if(strcmp(message, "\\WHERE") == 0){//this part is fine
      printf("%s\n","it is \\where" );
     // message = (char *)("%s "," Your location room is ");
      char* nickname = (char*)check_username_by_socket(connfd);
      printf("the nickname is %s",nickname);
      int r_id = check_user_in_which_room(nickname);
      printf("the room_id is %d",r_id);
      strcat(message,(char *) r_id);
      return send_message(connfd, message);

    }
    else if(strcmp(message, "\\WHO") == 0){//this part is fine
          printf("%s\n","it is \\WHO" );
          return send_userlist_message(connfd);
    }
    else if(strcmp(message, "\\HELP") == 0){//this part is fine
          printf("%s\n","it is \\HELP" );
          return send_helplist_message(connfd);
    }
    else if(strcmp(message, "\\nickname message") == 0){//this part will be in a same room and whisper by nickname
      //if you can not find the nickname then show it user not existed. some thing like this. much easy.
    }
    else{
      char tempMessage[1024] =" command not recognized";//this part is fine
      strcat(message,tempMessage);
      printf("%s\n ", message);
      return send_message(connfd,message);
    }
    return send_ROOM_message(connfd);
  } 
  else {//this part is fine
    printf("Server responding with echo response.\n");
    return send_message(connfd, message);
  }
}

void simple_message(int connfd){
  size_t n;
  char message[MAXLINE];

  // Create a default User object
  User defaultUser;
  strncpy(defaultUser.user_name, DEFAULT_USR_NAME, sizeof(DEFAULT_USR_NAME));
  defaultUser.socket = connfd;  // match this User object and the connected socket(client)
  defaultUser.room_id = 0;

  JOIN_Nickname_Room(connfd, (char *)DEFAULT_USR_NAME, (char *)"Lobby");

  while((n=receive_message(connfd, message))>0) {
    printf("From socket[%d]: Server received a meesage of %d bytes: %s\n", connfd, (int)n, message);
    n = process_message(connfd, message);
    bzero(message, sizeof(message));  // reintialize the message[] buffer
  }
}






// Intended Space - Don't erase empty lines







/***************
  SERVER PART
****************/
// Helper function to establish an open listening socket on given port.
int open_listenfd(int port) {
  int listenfd;    // the listening file descriptor.
  int optval = 1;  //
  struct sockaddr_in serveraddr;

  /* Create a socket descriptor */
  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) return -1;
  printf("[+]Server Socket is created.\n");

  /* Eliminates "Address already in use" error from bind */
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval,
                 sizeof(int)) < 0)
    return -1;

  /* Listenfd will be an endpoint for all requests to port
     on any IP address for this host */
  bzero((char *)&serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)port);
  if (bind(listenfd, (SA *)&serveraddr, sizeof(serveraddr)) < 0) return -1;
  printf("[+]Server Socket is binded to the server address.\n");

  /* Make it a listening socket ready to accept connection requests */
  if (listen(listenfd, LISTENQ) < 0) return -1;
  printf("[+]Server Socket is ready to listening connection.......\n");
  return listenfd;
}

// thread function prototype as we have a forward reference in main.
void *thread(void *vargp);

int main(int argc, char *argv[]){// we need help function before we call the thread.
  //we need let user change name first. the default name can a random string by your choose,
  // but once user connected, we create a user for this client and ask a name from this client.
  // check if it is repeat. this will all do in a help function.
  
  // Check the program arguments and print usage if necessary.
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(0);
  }

  // initialize message buffer.
  init_message_buf();

  // Initialize the message buffer lock.
  pthread_mutex_init(&lock, NULL);
  pthread_mutex_init(&room, NULL);

  // The port number for this server.
  int port = atoi(argv[1]);

  // The listening file descriptor.
  int listenfd = open_listenfd(port);

  // Initialize ROOMS, USERS, MESSAGES
  init_Rooms_Users_Messages();

  initialize_sockets();
  print_sockets();
  while(1){
    // The connection file descriptor.
    int *connfdp = (int *)malloc(sizeof(int));

    // The client's IP address information.
    struct sockaddr_in clientaddr;

    // Wait for incoming connections.
    socklen_t clientlen = sizeof(struct sockaddr_in);
    *connfdp = accept(listenfd, (SA *)&clientaddr, &clientlen);

    /* determine the domain name and IP address of the client */
    struct hostent *hp =
        gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
                      sizeof(clientaddr.sin_addr.s_addr), AF_INET);

    // The server IP address information.
    char *haddrp = inet_ntoa(clientaddr.sin_addr);

    // The client's port number.
    unsigned short client_port = ntohs(clientaddr.sin_port);

    printf("[+]Server connected to %s (%s), port %u, socket[%d]\n", hp->h_name, haddrp,
           client_port, *connfdp);

    // Create a new thread to handle the connection.
    add_sockets(*connfdp);
    // printf("\t\t sockets[] array at:%p \t sockets[0]:%d \t *(sockets[0]):%d \n", sockets, sockets[0], (sockets[0]));
    // printf("\t\t\t\t\t\t connfdp: %p \t *connfdp: %d\n", connfdp, *connfdp);
    pthread_t tid;
    pthread_create(&tid, NULL, thread, connfdp);
  }

  return 0;
}

/* thread routine */
void *thread(void *vargp) {
  // Grab the connection file descriptor.
  int connfd = *((int *)vargp);
  // Detach the thread to self reap.
  pthread_detach(pthread_self());
  // Free the incoming argument - allocated in the main thread.
  free(vargp);
  // Handle the echo client requests.
  printf("[+]New Thread created with the socket [%d]\n", connfd);
  print_sockets();
  simple_message(connfd);
  printf("[-]Client with socket [%d] disconnected.\n", connfd);
  delete_socket(connfd);
  print_sockets();
  // Don't forget to close the connection!
  close(connfd);
  return NULL;
}
