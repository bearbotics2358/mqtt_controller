/* mqtt_controller - activate and kill other programs based on remote request

	 Protocol:
	 Send message to camera
	 Cameras:
	 - claw
	 - cargo
	 Messages:
	 - on - turn on streaming for remote viewing
	 - off - turn off streaming for remote viewing
	 - vision - turn on computer vision

	 Ideally, remember which mode the camera is in and if necessary kill the active process before starting a new process (e.g., kill viewing before activating CV)

	 created 2/28/2019 BD from mqtt_example

	 

	 BUILD: gcc -o mqtt_controller mqtt_controller.c -lmosquitto
*/

#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h> // random()
#include <errno.h>

#include <mosquitto.h>

// #define mqtt_host "localhost"
// #define mqtt_port 1883

// roborio
// #define mqtt_host "10.23.58.2"
#define mqtt_host "10.23.58.26"
#define mqtt_port 1183

static int run = 1;

// one enum to cover commands and active processes
// invalid only applies to commands, not processes
enum processes {
	invalid = -1,
	off = 0,
	view = 1,
	vision = 2,
} ;

struct process_status {
	enum processes current_process;
	int pid;
} ;

struct process_status claw_process;
struct process_status cargo_process;

// strings for printing
const char *INVALID = "INVALID";
const char *OFF = "off";
const char *VIEW = "view";
const char *VISION = "vision";

// which camera is this?
int claw_f = 0;
int cargo_f = 0;


enum processes cmd_check(char * command)
{
	enum processes ret = invalid;

	if(!strcmp(command, "off")) {
		ret = off;
	} else if(!strcmp(command, "view")) {
		ret = view;
	} else if(!strcmp(command, "vision")) {
		ret = vision;
	}

	return ret;
}

const char * str_cmd(enum processes proc)
{
	const char * ret;
	
	switch(proc) {
	case off:
		ret = OFF;
		break;

	case view:
		ret = VIEW;
		break;

	case vision:
		ret = VISION;
		break;

	default:
	case invalid:
		ret = INVALID;
		break;
	}
	return ret;
}

// this function starts computer vision by forking
// the parent returns from the fork call with the pid of the child
// the child returns with a zero and launches the task we wish to start
void claw_start_cv()
{
	pid_t ret;
	
	ret = fork();
	printf("fork returned %d\n", ret);
	switch(ret) {
	case -1:
		// error
		printf("unable to fork - %s\n", strerror(errno));
		exit(-1);

	case 0:
		// child
		// exec new process
		//execl("useless.sh", "useless.sh", "vision", (char *) NULL);
		

		execl("vision_startup.sh","vision_startup.sh", (char *) NULL);

		// never reach this point
		
	default:
		// parent
		claw_process.pid = ret;
		break;
	}
}

// this function starts computer vision by forking
// the parent returns from the fork call with the pid of the child
// the child returns with a zero and launches the task we wish to start
void claw_start_view()
{
	pid_t ret;
	
	ret = fork();
	printf("fork returned %d\n", ret);
	switch(ret) {
	case -1:
		// error
		printf("unable to fork - %s\n", strerror(errno));
		exit(-1);

	case 0:
		// child
		// exec new process
		// execl("useless.sh", "useless.sh", "view", (char *) NULL);


		execl("/usr/bin/uv4l", "uv4l", "--auto-video_nr", "--driver", "raspicam",
		 "--encoding", "h264", "--width", "104", "--height", "96",
		 "--server-option", "--port=1187", "--framerate",
		 "25", "--enable-server", "on", "--rotation", "180", (char *) NULL);


		/*
		// this works
		execl("/usr/bin/uv4l", "uv4l", "--auto-video_nr", "--driver", "raspicam",
		 "--encoding", "h264", 
		 "--enable-server", "on", (char *) NULL);
		*/


		// never reach this point
		
	default:
		// parent
		claw_process.pid = ret;
		break;
	}
}
// this function starts computer vision by forking
// the parent returns from the fork call with the pid of the child
// the child returns with a zero and launches the task we wish to start
void claw_kill_view()
{
	pid_t ret;
	
	ret = fork();
	printf("fork returned %d\n", ret);
	switch(ret) {
	case -1:
		// error
		printf("unable to fork - %s\n", strerror(errno));
		exit(-1);

	case 0:
		// child
		// exec new process
		execl("/usr/bin/pkill", "pkill", "uv4l", (char *) NULL);


		// never reach this point
		
	default:
		// parent
		claw_process.pid = ret;
		break;
	}
}

