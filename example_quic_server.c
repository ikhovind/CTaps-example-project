#include <ctaps.h>
#include <stdio.h>
#include <string.h>

void respond_and_close_on_message_received(ct_connection_t* connection, ct_message_t* received_message, ct_message_context_t* message_context) {
    uint16_t port = ct_local_endpoint_get_resolved_port(
        ct_message_context_get_local_endpoint(message_context)
    );

    printf("Received message: %s on port %d\n", ct_message_get_content(received_message), port);

    ct_message_t* message = ct_message_new_with_content(
        ct_message_get_content(received_message),
        ct_message_get_length(received_message)
    );
    // CTaps takes a deep copy, so the message can be freed after this returns
    ct_send_message(connection, message);
    ct_message_free(message);
    // For QUIC this will send FIN flag
    ct_connection_close(connection);
}

void on_connection_received_receive_message(ct_listener_t* listener, ct_connection_t* new_connection) {
    printf("Listener received new connection\n");
    ct_receive_callbacks_t receive_message_request = {
      .receive_callback = respond_and_close_on_message_received,
    };

    ct_receive_message(new_connection, &receive_message_request);
}

void on_connection_ready_receive_message(ct_connection_t* connection) {
    printf("New connection ready\n");
    ct_receive_callbacks_t receive_message_request = {
      .receive_callback = respond_and_close_on_message_received,
    };

    ct_receive_message(connection, &receive_message_request);
}

void free_on_listener_closed(ct_listener_t* listener) {
    ct_listener_free(listener);
}

void free_on_connection_closed(ct_connection_t* connection) {
    ct_connection_free(connection);
}

int main() {
    // Needed to initialize internal CTaps event loop
    ct_initialize();

    ct_set_log_level(CT_LOG_ERROR);

    const ct_local_endpoint_t* listener_endpoint = ct_local_endpoint_new();
    ct_local_endpoint_with_interface(listener_endpoint, "lo");
    ct_local_endpoint_with_port(listener_endpoint, 1234);

    ct_transport_properties_t* listener_props = ct_transport_properties_new();
    ct_transport_properties_set_reliability(listener_props, REQUIRE);
    ct_transport_properties_set_preserve_msg_boundaries(listener_props, REQUIRE);
    // CTaps is currently only able to listen to a single protocol per
    // listener and will choose the most preferable protocol, in this
    // case QUIC
    ct_transport_properties_set_multistreaming(listener_props, REQUIRE);

    ct_security_parameters_t* server_security_parameters = ct_security_parameters_new();
    const char* alpn_strings = "example";
    ct_security_parameters_add_alpn(server_security_parameters, alpn_strings);

    ct_security_parameters_add_server_certificate(server_security_parameters, "resources/cert.pem", "resources/key.pem");

    ct_preconnection_t* listener_precon = ct_preconnection_new(
        &listener_endpoint, 1, NULL, 0, listener_props, server_security_parameters);

    ct_listener_callbacks_t listener_callbacks = {
        .connection_received = on_connection_received_receive_message,
        .listener_closed = free_on_listener_closed
    };

    ct_connection_callbacks_t connection_callbacks = {
        .ready = on_connection_ready_receive_message,
        .closed = free_on_connection_closed
    };

    // The original connection will be received through
    // the listener, but new streams will be received
    // through the connection ready callback.
    //
    // It is implemented this way so that you are able
    // to receive new streams for a QUIC connection even
    // after the listener has closed!
    ct_preconnection_listen(listener_precon, &listener_callbacks, &connection_callbacks);

    // This will run forever, stopping with ctrl + c works, but
    // will skip the cleanup below!
    ct_start_event_loop();

    // CTaps takes deep copies of the passed
    // objects internally, so the application retains
    // ownership of the originals
    ct_security_parameters_free(server_security_parameters);
    ct_local_endpoint_free(listener_endpoint);
    ct_preconnection_free(listener_precon);
    ct_transport_properties_free(listener_props);

    // Needed to free internal event loop
    ct_close();

    return 0;
}
