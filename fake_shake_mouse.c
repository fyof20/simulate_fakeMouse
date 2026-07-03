
//  this project is simulating mouse shaking on Linux by libevdev


#include <stdio.h>
#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h> 
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>

static int running = 1;

void stop(int sig) { running = 0; }

int main(){

    struct libevdev *dev;
    struct libevdev_uinput *uidev = NULL;

   //init
    dev =libevdev_new();
    libevdev_set_name(dev, "USB fyf_fakeMouse");
    libevdev_enable_event_type(dev, EV_REL);
    libevdev_enable_event_code(dev, EV_REL, REL_X, NULL);
    libevdev_enable_event_code(dev, EV_REL, REL_Y, NULL);
    //simnulate left key
    libevdev_enable_event_type(dev, EV_KEY);
    libevdev_enable_event_code(dev, EV_KEY, BTN_LEFT, NULL);

    libevdev_enable_event_type(dev, EV_SYN);
    libevdev_enable_event_code(dev, EV_SYN, SYN_REPORT, NULL);

    //create a fake mouse input dev 
    int code = libevdev_uinput_create_from_device(dev, LIBEVDEV_UINPUT_OPEN_MANAGED, &uidev);

    if ( code != 0) {
        perror("ERROR!:");
        return -1;
    }


    printf("shaking mouse (Ctrl+C to stop)\n");
    srand(time(NULL));
    signal(SIGINT, stop);
    signal(SIGTERM, stop);
    while (running){
        int direction = rand() % 2 ==0 ? REL_X : REL_Y;
        int amplitude = rand() % 41 - 20;

        libevdev_uinput_write_event(uidev, EV_REL, direction, amplitude);
        libevdev_uinput_write_event(uidev, EV_SYN, SYN_REPORT, 0);
        //printf("amplitude= %d, direction= %d\n", amplitude, direction);


        usleep(50000);
    }
    libevdev_uinput_destroy(uidev);
    libevdev_free(dev);
    printf("stopped.\n");
    return 0;
}