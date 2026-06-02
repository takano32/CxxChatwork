#include "config.hpp"
#include "hatena_bookmark_client.hpp"
#include "oauth.hpp"
#include "server.hpp"

#include <iostream>
#include <stdexcept>

int main() {
    try {
        chatwork::install_signal_handlers();

        const chatwork::Config config;

        chatwork::OAuth oauth(config.hatena_consumer_key(), config.hatena_consumer_secret());

        if (!config.hatena_access_token().empty()) {
            oauth.set_tokens(config.hatena_access_token(), config.hatena_access_token_secret());
        } else {
            oauth.authorize();
            std::cout << "HATENA_ACCESS_TOKEN=" << oauth.token() << std::endl;
            std::cout << "HATENA_ACCESS_TOKEN_SECRET=" << oauth.token_secret() << std::endl;
        }

        chatwork::HatenaBookmarkClient client(std::move(oauth));

        chatwork::Server server(std::move(client), config.slack_outgoing_token(),
                                config.listen_address(), config.listen_port());

        std::cout << "Listening on " << config.listen_address() << ":" << config.listen_port() << std::endl;

        server.run();
    } catch (const std::exception& error) {
        std::cerr << error.what() << std::endl;
        return 1;
    }

    return 0;
}
