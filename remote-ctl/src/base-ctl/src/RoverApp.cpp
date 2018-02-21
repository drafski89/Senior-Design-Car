#include "RoverApp.h"
#include <exception>
#include <stdexcept>
#include "ros/ros.h"
#include "std_msgs/String.h"

using namespace std;

const char* err_mesgs[] = {
    "No route to requested IP address",
    "Connection refused",
    "Connection timed out",
    "Connect was reset",
    "Unhandled connection error",
};

const char* connect_err_mesg(int code)
{
    const char* err_mesg = NULL;

    switch (code)
    {
    case EADDRNOTAVAIL:
        err_mesg = err_mesgs[0];
        break;

    case ECONNREFUSED:
        err_mesg = err_mesgs[1];
        break;

    case ENETUNREACH:
        err_mesg = err_mesgs[0];
        break;

    case ETIMEDOUT:
        err_mesg = err_mesgs[2];
        break;

    case ECONNRESET:
        err_mesg = err_mesgs[3];
        break;

    default:
        err_mesg = err_mesgs[4];
        break;
    }

    return err_mesg;
}

void* ctl_loop(void* rover_ptr)
{
    RoverApp* rover = (RoverApp*)rover_ptr;

    pthread_mutex_lock(&rover->write_lock);
    bool running = rover->running;
    pthread_mutex_unlock(&rover->write_lock);

    short steering_angle = 1500;
    short drive_power = 1500;

    struct timeval systime;
    gettimeofday(&systime, NULL);
    long start_time = systime.tv_sec * 1000000 + systime.tv_usec;
    int update_div = 0; // clock divider to avoid flooding Arduino
    int estop_expire = -1;

    while (running)
    {
        pthread_mutex_lock(&rover->write_lock);
        struct GamepadState gamepad_state = rover->gamepad_state;
        running = rover->running;
        rover->steering_angle = steering_angle;
        rover->drive_power = drive_power; // technically these will be one cycle behind but at 100hz this shouldn't be a problem
        pthread_mutex_unlock(&rover->write_lock);
        
        // if E Stop button pressed
        if (gamepad_state.button[B_BTN])
        {
            estop_expire = update_div + 100; // cut input for roughly 1 second
        }

        steering_angle = 1500;
        drive_power = 1500;

        if (update_div > estop_expire)
        {
            if (abs(gamepad_state.axis_lx) > 0.2)
            {
                steering_angle = (short)(gamepad_state.axis_lx * 500.0 + 1500.0);
            }

            if (abs(gamepad_state.axis_ry) > 0.3)
            {
                double axis_pitch_adj = -gamepad_state.axis_ry;
                if (axis_pitch_adj < 0)
                {
                    axis_pitch_adj = (axis_pitch_adj + 0.3) / 0.7;
                }
                else
                {
                    axis_pitch_adj = (axis_pitch_adj - 0.3) / 0.7;
                }

                drive_power = (short)(axis_pitch_adj * 100.0 + 1500.0);
            }
        }

        if (update_div % 10 == 0)
        {
            rover->pwm_gateway.send_mesg(0, (void*)&drive_power, sizeof(short));
            rover->pwm_gateway.send_mesg(1, (void*)&steering_angle, sizeof(short));
        }

        gettimeofday(&systime, NULL);
        long current_time = systime.tv_sec * 1000000 + systime.tv_usec;

        usleep(10 * 1000 - (current_time - start_time));

        gettimeofday(&systime, NULL);
        start_time = systime.tv_sec * 1000000 + systime.tv_usec;
        update_div++;
    }

    steering_angle = 1500;
    drive_power = 1500;

    rover->pwm_gateway.send_mesg(0, (void*)&drive_power, sizeof(short));
    rover->pwm_gateway.send_mesg(1, (void*)&steering_angle, sizeof(short));

    return NULL;
}

void* ros_publish_loop(void* rover_ptr)
{
    RoverApp* rover = (RoverApp*)rover_ptr;
    ros::NodeHandle rosnode;
    ros::Publisher base_publisher = rosnode.advertise<std_msgs::String>("robot_base_state", 4);
    
    pthread_mutex_lock(&rover->write_lock);
    bool running = rover->running;
    pthread_mutex_unlock(&rover->write_lock);
    
    while (running && ros::ok())
    {
        pthread_mutex_lock(&rover->write_lock);
        short steering_angle = rover->steering_angle;
        short drive_power = rover->drive_power;
        running = rover->running;
        pthread_mutex_unlock(&rover->write_lock);
        
        string mesg_data(to_string(steering_angle));
        mesg_data += ",";
        mesg_data += to_string(drive_power);
        
        std_msgs::String mesg;
        mesg.data = mesg_data;
        
        base_publisher.publish(mesg);
        
        usleep(100 * 1000);
    }
    
    return NULL;
}

