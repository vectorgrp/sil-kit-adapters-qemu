// Copyright (c) Vector Informatik GmbH. All rights reserved.

struct Options
{
    Options(int argc, char **argv)
    {
        if (argc < 3)
        {
            qemu_plugin_outs("Please pass all mandatory options: Path To Host to Guest Pipe, Path To Guest to Host Pipe, Advance Time in Micro Seconds\n");
            throw std::runtime_error("Too few arguments");
        }

        for (int i = 0; i < argc; i++)
        {
            auto option = std::string(argv[i]);
            auto equalSignPos = option.find("=");
            auto key = option.substr(0, equalSignPos);
            auto value = option.substr(equalSignPos + 1);

            if (key == "HostToGuestPipe")
            {
                HostToGuestPipe = value;
                continue;
            }

            if (key == "GuestToHostPipe")
            {
                GuestToHostPipe = value;
                continue;
            }

            if (key == "TimeInMicroSeconds")
            {
                AdvanceTimeInMicroSeconds = std::stoi(value);
                continue;
            }

            throw std::runtime_error("Invalid Option: " + key);
        }
    }

    std::string HostToGuestPipe;
    std::string GuestToHostPipe;
    uint32_t AdvanceTimeInMicroSeconds;
};