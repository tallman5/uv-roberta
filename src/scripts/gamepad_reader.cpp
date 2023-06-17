#include <iostream>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>

void moveCursorTo(int row, int col)
{
    std::cout << "\033[" << row << ";" << col << "H";
}

struct GamepadEvent
{
    int code;
    int value;
};

int main()
{
    // Clear console
#ifdef _WIN32
    system("cls"); // For Windows
#else
    system("clear"); // For Unix-like systems
#endif

    const char *dev = "/dev/input/by-id/usb-FrSky_FrSky_Simulator_4995316A3546-event-joystick"; // Replace 'X' with the event number of your gamepad
    int fd = open(dev, O_RDONLY);

    if (fd < 0)
    {
        std::cerr << "Failed to open the input device" << std::endl;
        return 1;
    }

    std::cout << "Successfully opened the input device" << std::endl;

    std::vector<GamepadEvent> gamepadEvents(8);

    while (true)
    {
        moveCursorTo(0, 0);

        struct input_event ev;
        ssize_t bytesRead = read(fd, &ev, sizeof(struct input_event));

        if (bytesRead < sizeof(struct input_event))
        {
            std::cerr << "Failed to read input event" << std::endl;
            break;
        }

        // Process the input event
        gamepadEvents[ev.code].code = ev.code;
        gamepadEvents[ev.code].value = ev.value;

        for (const auto &event : gamepadEvents)
        {
            std::cout << "Channel " << event.code << ": " << event.value << "        " << std::endl;
        }

        // if (ev.type == EV_KEY)
        // {
        //     std::cout << "Button " << ev.code << " " << (ev.value ? "pressed" : "released") << std::endl;
        // }
        // else if (ev.type == EV_ABS)
        // {
        //     std::cout << "Axis " << ev.code << " value " << ev.value << std::endl;
        // }
    }

    close(fd);

    return 0;
}
