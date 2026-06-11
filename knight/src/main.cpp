#include "common/log.h"
#include "runtime/runtime.h"
#include "runtime/signal_listener.h"
#include "common/config.h"

#include <stdexcept>
#include <string>
#include <string_view>
#include <fstream>
#include <iterator>
#include <memory>

int main(int argc, char* argv[])
{
    log::initialize();
    log::info("Main", "knight starting...");

    if (argc < 2) {
        log::error("Main", "usage: knight <config_path>");
        return 1;
    }

    const std::string config_path = argv[1];
    log::debug("Main", "config path: {}", config_path);

    std::ifstream config_file(config_path, std::ios::binary);
    if (!config_file) {
        log::error("Main", "failed to open config file: {}", config_path);
        return 1;
    }

    const std::string config_payload{std::istreambuf_iterator<char>{config_file},
                                     std::istreambuf_iterator<char>{}};

    try {
        Config config;
        if (!config.from_string(config_payload)) {
            log::error("Main", "failed to parse config file: {}", config_path);
            return 1;
        }

        log::debug("Main", "creating runtime factory");

        auto factory = std::make_unique<runtime::RuntimeFactory>(std::move(config));

        log::info("Main", "runtime factory created, starting runtime");

        runtime::Runtime bot_runtime{*factory};

        runtime::SignalListener signal_listener([&bot_runtime](int signal) {
            log::info("Main", "received signal {}, requesting shutdown", signal);
            bot_runtime.stop();
        });

        if (!signal_listener.start()) {
            log::error("Main", "failed to start signal listener");
            return 1;
        }

        bot_runtime.run();
        signal_listener.stop();

        log::info("Main", "knight finished successfully");

        return 0;
    } catch (const std::exception& e) {
        log::error("Main", "fatal error: {}", e.what());
        return 1;
    }
}