void claw_process_change(char *command)
{
	enum processes cmd;
	int ret;
	
	cmd = cmd_check(command);
	if(cmd == invalid) {
		return;
	}

	// are we already running this command?
	if(claw_process.current_process == cmd) {
		// nothing to do
		printf("Claw already in %s\n", str_cmd(cmd));
		return;
	}

	// are we running something else that must be shutdown?
	if(claw_process.current_process != off) {
		printf("Kill claw pid %d\n", claw_process.pid);
		if(claw_process.pid != 0) {
			if(claw_process.current_process == view) {
				claw_kill_view();
			} else {
				ret = kill(claw_process.pid, SIGTERM);
				printf("Kill returned %d\n", ret);
			}
		}
		claw_process.current_process = off;
	}
	if(cmd == off) {
		// done
		return;
	}
	
	// start new process
	if(cmd == vision) {
		claw_start_cv();
	} else if(cmd == view) {
		claw_start_view();
	}
	claw_process.current_process = cmd;
	printf("Starting new claw process pid %d\n", claw_process.pid);
	
}


// this function starts computer vision by forking
// the parent returns from the fork call with the pid of the child
// the child returns with a zero and launches the task we wish to start
void cargo_start_cv()
{
	pid_t ret;
	
	ret = fork();
	printf("fork returned %d\n", ret);
	switch(ret) {
	case -1:
		// error
		printf("unable to fork - %s\n", strerror(errno));
		exit(-1);

	case 0:
		// child
		// exec new process
		execl("useless.sh", "useless.sh", "vision", (char *) NULL);
		// never reach this point
		
	default:
		// parent
		cargo_process.pid = ret;
		break;
	}
}


// this function starts remote viewing by forking
// the parent returns from the fork call with the pid of the child
// the child returns with a zero and launches the task we wish to start
void cargo_start_view()
{
	pid_t ret;
	
	ret = fork();
	printf("fork returned %d\n", ret);
	switch(ret) {
	case -1:
		// error
		printf("unable to fork - %s\n", strerror(errno));
		exit(-1);

	case 0:
		// child
		// exec new process
		// execl("useless.sh", "useless.sh", "view", (char *) NULL);


		execl("/usr/bin/uv4l", "uv4l", "--auto-video_nr", "--driver", "raspicam",
		 "--encoding", "h264", "--width", "104", "--height", "96",
		 "--server-option", "--port=1187", "--framerate",
		 "25", "--enable-server", "on", "--rotation", "180", (char *) NULL);


		/*
		// this works
		execl("/usr/bin/uv4l", "uv4l", "--auto-video_nr", "--driver", "raspicam",
		 "--encoding", "h264", 
		 "--enable-server", "on", (char *) NULL);
		*/


		// never reach this point
		
	default:
		// parent
		cargo_process.pid = ret;
		break;
	}
}
// this function kills remote viewing by forking
// the parent returns from the fork call with the pid of the child
// the child returns with a zero and launches the task we wish to start
void cargo_kill_view()
{
	pid_t ret;
	
	ret = fork();
	printf("fork returned %d\n", ret);
	switch(ret) {
	case -1:
		// error
		printf("unable to fork - %s\n", strerror(errno));
		exit(-1);

	case 0:
		// child
		// exec new process
		execl("/usr/bin/pkill", "pkill", "uv4l", (char *) NULL);


		// never reach this point
		
	default:
		// parent
		cargo_process.pid = ret;
		break;
	}
}