RoverApp::RoverApp(const struct in_addr& host)
{
    printf("Creating TCP socket for recieving commands...\n");
    cmd_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (cmd_socket < 0)
    {
        throw runtime_error(string("Failed to create TCP socket. socket() error ") + to_string(errno));
    }

    memset(&cmd_host_addr, 0, sizeof(struct sockaddr_in));
    cmd_host_addr.sin_family = AF_INET;         // internet type socket
    cmd_host_addr.sin_port = htons(5309);       // port 5309. need to use correct byte order
    cmd_host_addr.sin_addr.s_addr = host.s_addr;

    printf("Attempting to connect to control host at %s:5309...\n", inet_ntoa(cmd_host_addr.sin_addr));

    if (connect(cmd_socket, (struct sockaddr*)&cmd_host_addr, sizeof(struct sockaddr_in)) == -1)
    {
        close(cmd_socket);
        throw runtime_error(string("Failed to connect to control host. connect() error ") + connect_err_mesg(errno));
    }

    printf("Success! Now listening for commands...\n");
    memset((void*)&gamepad_state, 0, sizeof(struct GamepadState));

    if (pthread_mutex_init(&write_lock, NULL) == -1)
    {
        close(cmd_socket);
        throw runtime_error(string("Failed to create mutex. pthread_mutex_create() error ") + to_string(errno));
    }

    running = true;

    if (pthread_create(&ctl_thread, NULL, &ctl_loop, (void*)this) == -1)
    {
        close(cmd_socket);
        throw runtime_error(string("Failed to create control thread. pthread_create() error ") + to_string(errno));
    }
    
    if (pthread_create(&ros_publish_thread, NULL, &ros_publish_loop, (void*)this) == -1)
    {
        pthread_mutex_lock(&write_lock);
        running = false;
        pthread_mutex_unlock(&write_lock);
        pthread_join(ctl_thread, NULL);
     
        close(cmd_socket);
        pthread_mutex_destroy(&write_lock);
        
        throw runtime_error(string("Failed to create control thread. pthread_create() error ") + to_string(errno));
    }
}

RoverApp::~RoverApp()
{
    pthread_mutex_lock(&write_lock);
    running = false;
    pthread_mutex_unlock(&write_lock);

    pthread_join(ctl_thread, NULL);
    pthread_join(ros_publish_thread, NULL);
    
    // ensure the car stops!!!
    short servo_neutral = 1500;
    pwm_gateway.send_mesg(0, (void*)&servo_neutral, sizeof(short));
    pwm_gateway.send_mesg(1, (void*)&servo_neutral, sizeof(short));

    pthread_mutex_destroy(&write_lock);
    close(cmd_socket);
}

void RoverApp::recieve_cmds()
{
    char mesg[DEFAULT_MESG_SIZE];
    int bytes_recieved = 0;

    while (bytes_recieved > -1)
    {
        memset((void*)mesg, 0, DEFAULT_MESG_SIZE);
        bytes_recieved = recv(cmd_socket, (void*)mesg, DEFAULT_MESG_SIZE, 0);

        if (bytes_recieved == -1)
        {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
            {
                continue;
            }
            else if (errno == ECONNREFUSED)
            {
                printf("Lost connection to host. Stopping...\n");
                close(cmd_socket);
            }
            else
            {
                printf("Connection error. recv() error %d\n", errno);
                close(cmd_socket);
            }
        }
        else if (bytes_recieved == 0)
        {
            printf("Host at %s closed connection. Stopping...\n", inet_ntoa(cmd_host_addr.sin_addr));
            shutdown(cmd_socket, SHUT_RDWR);
            close(cmd_socket);
            break;
        }
        else
        {
            pthread_mutex_lock(&write_lock);
            memcpy((void*)&gamepad_state, (void*)mesg, sizeof(struct GamepadState));
            pthread_mutex_unlock(&write_lock);

            char button[33];
            memset((void*)button, 0, 33);

            for (int index = 0; index < 32; index++)
            {
                button[index] = gamepad_state.button[index] + 0x30;
            }

            printf("LX: % 6.04f    LY: % 6.04f    LT: % 6.04f    RX: % 6.04f    RY: % 6.04f    RT: % 6.04f    %s\r",
                   gamepad_state.axis_lx, gamepad_state.axis_ly, gamepad_state.axis_lt,
                   gamepad_state.axis_rx, gamepad_state.axis_ry, gamepad_state.axis_rt,
                   button);
        }
    }
}