void cargo_process_change(char *command)
{
	enum processes cmd;
	int ret;
	
	cmd = cmd_check(command);
	if(cmd == invalid) {
		return;
	}

	// are we already running this command?
	if(cargo_process.current_process == cmd) {
		// nothing to do
		printf("cargo already in %s\n", str_cmd(cmd));
		return;
	}

	// are we running something else that must be shutdown?
	if(cargo_process.current_process != off) {
		printf("Kill cargo pid %d\n", cargo_process.pid);
		if(cargo_process.pid != 0) {
			if(cargo_process.current_process == view) {
				cargo_kill_view();
			} else {
				ret = kill(cargo_process.pid, SIGTERM);
				printf("Kill returned %d\n", ret);
			}
		}
		cargo_process.current_process = off;
	}
	if(cmd == off) {
		// done
		return;
	}
	
	// start new process
	if(cmd == vision) {
		cargo_start_cv();
	} else if(cmd == view) {
		cargo_start_view();
	}
	cargo_process.current_process = cmd;
	printf("Starting new cargo process pid %d\n", cargo_process.pid);
	
}




void handle_signal(int s)
{
	run = 0;
}

void connect_callback(struct mosquitto *mosq, void *obj, int result)
{
	printf("connect callback, rc=%d\n", result);
}

void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
	bool match = 0;

	printf("got message '%.*s' for topic '%s'\n", message->payloadlen, (char*) message->payload, message->topic);

	mosquitto_topic_matches_sub("/camera/controls/claw/+", message->topic, &match);
	if(claw_f && match) {
		printf("got message for claw camera controls topic\n");
		claw_process_change(message->payload);
	}

	mosquitto_topic_matches_sub("/camera/controls/cargo/+", message->topic, &match);
	if(cargo_f && match) {
		printf("got message for cargo camera controls topic\n");
		cargo_process_change(message->payload);
	}

}

void usage()
{
	printf("usage: mqtt_controller <camera choice>\n");
	printf("where <camera choice> is either:\n");
	printf("	claw\n");
	printf("	cargo\n");
}

int main(int argc, char *argv[])
{
	uint8_t reconnect = true;
	char clientid[24];
	struct mosquitto *mosq;
	int rc = 0;


	if(argc != 2) {
		usage();
		exit(-1);
	}

	if(strcmp(argv[1], "claw") == 0) {
		claw_f = 1;
		cargo_f = 0;
	} else if(strcmp(argv[1], "cargo") == 0) {
		claw_f = 0;
		cargo_f = 1;
	} else {
		usage();
		exit(-1);
	}

	signal(SIGINT, handle_signal);
	signal(SIGTERM, handle_signal);

	// initialize processes
	claw_process.current_process = off;
	claw_process.pid = -1;
	cargo_process.current_process = off;
	cargo_process.pid = -1;

	mosquitto_lib_init();

	memset(clientid, 0, 24);
	snprintf(clientid, 23, "mysql_log_%d", getpid());
	mosq = mosquitto_new(clientid, true, 0);

	if(mosq){
		mosquitto_connect_callback_set(mosq, connect_callback);
		mosquitto_message_callback_set(mosq, message_callback);

	    rc = mosquitto_connect(mosq, mqtt_host, mqtt_port, 60);

		mosquitto_subscribe(mosq, NULL, "/camera/controls/+/+", 0);

		while(run){
			// BD changed timeout to 0 from -1 (1000ms)
			rc = mosquitto_loop(mosq, 0, 1);
			if(run && rc){
				printf("connection error!\n");
				sleep(10);
				mosquitto_reconnect(mosq);
			}
			sleep(1);
		}
		mosquitto_destroy(mosq);
	}

	mosquitto_lib_cleanup();

	return rc;
}

